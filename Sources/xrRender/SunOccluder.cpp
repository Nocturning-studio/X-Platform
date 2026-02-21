#include "stdafx.h"
#include "SunOccluder.h"

// Простая структура вершины. Для теней нам нужна только позиция.
struct v_occluder
{
	float3 P;
};

// Формат вершин: только позиция
static const u32 v_occluder_fvf = D3DFVF_XYZ;

CSunOccluder::CSunOccluder()
{
	m_VB = NULL;
	m_IB = NULL;
	m_VertexCount = 0;
	m_IndexCount = 0;
	m_Loaded = false;
	m_Shader.create("sun_occluder");
}

CSunOccluder::~CSunOccluder()
{
	Unload();
}

void CSunOccluder::Load()
{
	// Путь к файлу: gamedata/levels/<current_level>/sun_occluder.obj
	string_path fn;
	if (!FS.exist(fn, "$level$", "sun_occluder.obj"))
	{
		Msg("! [SunOccluder] File not found: %s", fn);
		return;
	}

	IReader* F = FS.r_open(fn);
	if (!F)
		return;

	xr_vector<float3> temp_verts;
	xr_vector<u16> temp_inds;

	string512 line;
	while (!F->eof())
	{
		F->r_string(line, sizeof(line));

		// Пропускаем пустые строки
		if (line[0] == 0)
			continue;

		if (line[0] == 'v' && line[1] == ' ')
		{
			// Вершина (v x y z)
			float3 v;
			float x, y, z;
			sscanf(line + 2, "%f %f %f", &x, &y, &z);

			// !!! ИЗМЕНЕНИЕ 1: Инвертируем X для зеркального отражения
			v.set(-x, y, z);

			temp_verts.push_back(v);
		}
		else if (line[0] == 'f' && line[1] == ' ')
		{
			// Грань (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3)
			int i1 = 0, i2 = 0, i3 = 0;
			char* p = line + 2; // Пропускаем "f "

			auto parse_index = [&](char*& ptr) -> int {
				while (*ptr == ' ' || *ptr == '\t')
					ptr++;
				if (*ptr == 0)
					return 0;
				int val = atoi(ptr);
				while (*ptr != 0 && *ptr != ' ' && *ptr != '\t')
					ptr++;
				return val;
			};

			i1 = parse_index(p);
			i2 = parse_index(p);
			i3 = parse_index(p);

			// Проверка на корректность индексов
			if (i1 > 0 && i2 > 0 && i3 > 0)
			{
				if (i1 <= temp_verts.size() && i2 <= temp_verts.size() && i3 <= temp_verts.size())
				{
					// !!! ИЗМЕНЕНИЕ 2: Свапаем порядок индексов (i2 и i3 местами).
					// Поскольку мы отзеркалили одну ось (X), порядок обхода вершин
					// изменился на противоположный. Нужно вернуть его обратно,
					// иначе Backface Culling скроет лицевые грани.

					temp_inds.push_back((u16)(i1 - 1));
					temp_inds.push_back((u16)(i3 - 1)); // Было i2
					temp_inds.push_back((u16)(i2 - 1)); // Было i3
				}
			}
		}
	}
	FS.r_close(F);

	m_VertexCount = temp_verts.size();
	m_IndexCount = temp_inds.size();

	if (m_VertexCount == 0 || m_IndexCount == 0)
	{
		Msg("! [SunOccluder] Error: Mesh is empty!");
		return;
	}

	Msg("* [SunOccluder] Loaded: %d verts, %d indices", m_VertexCount, m_IndexCount);

	// --- Создание буферов ---

	u32 vSize = sizeof(v_occluder);
	R_CHK(HW.GetDevice()->CreateVertexBuffer(m_VertexCount * vSize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_VB, 0));

	v_occluder* pV;
	R_CHK(m_VB->Lock(0, 0, (void**)&pV, 0));
	for (u32 i = 0; i < m_VertexCount; i++)
	{
		pV[i].P = temp_verts[i];
	}
	R_CHK(m_VB->Unlock());

	R_CHK(
		HW.GetDevice()->CreateIndexBuffer(m_IndexCount * 2, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_IB, 0));

	u16* pI;
	R_CHK(m_IB->Lock(0, 0, (void**)&pI, 0));

	CopyMemory(pI, &temp_inds[0], m_IndexCount * sizeof(u16));

	R_CHK(m_IB->Unlock());

	// Декларация
	static D3DVERTEXELEMENT9 dwDecl[] = {{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
										 D3DDECL_END()};
	m_Geom.create(dwDecl, m_VB, m_IB);

	m_Loaded = true;
}

void CSunOccluder::Unload()
{
	m_Geom.destroy();
	_RELEASE(m_IB);
	_RELEASE(m_VB);
	m_Loaded = false;
}

void CSunOccluder::Render()
{
	PROFILE_FUNCTION();

	if (!m_Loaded)
		return;

	RenderBackend.set_Geometry(m_Geom);
	RenderBackend.set_transform_world(Fidentity);
	RenderBackend.set_Shader(m_Shader);

	u32 primCount = m_IndexCount / 3;
	RenderBackend.Render(D3DPT_TRIANGLELIST, 0, 0, m_VertexCount, 0, primCount);
}
