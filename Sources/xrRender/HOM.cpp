#include "stdafx.h"
#include "HOM.h"
#include "occRasterizer.h"
#include "../xrEngine/GameFont.h"
#include <algorithm>

float psOSSR = .001f;

// -----------------------------------------------------------------------------
void CHOM::StartFrame()
{
	// Меняем буферы местами
	// То, что записали в прошлом кадре (Write), теперь становится доступно для чтения (Read)
	// А старый буфер чтения отдаем на перезапись
	std::swap(m_idx_read, m_idx_write);
}

void __stdcall CHOM::MT_RENDER()
{
	PROFILE_FUNCTION();

	if (g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive())
		return;

	// Мы пишем в m_idx_write, а Main Thread читает из m_idx_read.
	CFrustum ViewBase;
	ViewBase.CreateFromMatrix(Engine.RenderView.ViewProjection, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
	Enable();
	Render(ViewBase);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHOM::CHOM()
{
	bEnabled = FALSE;
	m_pModel = 0;
	m_pTris = 0;
	m_idx_read = 0;
	m_idx_write = 1;

#ifdef DEBUG
	Engine.Events.Render.Add(this, REG_PRIORITY_LOW - 1000);
#endif
}

CHOM::~CHOM()
{
#ifdef DEBUG
	Engine.Events.Render.Remove(this);
#endif
}

#pragma pack(push, 4)
struct HOM_poly
{
	fvec3 v1, v2, v3;
	u32 flags;
};
#pragma pack(pop)

IC float Area(fvec3& v0, fvec3& v1, fvec3& v2)
{
	float e1 = v0.distance_to(v1);
	float e2 = v0.distance_to(v2);
	float e3 = v1.distance_to(v2);

	float p = (e1 + e2 + e3) / 2.f;
	return _sqrt(p * (p - e1) * (p - e2) * (p - e3));
}

void CHOM::Load()
{
	string_path fName;
	FS.update_path(fName, "$level$", "level.hom");
	if (!FS.exist(fName))
	{
		Msg(" WARNING: Occlusion map '%s' not found.", fName);
		return;
	}
	Msg("* Loading HOM: %s", fName);

	IReader* fs = FS.r_open(fName);
	IReader* S = fs->open_chunk(1);

	CDB::Collector CL;
	while (!S->eof())
	{
		HOM_poly P;
		S->r(&P, sizeof(P));
		CL.add_face_packed_D(P.v1, P.v2, P.v3, P.flags, 0.01f);
	}

	xr_vector<u32> adjacency;
	CL.calc_adjacency(adjacency);

	m_pTris = xr_alloc<occTri>(u32(CL.getTS()));
	for (u32 it = 0; it < CL.getTS(); it++)
	{
		CDB::TRI& clT = CL.getT()[it];
		occTri& rT = m_pTris[it];
		fvec3& v0 = CL.getV()[clT.verts[0]];
		fvec3& v1 = CL.getV()[clT.verts[1]];
		fvec3& v2 = CL.getV()[clT.verts[2]];
		rT.adjacent[0] = (0xffffffff == adjacency[3 * it + 0]) ? ((occTri*)(-1)) : (m_pTris + adjacency[3 * it + 0]);
		rT.adjacent[1] = (0xffffffff == adjacency[3 * it + 1]) ? ((occTri*)(-1)) : (m_pTris + adjacency[3 * it + 1]);
		rT.adjacent[2] = (0xffffffff == adjacency[3 * it + 2]) ? ((occTri*)(-1)) : (m_pTris + adjacency[3 * it + 2]);
		rT.flags = clT.dummy;
		rT.area = Area(v0, v1, v2);
		rT.plane.build(v0, v1, v2);
		rT.skip = 0;
		rT.center.add(v0, v1).add(v2).div(3.f);
	}

	m_pModel = xr_new<CDB::MODEL>();
	m_pModel->build(CL.getV(), int(CL.getVS()), CL.getT(), int(CL.getTS()));
	bEnabled = TRUE;
	S->close();
	FS.r_close(fs);
}

void CHOM::Unload()
{
	xr_delete(m_pModel);
	xr_free(m_pTris);
	bEnabled = FALSE;
}

class pred_fb
{
  public:
	occTri* m_pTris;
	fvec3 camera;

  public:
	pred_fb(occTri* _t) : m_pTris(_t)
	{
	}
	pred_fb(occTri* _t, fvec3& _c) : m_pTris(_t), camera(_c)
	{
	}
	ICF bool operator()(const CDB::RESULT& _1, const CDB::RESULT& _2) const
	{
		occTri& t0 = m_pTris[_1.id];
		occTri& t1 = m_pTris[_2.id];
		return camera.distance_to_sqr(t0.center) < camera.distance_to_sqr(t1.center);
	}
	ICF bool operator()(const CDB::RESULT& _1) const
	{
		occTri& T = m_pTris[_1.id];
		return T.skip > Engine.TimeManager.GetFrameCount();
	}
};

void CHOM::ProcessTriangle(CDB::RESULT* it, u32 _frame, const fvec3& COP, CFrustum& clip)
{
	sPoly src, dst;
	occTri& T = m_pTris[it->id];
	u32 next = _frame + ::Random.randI(3, 10);

	if (!(T.flags || (T.plane.classify(COP) > 0)))
	{
		T.skip = next;
		return;
	}

	CDB::TRI& t = m_pModel->get_tris()[it->id];
	fvec3* fvert = m_pModel->get_verts();
	src.clear();
	src.push_back(fvert[t.verts[0]]);
	src.push_back(fvert[t.verts[1]]);
	src.push_back(fvert[t.verts[2]]);

	sPoly* P = clip.ClipPoly(src, dst);
	if (0 == P)
	{
		T.skip = next;
		return;
	}

#ifdef DEBUG
	InterlockedIncrement(&tris_in_frame_visible);
#endif
	u32 pixels = 0;
	int limit = int(P->size()) - 1;
	for (int vert_it = 1; vert_it < limit; vert_it++)
	{
		m_transform.transform(T.raster[0], (*P)[0]);
		m_transform.transform(T.raster[1], (*P)[vert_it + 0]);
		m_transform.transform(T.raster[2], (*P)[vert_it + 1]);

		// ВАЖНО: Используем буфер ЗАПИСИ
		pixels += m_Raster[m_idx_write].rasterize(&T);
	}
	if (0 == pixels)
	{
		T.skip = next;
	}
}

void CHOM::Render_DB(CFrustum& base)
{
	xrc.frustum_options(0);
	xrc.frustum_query(m_pModel, base);
	if (0 == xrc.r_count())
		return;

	CDB::RESULT* it = xrc.r_begin();
	CDB::RESULT* end = xrc.r_end();

	fvec3 COP = Engine.RenderView.Position;

	end = std::remove_if(it, end, pred_fb(m_pTris));

	if (it == end)
		return;

	// Сортировка - здесь можно оставить параллельность (сортировка безопасна)
	// Но для простоты оставим std::sort, так как треугольников HOM обычно не миллионы.
	std::sort(it, end, pred_fb(m_pTris, COP));

	float view_dim = occ_dim_0;
	fmat4x4 m_viewport = {view_dim / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, -view_dim / 2.f,		  0.0f,
						  0.0f,			  0.0f, 0.0f, 1.0f, 0.0f, view_dim / 2.f + 0 + 0, view_dim / 2.f + 0 + 0,
						  0.0f,			  1.0f};
	fmat4x4 m_viewport_01 = {1.f / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, -1.f / 2.f,		   0.0f,
							 0.0f,		0.0f, 0.0f, 1.0f, 0.0f, 1.f / 2.f + 0 + 0, 1.f / 2.f + 0 + 0,
							 0.0f,		1.0f};
	m_transform.mul(m_viewport, Engine.RenderView.ViewProjection);
	m_transform_01.mul(m_viewport_01, Engine.RenderView.ViewProjection);

	CFrustum clip;
	clip.CreateFromMatrix(Engine.RenderView.ViewProjection, FRUSTUM_P_NEAR);
	u32 _frame = Engine.TimeManager.GetFrameCount();
#ifdef DEBUG
	tris_in_frame = xrc.r_count();
	tris_in_frame_visible = 0;
#endif

	// ИСПРАВЛЕНИЕ 2: Убрана параллельная растеризация.
	// Raster.rasterize пишет в общий буфер без блокировок. Параллелить это нельзя.
	for (; it != end; it++)
	{
		ProcessTriangle(it, _frame, COP, clip);
	}
}

void CHOM::Render(CFrustum& base)
{
	if (!bEnabled)
		return;

	Engine.Statistic->RenderCALC_HOM.Begin();

	// Используем буфер для ЗАПИСИ
	m_Raster[m_idx_write].clear();
	Render_DB(base);
	m_Raster[m_idx_write].propagade();

	Engine.Statistic->RenderCALC_HOM.End();
}

// =============================================================================
// Helper Math Functions (Projection & AABB calculation)
// =============================================================================

// Проецирует первую точку (начало расчета Bounding Rect)
ICF BOOL transform_b0(fvec2& min, fvec2& max, float& minz, fmat4x4& X, float _x, float _y, float _z)
{
	float z = _x * X._13 + _y * X._23 + _z * X._33 + X._43;
	// Если точка за ближней плоскостью отсечения (сзади камеры) - объект считается видимым
	if (z < EPS)
		return TRUE;

	float iw = 1.f / (_x * X._14 + _y * X._24 + _z * X._34 + X._44);

	min.x = max.x = (_x * X._11 + _y * X._21 + _z * X._31 + X._41) * iw;
	min.y = max.y = (_x * X._12 + _y * X._22 + _z * X._32 + X._42) * iw;
	minz = 0.f + z * iw;

	return FALSE;
}

// Проецирует последующие точки (расширение Bounding Rect)
ICF BOOL transform_b1(fvec2& min, fvec2& max, float& minz, fmat4x4& X, float _x, float _y, float _z)
{
	float t;
	float z = _x * X._13 + _y * X._23 + _z * X._33 + X._43;
	// Если точка за ближней плоскостью отсечения
	if (z < EPS)
		return TRUE;

	float iw = 1.f / (_x * X._14 + _y * X._24 + _z * X._34 + X._44);

	// X
	t = (_x * X._11 + _y * X._21 + _z * X._31 + X._41) * iw;
	if (t < min.x)
		min.x = t;
	else if (t > max.x)
		max.x = t;

	// Y
	t = (_x * X._12 + _y * X._22 + _z * X._32 + X._42) * iw;
	if (t < min.y)
		min.y = t;
	else if (t > max.y)
		max.y = t;

	// Z (нам нужен minZ, то есть самая близкая точка к камере)
	t = 0.f + z * iw;
	if (t < minz)
		minz = t;

	return FALSE;
}

// Проверка видимости для AABB (8 углов)
IC BOOL _visible(Fbox& B, fmat4x4& m_transform_01, occRasterizer& raster)
{
	fvec2 min, max;
	float z;

	if (transform_b0(min, max, z, m_transform_01, B.min.x, B.min.y, B.min.z))
		return TRUE;
	if (transform_b1(min, max, z, m_transform_01, B.min.x, B.min.y, B.max.z))
		return TRUE;
	if (transform_b1(min, max, z, m_transform_01, B.max.x, B.min.y, B.max.z))
		return TRUE;
	if (transform_b1(min, max, z, m_transform_01, B.max.x, B.min.y, B.min.z))
		return TRUE;
	if (transform_b1(min, max, z, m_transform_01, B.min.x, B.max.y, B.min.z))
		return TRUE;
	if (transform_b1(min, max, z, m_transform_01, B.min.x, B.max.y, B.max.z))
		return TRUE;
	if (transform_b1(min, max, z, m_transform_01, B.max.x, B.max.y, B.max.z))
		return TRUE;
	if (transform_b1(min, max, z, m_transform_01, B.max.x, B.max.y, B.min.z))
		return TRUE;

	return raster.test(min.x, min.y, max.x, max.y, z);
}

// =============================================================================
// Public Interface Implementation
// =============================================================================

BOOL CHOM::visible(Fbox3& B)
{
	if (!bEnabled)
		return TRUE;
	if (B.contains(Engine.RenderView.Position))
		return TRUE;

	// Читаем из ГОТОВОГО буфера (прошлый кадр)
	return _visible(B, m_transform_01, m_Raster[m_idx_read]);
}

BOOL CHOM::visible(Fbox2& B, float depth)
{
	if (!bEnabled)
		return TRUE;
	return m_Raster[m_idx_read].test(B.min.x, B.min.y, B.max.x, B.max.y, depth);
}

BOOL CHOM::visible(vis_data& vis)
{
	if (Engine.TimeManager.GetFrameCount() < vis.hom_frame)
		return TRUE;

	if (!bEnabled)
		return TRUE;

	u32 frame_current = Engine.TimeManager.GetFrameCount();

#ifdef DEBUG
	Engine.Statistic->RenderCALC_HOM.Begin();
#endif

	// Читаем из ГОТОВОГО буфера
	BOOL result = _visible(vis.box, m_transform_01, m_Raster[m_idx_read]);

	u32 delay = 1;
	if (result)
	{
		delay = ::Random.randI(5 * 2, 5 * 5);
	}
	else
	{
		delay = 1;
	}

	vis.hom_frame = frame_current + delay;
	vis.hom_tested = frame_current;

#ifdef DEBUG
	Engine.Statistic->RenderCALC_HOM.End();
#endif

	return result;
}


BOOL CHOM::visible(sPoly& P)
{
	if (!bEnabled)
		return TRUE;

	fvec2 min, max;
	float z;

	if (P.empty())
		return TRUE;

	if (transform_b0(min, max, z, m_transform_01, P.front().x, P.front().y, P.front().z))
		return TRUE;

	for (u32 it = 1; it < P.size(); it++)
		if (transform_b1(min, max, z, m_transform_01, P[it].x, P[it].y, P[it].z))
			return TRUE;

	return m_Raster[m_idx_read].test(min.x, min.y, max.x, max.y, z);
}

void CHOM::Disable()
{
	bEnabled = FALSE;
}

void CHOM::Enable()
{
	bEnabled = m_pModel ? TRUE : FALSE;
}

// =============================================================================
// Debug Functions
// =============================================================================

#ifdef DEBUG
void CHOM::OnRender()
{
	// Отрисовка геометрии HOM (Occluders) для отладки
	if (psDeviceFlags.is(rsOcclusionDraw))
	{
		if (m_pModel)
		{
			DEFINE_VECTOR(FVF::L, LVec, LVecIt);
			static LVec poly;
			poly.resize(m_pModel->get_tris_count() * 3);
			static LVec line;
			line.resize(m_pModel->get_tris_count() * 6);

			for (int it = 0; it < m_pModel->get_tris_count(); it++)
			{
				CDB::TRI* T = m_pModel->get_tris() + it;
				fvec3* verts = m_pModel->get_verts();

				// Заполнение треугольников (полупрозрачные)
				poly[it * 3 + 0].set(*(verts + T->verts[0]), 0x80FFFFFF);
				poly[it * 3 + 1].set(*(verts + T->verts[1]), 0x80FFFFFF);
				poly[it * 3 + 2].set(*(verts + T->verts[2]), 0x80FFFFFF);

				// Заполнение линий (каркас)
				line[it * 6 + 0].set(*(verts + T->verts[0]), 0xFFFFFFFF);
				line[it * 6 + 1].set(*(verts + T->verts[1]), 0xFFFFFFFF);
				line[it * 6 + 2].set(*(verts + T->verts[1]), 0xFFFFFFFF);
				line[it * 6 + 3].set(*(verts + T->verts[2]), 0xFFFFFFFF);
				line[it * 6 + 4].set(*(verts + T->verts[2]), 0xFFFFFFFF);
				line[it * 6 + 5].set(*(verts + T->verts[0]), 0xFFFFFFFF);
			}

			RenderBackendLegacy.set_transform_world(Fidentity);

			// draw solid
			Device.SetNearer(TRUE);
			RenderBackendLegacy.set_Shader(Device.m_SelectionShader);
			RenderBackendLegacy.dbg_Draw(D3DPT_TRIANGLELIST, &*poly.begin(), poly.size() / 3);
			Device.SetNearer(FALSE);

			// draw wire
			if (bDebug)
			{
				RenderImplementation.set_render_mode(IRender_interface::MODE_NEAR);
			}
			else
			{
				Device.SetNearer(TRUE);
			}

			RenderBackendLegacy.set_Shader(Device.m_SelectionShader);
			RenderBackendLegacy.dbg_Draw(D3DPT_LINELIST, &*line.begin(), line.size() / 2);

			if (bDebug)
			{
				RenderImplementation.set_render_mode(IRender_interface::MODE_NORMAL);
			}
			else
			{
				Device.SetNearer(FALSE);
			}
		}
	}
}

void CHOM::stats()
{
	if (m_pModel)
	{
		CGameFont& F = *Engine.Statistic->Font();
		F.OutNext(" **** HOM-occ ****");
		F.OutNext("  visible:  %2d", tris_in_frame_visible);
		F.OutNext("  frustum:  %2d", tris_in_frame);
		F.OutNext("    total:  %2d", m_pModel->get_tris_count());
	}
}
#endif