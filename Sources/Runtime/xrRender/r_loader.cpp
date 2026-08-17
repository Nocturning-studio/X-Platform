#include "stdafx.h"
#include "render.h"
#include "../xrEngine/resourcemanager.h"
#include "../xrEngine/fbasicvisual.h"
#include "../xrEngine/fmesh.h"
#include "../xrEngine/xrLevel.h"
#include "../xrEngine/Engine.h"
#include "../xrEngine/IGame_Persistent.h"
#include "../xrCore/stream_reader.h"
#include "../xrEngine/xr_ioconsole.h"
#include "../xrEngine/LevelLoadingScreen.h"

#include <ppl.h>
#include <future>
#include <atomic>

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

struct ShaderRequest
{
	shared_str name;
	shared_str textures;
};

void CRender::LevelLoad(IReader* fs)
{
	R_ASSERT(0 != g_pGameLevel);
	R_ASSERT(!SceneGraph.b_loaded);

	// Группа задач для Визуалов
	concurrency::task_group tg_visuals;
	std::atomic<int> active_tasks = 0;

	// Хелпер
	auto run_task = [&](concurrency::task_group& tg, auto func) {
		active_tasks++;
		tg.run([func, &active_tasks]() {
			func();
			active_tasks--;
		});
	};

	Engine.LoadingScreen->Show();
	Engine.ResourceManager->DeferredLoad(TRUE);

	// RAM BUFFERING
	fs->seek(0);
	u32 level_size = fs->elapsed();
	u8* level_data_ptr = (u8*)xr_malloc(level_size);
	fs->r(level_data_ptr, level_size);
	IReader mem_fs(level_data_ptr, level_size);

	g_pGamePersistent->LoadTitle("st_loading_components");

	// --- ОПТИМИЗАЦИЯ СЕРВЕРА: Пропускаем создание визуальных эффектов ---
	if (!g_dedicated_server)
	{
		Wallmarks = xr_new<CWallmarksEngine>();
		Details = xr_new<CDetailManager>();
		m_SunOccluder = xr_new<CSunOccluder>();
	}
	else
	{
		Wallmarks = nullptr;
		Details = nullptr;
		m_SunOccluder = nullptr;
	}
	// ---------------------------------------------------------------------

	// =================================================================================
	// ШЕЙДЕРЫ (MAIN THREAD - СИНХРОННО)
	// =================================================================================
	g_pGamePersistent->LoadTitle("st_loading_shaders");
	{
		// RT шейдеры нужны только для картинки
		if (!g_dedicated_server)
		{
			Msg("* Compiling RenderTarget shaders...");
			if (RenderTarget)
				RenderTarget->CompileShaders();
		}

		IReader* chunk = mem_fs.open_chunk(fsL_SHADERS);
		if (chunk)
		{
			u32 count = chunk->r_u32();
			Shaders.resize(count);

			for (u32 i = 0; i < count; i++)
			{
				string512 n_sh, n_tlist;
				LPCSTR n = LPCSTR(chunk->pointer());
				chunk->skip_stringZ();

				if (0 == n[0])
					continue;

				strcpy(n_sh, n);
				LPSTR delim = strchr(n_sh, '/');
				*delim = 0;
				strcpy(n_tlist, delim + 1);

				Shaders[i] = Engine.ResourceManager->Create(n_sh, n_tlist);
			}
			chunk->close();
		}
	}

	// Геометрия
	{
		g_pGamePersistent->LoadTitle("st_loading_geometry");

		CStreamReader* geom = FS.rs_open("$level$", "level.geom");
		R_ASSERT2(geom, "level.geom");
		LoadBuffers(geom, FALSE);
		LoadSWIs(geom);
		FS.r_close(geom);

		Engine.LoadingScreen->ForceRender();

		g_pGamePersistent->LoadTitle("st_loading_fast_geometry");

		CStreamReader* geomx = FS.rs_open("$level$", "level.geomx");
		R_ASSERT2(geomx, "level.geomX");
		LoadBuffers(geomx, TRUE);
		FS.r_close(geomx);
	}

	g_pGamePersistent->LoadTitle("st_loading_spatial_db");

	// =================================================================================
	// ПАРАЛЛЕЛЬНАЯ ЗАГРУЗКА ОБЪЕКТОВ
	// =================================================================================

	// ЗАДАЧА A: Визуалы
	// Загружаем даже на сервере, так как они нужны для RayPick'ов и определения хитбоксов
	// в некоторых старых реализациях, а также для предотвращения пустых ссылок.
	IReader local_fs(level_data_ptr, level_size);
	LoadVisuals(&local_fs);

	if (!g_dedicated_server)
	{
		// ЗАДАЧА A: HOM (Hierarchical Occlusion Culling) - только для рендеринга
		run_task(tg_visuals, [this]() { HOM.Load(); CPUOCC.Load(HOM); });

		// ЗАДАЧА B: Детейлы (Трава)
		run_task(tg_visuals, [this]() { Details->Load(); });

		// ЗАДАЧА C: Sun Occluder
		run_task(tg_visuals, [this]() { m_SunOccluder->Load(); });
	}

	// === ACTIVE WAIT ===
	while (active_tasks > 0)
	{
		Engine.LoadingScreen->ForceRender();
		Sleep(1);
	}
	tg_visuals.wait();

	// Финализация (Main Thread)
	g_pGamePersistent->LoadTitle("st_loading_sectors_portals");
	{
		IReader local_fs_sectors(level_data_ptr, level_size);
		LoadSectors(&local_fs_sectors);
	}

	Engine.LoadingScreen->ForceRender();

	g_pGamePersistent->LoadTitle("st_loading_lights");
	{
		if (!g_dedicated_server)
		{
			IReader local_fs_lights(level_data_ptr, level_size);
			LoadLights(&local_fs_lights);
		}
	}
	// ----------------------------------------------------------------------------

	xr_free(level_data_ptr);

	// Очищаем списки через m_packet
	SceneGraph.m_packet.lstLODs.clear();
	SceneGraph.m_packet.lstLODgroups.clear();
	SceneGraph.m_packet.mapLOD.clear();

	SceneGraph.b_loaded = TRUE;
}

void CRender::LevelUnload()
{
	if (0 == g_pGameLevel)
		return;
	if (!SceneGraph.b_loaded)
		return;

	WaitForPendingTasks();

	u32 I;

	// HOM
	g_pGamePersistent->LoadTitle("st_unloading_hom");
	CPUOCC.Unload();
	HOM.Unload();

	if (m_SunOccluder)
	{
		m_SunOccluder->Unload();
		xr_delete(m_SunOccluder);
	}

	//*** Details
	if (Details)
	{
		g_pGamePersistent->LoadTitle("st_unloading_details");
		Details->Unload();
		xr_delete(Details);
	}
	// ----------------------------------------------

	//*** Sectors
	g_pGamePersistent->LoadTitle("st_unloading_sectors_portals");
	xr_delete(rmPortals);
	pLastSector = 0;
	vLastCameraPos.set(0, 0, 0);
	for (I = 0; I < Sectors.size(); I++)
		xr_delete(Sectors[I]);
	Sectors.clear();
	Portals.clear();

	//*** Lights
	g_pGamePersistent->LoadTitle("st_unloading_lights");
	Lights.Unload();

	//*** Visuals
	g_pGamePersistent->LoadTitle("st_unloading_spatial_db");
	for (I = 0; I < Visuals.size(); I++)
	{
		Visuals[I]->Release();
		xr_delete(Visuals[I]);
	}
	Visuals.clear();

	//*** VB/IB
	g_pGamePersistent->LoadTitle("st_unloading_geometry");
	for (I = 0; I < nVB.size(); I++)
		_RELEASE(nVB[I]);
	for (I = 0; I < xVB.size(); I++)
		_RELEASE(xVB[I]);
	nVB.clear();
	xVB.clear();
	for (I = 0; I < nIB.size(); I++)
		_RELEASE(nIB[I]);
	for (I = 0; I < xIB.size(); I++)
		_RELEASE(xIB[I]);
	nIB.clear();
	xIB.clear();
	nDC.clear();
	xDC.clear();

	//*** Components
	g_pGamePersistent->LoadTitle("st_unloading_components");

	// Details уже удален выше
	if (Wallmarks)
	{
		xr_delete(Wallmarks);
	}

	//*** Shaders
	g_pGamePersistent->LoadTitle("st_unloading_shaders");
	Shaders.clear_and_free();

	SceneGraph.b_loaded = FALSE;
}

void CRender::LoadBuffers(CStreamReader* base_fs, BOOL _alternative)
{
	R_ASSERT2(base_fs, "Could not load geometry. File not found.");
	Engine.ResourceManager->Evict();
	u32 dwUsage = D3DUSAGE_WRITEONLY;

	xr_vector<VertexDeclarator>& _DC = _alternative ? xDC : nDC;
	xr_vector<IDirect3DVertexBuffer9*>& _VB = _alternative ? xVB : nVB;
	xr_vector<IDirect3DIndexBuffer9*>& _IB = _alternative ? xIB : nIB;

	// Vertex buffers
	{
		CStreamReader* fs = base_fs->open_chunk(fsL_VB);
		R_ASSERT2(fs, "Could not load geometry. File 'level.geom?' corrupted.");
		u32 count = fs->r_u32();
		_DC.resize(count);
		_VB.resize(count);

		// Используем временный буфер для чтения
		xr_vector<u8> temp_buffer;

		for (u32 i = 0; i < count; i++)
		{
			// Читаем декларацию
			u32 buffer_size = (MAXD3DDECLLENGTH + 1) * sizeof(D3DVERTEXELEMENT9);
			D3DVERTEXELEMENT9* dcl = (D3DVERTEXELEMENT9*)_alloca(buffer_size);
			fs->r(dcl, buffer_size);
			fs->advance(-(int)buffer_size);

			u32 dcl_len = D3DXGetDeclLength(dcl) + 1;
			_DC[i].resize(dcl_len);
			fs->r(_DC[i].begin(), dcl_len * sizeof(D3DVERTEXELEMENT9));

			// Читаем данные вершин
			u32 vCount = fs->r_u32();
			u32 vSize = D3DXGetDeclVertexSize(dcl, 0);
			u32 byteSize = vCount * vSize;

			Msg("* [Loading VB] %d verts, %d Kb", vCount, byteSize / 1024);

			// Читаем в RAM
			temp_buffer.resize(byteSize);
			fs->r(temp_buffer.data(), byteSize);

			// Создаем буфер
			R_CHK(RenderBackend.GetDevice()->CreateVertexBuffer(byteSize, dwUsage, 0, D3DPOOL_DEFAULT, &_VB[i], 0));

			// Копируем из RAM в VRAM
			void* pData = 0;
			R_CHK(_VB[i]->Lock(0, 0, (void**)&pData, 0));
			CopyMemory(pData, temp_buffer.data(), byteSize);
			_VB[i]->Unlock();
		}
		fs->close();
	}

	// Index buffers
	{
		CStreamReader* fs = base_fs->open_chunk(fsL_IB);
		u32 count = fs->r_u32();
		_IB.resize(count);

		xr_vector<u8> temp_buffer;

		for (u32 i = 0; i < count; i++)
		{
			u32 iCount = fs->r_u32();
			u32 byteSize = iCount * 2;
			Msg("* [Loading IB] %d indices, %d Kb", iCount, byteSize / 1024);

			// ОПТИМИЗАЦИЯ: Читаем в RAM
			temp_buffer.resize(byteSize);
			fs->r(temp_buffer.data(), byteSize);

			// Создаем и копируем
			void* pData = 0;
			R_CHK(RenderBackend.GetDevice()->CreateIndexBuffer(byteSize, dwUsage, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &_IB[i], 0));
			R_CHK(_IB[i]->Lock(0, 0, (void**)&pData, 0));
			CopyMemory(pData, temp_buffer.data(), byteSize);
			_IB[i]->Unlock();
		}
		fs->close();
	}
}

void CRender::LoadVisuals(IReader* fs)
{
	IReader* chunk = 0;
	u32 index = 0;
	IRender_Visual* V = 0;
	ogf_header H;

	IReader* main_chunk = fs->open_chunk(fsL_VISUALS);
	if (!main_chunk)
		return;

	while ((chunk = main_chunk->open_chunk(index)) != 0)
	{
		chunk->r_chunk_safe(OGF_HEADER, &H, sizeof(H));

		V = Models->Instance_Create(H.type);
		V->Load(0, chunk, 0);
		Visuals.push_back(V);

		chunk->close();
		index++;
	}
	main_chunk->close();
}

void CRender::LoadLights(IReader* fs)
{
	Lights.Load(fs);
}

struct b_portal
{
	u16 sector_front;
	u16 sector_back;
	svector<fvec3, 6> vertices;
};

void CRender::LoadSectors(IReader* fs)
{
	// allocate memory for portals
	u32 size = fs->find_chunk(fsL_PORTALS);
	R_ASSERT(0 == size % sizeof(b_portal));
	u32 count = size / sizeof(b_portal);
	Portals.resize(count);
	for (u32 c = 0; c < count; c++)
		Portals[c] = xr_new<CPortal>();

	// load sectors
	IReader* S = fs->open_chunk(fsL_SECTORS);
	for (u32 i = 0;; i++)
	{
		IReader* P = S->open_chunk(i);
		if (0 == P)
			break;

		CSector* __S = xr_new<CSector>();
		__S->Load(*P);
		Sectors.push_back(__S);

		P->close();
	}
	S->close();

	// load portals
	if (count)
	{
		CDB::Collector CL;
		fs->find_chunk(fsL_PORTALS);
		for (u32 i = 0; i < count; i++)
		{
			b_portal P;
			fs->r(&P, sizeof(P));
			CPortal* __P = (CPortal*)Portals[i];

			__P->Setup(P.vertices.begin(), P.vertices.size(), (CSector*)getSector(P.sector_front),
					   (CSector*)getSector(P.sector_back));

			for (u32 j = 2; j < P.vertices.size(); j++)
				CL.add_face_packed_D(P.vertices[0], P.vertices[j - 1], P.vertices[j], u32(i));
		}
		if (CL.getTS() < 2)
		{
			fvec3 v1, v2, v3;
			v1.set(-20000.f, -20000.f, -20000.f);
			v2.set(-20001.f, -20001.f, -20001.f);
			v3.set(-20002.f, -20002.f, -20002.f);
			CL.add_face_packed_D(v1, v2, v3, 0);
		}

		// build portal model
		rmPortals = xr_new<CDB::MODEL>();
		rmPortals->build(CL.getV(), int(CL.getVS()), CL.getT(), int(CL.getTS()));
	}
	else
	{
		rmPortals = 0;
	}

	pLastSector = 0;
}

void CRender::LoadSWIs(CStreamReader* base_fs)
{
	// allocate memory for portals
	if (base_fs->find_chunk(fsL_SWIS))
	{
		CStreamReader* fs = base_fs->open_chunk(fsL_SWIS);
		u32 item_count = fs->r_u32();

		xr_vector<FSlideWindowItem>::iterator it = SWIs.begin();
		xr_vector<FSlideWindowItem>::iterator it_e = SWIs.end();

		for (; it != it_e; ++it)
			xr_free((*it).sw);

		SWIs.clear_not_free();

		SWIs.resize(item_count);
		for (u32 c = 0; c < item_count; c++)
		{
			FSlideWindowItem& swi = SWIs[c];
			swi.reserved[0] = fs->r_u32();
			swi.reserved[1] = fs->r_u32();
			swi.reserved[2] = fs->r_u32();
			swi.reserved[3] = fs->r_u32();
			swi.count = fs->r_u32();
			VERIFY(NULL == swi.sw);
			swi.sw = xr_alloc<FSlideWindow>(swi.count);
			fs->r(swi.sw, sizeof(FSlideWindow) * swi.count);
		}
		fs->close();
	}
}
