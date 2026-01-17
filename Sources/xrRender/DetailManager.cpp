// DetailManager.cpp: implementation of the CDetailManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "DetailManager.h"
#include "cl_intersect.h"

#ifdef _EDITOR
#include "ESceneClassList.h"
#include "Scene.h"
#include "SceneObject.h"
#include "igame_persistent.h"
#include "environment.h"
#else
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#endif

#include <xmmintrin.h>

const float dbgOffset = 0.f;
const int dbgItems = 128;

//--------------------------------------------------- Decompression
static int magic4x4[4][4] = {{0, 14, 3, 13}, {11, 5, 8, 6}, {12, 2, 15, 1}, {7, 9, 4, 10}};

void bwdithermap(int levels, int magic[16][16])
{
	/* Get size of each step */
	float N = 255.0f / (levels - 1);

	/*
	 * Expand 4x4 dither pattern to 16x16.  4x4 leaves obvious patterning,
	 * and doesn't give us full intensity range (only 17 sublevels).
	 *
	 * magicfact is (N - 1)/16 so that we get numbers in the matrix from 0 to
	 * N - 1: mod N gives numbers in 0 to N - 1, don't ever want all
	 * pixels incremented to the next level (this is reserved for the
	 * pixel value with mod N == 0 at the next level).
	 */

	float magicfact = (N - 1) / 16;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				for (int l = 0; l < 4; l++)
					magic[4 * k + i][4 * l + j] =
						(int)(0.5 + magic4x4[i][j] * magicfact + (magic4x4[k][l] / 16.) * magicfact);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDetailManager::CDetailManager()
{
	dtFS = 0;
	dtSlots = 0;
	hw_Geom = 0;
	hw_BatchSize = 0;
	hw_VB = 0;
	hw_IB = 0;
	m_vis_render_id = 0;
	m_vis_calc_id = 1;
	m_vCameraPos_calc = Device.vCameraPosition;
	m_mFullTransform_calc = Device.mFullTransform;
}

CDetailManager::~CDetailManager()
{
}

#ifndef _EDITOR
void CDetailManager::Load()
{
	// Open file stream
	if (!FS.exist("$level$", "level.details"))
	{
		dtFS = NULL;
		return;
	}
	string_path fn;
	FS.update_path(fn, "$level$", "level.details");
	dtFS = FS.r_open(fn);

	// Header
	dtFS->r_chunk_safe(0, &dtH, sizeof(dtH));
	R_ASSERT(dtH.version == DETAIL_VERSION);
	u32 m_count = dtH.object_count;

	// Models
	IReader* m_fs = dtFS->open_chunk(1);
	for (u32 m_id = 0; m_id < m_count; m_id++)
	{
		CDetail* dt = xr_new<CDetail>();
		IReader* S = m_fs->open_chunk(m_id);
		dt->Load(S);
		objects.push_back(dt);
		S->close();
	}
	m_fs->close();

	// Get pointer to database (slots)
	IReader* m_slots = dtFS->open_chunk(2);
	dtSlots = (DetailSlot*)m_slots->pointer();
	m_slots->close();

	// Initialize 'vis' and 'cache'
	// === ИЗМЕНЕНИЕ: Инициализируем 2 буфера * 3 типа волн ===
	for (u32 buf_id = 0; buf_id < 2; ++buf_id)
	{
		for (u32 wave_id = 0; wave_id < 3; ++wave_id)
		{
			// Ресайзим под количество уникальных объектов (моделей травы)
			m_visibles[buf_id][wave_id].resize(objects.size());
		}
	}
	// ========================================================

	cache_Initialize();

	// Make dither matrix
	bwdithermap(2, dither);
	hw_Load();
}
#endif

void CDetailManager::Unload()
{
	hw_Unload();

	for (DetailIt it = objects.begin(); it != objects.end(); it++)
	{
		(*it)->Unload();
		xr_delete(*it);
	}
	objects.clear();

	// === ИЗМЕНЕНИЕ: Очистка двойного буфера ===
	for (u32 buf_id = 0; buf_id < 2; ++buf_id)
	{
		for (u32 wave_id = 0; wave_id < 3; ++wave_id)
		{
			m_visibles[buf_id][wave_id].clear();
		}
	}
	// ==========================================

	FS.r_close(dtFS);
	dtFS = NULL; // Хорошая практика обнулять указатель
}

extern ECORE_API float r_ssaDISCARD;

void CDetailManager::UpdateVisibility()
{
	PROFILE_FUNCTION();

	Fvector EYE = m_vCameraPos_calc;

	CFrustum View;
	View.CreateFromMatrix(m_mFullTransform_calc, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

	float max_physical_radius = float(dm_size * dm_slot_size);
	float current_radius = ps_r_Detail_radius;
	if (current_radius > max_physical_radius)
		current_radius = max_physical_radius;
	if (current_radius < 20.f)
		current_radius = 20.f;

	float fade_limit = current_radius * current_radius;
	float fade_range_start = current_radius - 15.f;
	if (fade_range_start < 0)
		fade_range_start = 0;
	float fade_start = fade_range_start * fade_range_start;
	float fade_range = fade_limit - fade_start;
	float r_ssaCHEAP = 16 * r_ssaDISCARD;

	vis_list* working_vis = m_visibles[m_vis_calc_id];
	u32 current_frame = Engine.TimeManager.GetFrameCount();

	Device.Statistic->RenderDUMP_DT_VIS.Begin();

	for (int _mz = 0; _mz < dm_cache1_line; _mz++)
	{
		for (int _mx = 0; _mx < dm_cache1_line; _mx++)
		{
			CacheSlot1& MS = cache_level1[_mz][_mx];
			if (MS.empty)
				continue;

			u32 mask = 0xff;
			u32 res = View.testSAABB(MS.vis.sphere.P, MS.vis.sphere.R, MS.vis.box.data(), mask);
			if (fcvNone == res)
				continue;

#ifndef _EDITOR
			if (!RenderImplementation.HOM.visible(MS.vis))
				continue;
#endif

			for (int _i = 0; _i < dm_cache1_count * dm_cache1_count; _i++)
			{
				Slot* PS = *MS.slots[_i];
				Slot& S = *PS;
				if (S.empty)
					continue;

				if (fcvPartial == res)
				{
					u32 _mask = mask;
					u32 _res = View.testSAABB(S.vis.sphere.P, S.vis.sphere.R, S.vis.box.data(), _mask);
					if (fcvNone == _res)
						continue;
				}

#ifndef _EDITOR
				if (!RenderImplementation.HOM.visible(S.vis))
					continue;
#endif

				float dist_sq = EYE.distance_to_sqr(S.vis.sphere.P);
				if (dist_sq > fade_limit)
					continue;

				// === 1. ОБНОВЛЕНИЕ ПАРАМЕТРОВ (редко) ===
				if (current_frame > S.frame)
				{
					float alpha = (dist_sq < fade_start) ? 0.f : (dist_sq - fade_start) / fade_range;
					float alpha_i = 1.f - alpha;
					float dist_sq_rcp = 1.f / dist_sq;

					S.frame = current_frame + Random.randI(15, 30);

					if (alpha_i > 0.01f)
					{
						for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
						{
							SlotPart& sp = S.G[sp_id];
							if (sp.id == DetailSlot::ID_Empty)
								continue;

							float R = objects[sp.id]->bv_sphere.R;
							float Rq_drcp = R * R * dist_sq_rcp;

							for (auto item : sp.items)
							{
								item->scale_calculated = item->scale * alpha_i;
								float ssa = item->scale_calculated * item->scale_calculated * Rq_drcp;
								if (ssa < r_ssaDISCARD)
									item->vis_ID = 0xff;
								else if (ssa > r_ssaCHEAP)
									item->vis_ID = item->vis_ID;
								else
									item->vis_ID = 0;
							}
						}
					}
				}

				// === 2. ЗАПОЛНЕНИЕ ДАННЫХ ДЛЯ GPU (всегда, В ПАРАЛЛЕЛЬНОМ ПОТОКЕ) ===
				for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
				{
					SlotPart& sp = S.G[sp_id];
					if (sp.id == DetailSlot::ID_Empty)
						continue;

					auto& buffer_static = sp.r_items[m_vis_calc_id][0];
					auto& buffer_wave1 = sp.r_items[m_vis_calc_id][1];
					auto& buffer_wave2 = sp.r_items[m_vis_calc_id][2];

					buffer_static.clear_not_free();
					buffer_wave1.clear_not_free();
					buffer_wave2.clear_not_free();

					for (auto pItem : sp.items)
					{
						if (pItem->scale_calculated > EPS && pItem->vis_ID != 0xff)
						{
							u32 v_id = (pItem->vis_ID > 2) ? 0 : pItem->vis_ID;

							// Выбираем нужный батч
							DetailBatch& destBatch = sp.r_items[m_vis_calc_id][v_id];

							// 1. Сохраняем позицию для CPU (быстрый куллинг)
							destBatch.positions.push_back(pItem->mRotY.c);

							// 2. Рассчитываем матрицы для GPU с использованием SSE
							// Мы резервируем место и получаем указатель на новую структуру, 
							// чтобы писать прямо в память вектора без лишних копий.
							destBatch.instances.resize(destBatch.instances.size() + 1);
							InstanceData& inst = destBatch.instances.back();

							float scale = pItem->scale_calculated;
							Fmatrix& M = pItem->mRotY;

							// === SSE OPTIMIZATION START ===
							// Нам нужно преобразовать матрицу X-Ray (Row-Major) в формат шейдера
							// и умножить компоненты вращения (3x3) на scale, не трогая позицию (w).
							// Текущая логика:
							// Mat0 = (M._11*s, M._21*s, M._31*s, M._41*1)
							// Mat1 = (M._12*s, M._22*s, M._32*s, M._42*1)
							// Mat2 = (M._13*s, M._23*s, M._33*s, M._43*1)

							// 1. Подготовка вектора масштаба: {scale, scale, scale, 1.0f}
							// _mm_set_ps принимает аргументы в порядке (e3, e2, e1, e0), где e0 - младший адрес.
							__m128 S = _mm_set_ps(1.0f, scale, scale, scale);

							// 2. Загрузка строк матрицы
							__m128 R0 = _mm_loadu_ps(&M._11); // Row 1
							__m128 R1 = _mm_loadu_ps(&M._21); // Row 2
							__m128 R2 = _mm_loadu_ps(&M._31); // Row 3
							__m128 R3 = _mm_loadu_ps(&M._41); // Row 4 (Pos)

							// 3. Транспонирование 4x4 (превращаем строки в столбцы)
							// После этого:
							// R0 содержит: _11, _21, _31, _41
							// R1 содержит: _12, _22, _32, _42
							// R2 содержит: _13, _23, _33, _43
							_MM_TRANSPOSE4_PS(R0, R1, R2, R3);

							// 4. Умножение на масштаб
							R0 = _mm_mul_ps(R0, S); // Mat0
							R1 = _mm_mul_ps(R1, S); // Mat1
							R2 = _mm_mul_ps(R2, S); // Mat2
							
							// 5. Выгрузка результата прямо в структуру
							_mm_storeu_ps((float*)&inst.Mat0, R0);
							_mm_storeu_ps((float*)&inst.Mat1, R1);
							_mm_storeu_ps((float*)&inst.Mat2, R2);
							// === SSE OPTIMIZATION END ===

							// Цвет (без изменений, так как это просто установка)
							float h = pItem->c_hemi;
							float s = pItem->c_sun;
							inst.Color.set(s, s, s, h);
						}
					}

					// Добавляем указатели в список видимости, если батч не пустой
					if (!buffer_static.empty())
						working_vis[0][sp.id].push_back(&buffer_static);
					if (!buffer_wave1.empty())
						working_vis[1][sp.id].push_back(&buffer_wave1);
					if (!buffer_wave2.empty())
						working_vis[2][sp.id].push_back(&buffer_wave2);
				}
			}
		}
	}
	Device.Statistic->RenderDUMP_DT_VIS.End();
}

void CDetailManager::PrepareToCalc()
{
	PROFILE_FUNCTION();

	// 1. Своп индексов.
	// То, что рисовали (render_id), теперь становится буфером для нового расчета.
	std::swap(m_vis_render_id, m_vis_calc_id);

	// 2. Очистка буфера, в который будем писать (бывший render_id).
	// Буфер m_vis_render_id НЕ трогаем, он сейчас пойдет на отрисовку.
	for (int i = 0; i < 3; ++i)
	{
		for (u32 j = 0; j < m_visibles[m_vis_calc_id][i].size(); ++j)
		{
			// Быстрая очистка векторов (оставляет capacity)
			m_visibles[m_vis_calc_id][i][j].clear_not_free();
		}
		// Очищаем сам список списков, но не удаляем вектора внутри (они переиспользуются)
		// На самом деле, m_visibles - это вектор векторов указателей.
		// Нам нужно очистить только верхний уровень, так как мы будем push_back-ать заново.

		// ВАЖНО: В реализации X-Ray m_visibles[i] это vector<vector<SlotItemVec*>>.
		// Нам нужно очистить списки для каждого слота.
		// Но структура данных X-Ray здесь хитрая: m_visibles[0] имеет размер objects.size().
		// Внутри каждого объекта лежит список видимых слотов.

		for (auto& vec : m_visibles[m_vis_calc_id][i])
		{
			vec.clear_not_free();
		}
	}

	// 3. Захват состояния камеры для потока
	m_vCameraPos_calc = Device.vCameraPosition;
	m_mFullTransform_calc = Device.mFullTransform;
}

void __stdcall CDetailManager::MT_CALC()
{
	PROFILE_FUNCTION();

#ifndef _EDITOR
	if (0 == RenderImplementation.Details)
		return;
	if (0 == dtFS)
		return;
	if (!psDeviceFlags.is(rsDetails))
		return;
#endif

	MT.Enter();

	// Используем ЗАХВАЧЕННУЮ позицию камеры
	Fvector EYE = m_vCameraPos_calc;

	int s_x = iFloor(EYE.x / dm_slot_size + .5f);
	int s_z = iFloor(EYE.z / dm_slot_size + .5f);

	Device.Statistic->RenderDUMP_DT_Cache.Begin();
	cache_Update(s_x, s_z, EYE, dm_max_decompress);
	Device.Statistic->RenderDUMP_DT_Cache.End();

	UpdateVisibility();

	MT.Leave();
}

void CDetailManager::cache_Initialize()
{
	PROFILE_FUNCTION();

	// Centroid
	cache_cx = 0;
	cache_cz = 0;

	// Initialize cache-grid
	Slot* slt = cache_pool;
	for (u32 i = 0; i < dm_cache_line; i++)
		for (u32 j = 0; j < dm_cache_line; j++, slt++)
		{
			cache[i][j] = slt;
			cache_Task(j, i, slt);
		}
	VERIFY(cache_Validate());

	for (int _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
	{
		for (int _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
		{
			CacheSlot1& MS = cache_level1[_mz1][_mx1];
			for (int _z = 0; _z < dm_cache1_count; _z++)
				for (int _x = 0; _x < dm_cache1_count; _x++)
					MS.slots[_z * dm_cache1_count + _x] =
						&cache[_mz1 * dm_cache1_count + _z][_mx1 * dm_cache1_count + _x];
		}
	}
}

CDetailManager::Slot* CDetailManager::cache_Query(int r_x, int r_z)
{
	int gx = w2cg_X(r_x + cache_cx);
	VERIFY(gx >= 0 && gx < dm_cache_line);
	int gz = w2cg_Z(r_z + cache_cz);
	VERIFY(gz >= 0 && gz < dm_cache_line);
	return cache[gz][gx];
}

void CDetailManager::cache_Task(int gx, int gz, Slot* D)
{
	int sx = cg2w_X(gx);
	int sz = cg2w_Z(gz);
	DetailSlot& DS = QueryDB(sx, sz);

	D->empty = (DS.id0 == DetailSlot::ID_Empty) && (DS.id1 == DetailSlot::ID_Empty) &&
			   (DS.id2 == DetailSlot::ID_Empty) && (DS.id3 == DetailSlot::ID_Empty);

	// Unpacking
	u32 old_type = D->type;
	D->type = stPending;
	D->sx = sx;
	D->sz = sz;

	D->vis.box.min.set(sx * dm_slot_size, DS.r_ybase(), sz * dm_slot_size);
	D->vis.box.max.set(D->vis.box.min.x + dm_slot_size, DS.r_ybase() + DS.r_yheight(), D->vis.box.min.z + dm_slot_size);
	D->vis.box.grow(EPS_L);

	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		D->G[i].id = DS.r_id(i);
		for (u32 clr = 0; clr < D->G[i].items.size(); clr++)
			poolSI.destroy(D->G[i].items[clr]);
		D->G[i].items.clear();
	}

	if (old_type != stPending)
	{
		VERIFY(stPending == D->type);
		cache_task.push_back(D);
	}
}

BOOL CDetailManager::cache_Validate()
{
	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			int w_x = cg2w_X(x);
			int w_z = cg2w_Z(z);
			Slot* D = cache[z][x];

			if (D->sx != w_x)
				return FALSE;
			if (D->sz != w_z)
				return FALSE;
		}
	}
	return TRUE;
}

void CDetailManager::cache_Update(int v_x, int v_z, Fvector& view, int limit)
{
	bool bNeedMegaUpdate = (cache_cx != v_x) || (cache_cz != v_z);

	// Сдвиг кеша (оставляем код сдвига)
	while (cache_cx != v_x)
	{
		if (v_x > cache_cx)
		{
			cache_cx++;
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* S = cache[z][0];
				for (int x = 1; x < dm_cache_line; x++)
					cache[z][x - 1] = cache[z][x];
				cache[z][dm_cache_line - 1] = S;
				cache_Task(dm_cache_line - 1, z, S);
			}
		}
		else
		{
			cache_cx--;
			for (int z = 0; z < dm_cache_line; z++)
			{
				Slot* S = cache[z][dm_cache_line - 1];
				for (int x = dm_cache_line - 1; x > 0; x--)
					cache[z][x] = cache[z][x - 1];
				cache[z][0] = S;
				cache_Task(0, z, S);
			}
		}
	}
	while (cache_cz != v_z)
	{
		if (v_z > cache_cz)
		{
			cache_cz++;
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* S = cache[dm_cache_line - 1][x];
				for (int z = dm_cache_line - 1; z > 0; z--)
					cache[z][x] = cache[z - 1][x];
				cache[0][x] = S;
				cache_Task(x, 0, S);
			}
		}
		else
		{
			cache_cz--;
			for (int x = 0; x < dm_cache_line; x++)
			{
				Slot* S = cache[0][x];
				for (int z = 1; z < dm_cache_line; z++)
					cache[z - 1][x] = cache[z][x];
				cache[dm_cache_line - 1][x] = S;
				cache_Task(x, dm_cache_line - 1, S);
			}
		}
	}

	bool bTasksProcessed = !cache_task.empty();

	// PPL Распаковка
	if (bTasksProcessed)
	{
		concurrency::parallel_for(size_t(0), cache_task.size(), [&](size_t i) {
			xrXRC thread_local_xrc;
			cache_Decompress(cache_task[i], thread_local_xrc);
		});
		cache_task.clear();
	}

	// Обновление глобального AABB (MegaUpdate)
	// ВАЖНО: обновляем если сдвинулись ИЛИ если распаковали новые слоты
	if (bNeedMegaUpdate || bTasksProcessed)
	{
		for (int _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
		{
			for (int _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
			{
				CacheSlot1& MS = cache_level1[_mz1][_mx1];
				MS.empty = TRUE;
				MS.vis.clear();
				for (int _i = 0; _i < dm_cache1_count * dm_cache1_count; _i++)
				{
					Slot* PS = *MS.slots[_i];
					Slot& S = *PS;
					if (!S.empty)
					{
						MS.empty = FALSE;
						MS.vis.box.merge(S.vis.box);
					}
				}
				if (!MS.empty)
					MS.vis.box.getsphere(MS.vis.sphere.P, MS.vis.sphere.R);
			}
		}
	}
}

DetailSlot& CDetailManager::QueryDB(int sx, int sz)
{
	int db_x = sx + dtH.offs_x;
	int db_z = sz + dtH.offs_z;
	if ((db_x >= 0) && (db_x < int(dtH.size_x)) && (db_z >= 0) && (db_z < int(dtH.size_z)))
	{
		u32 linear_id = db_z * dtH.size_x + db_x;
		return dtSlots[linear_id];
	}
	else
	{
		// Empty slot
		DS_empty.w_id(0, DetailSlot::ID_Empty);
		DS_empty.w_id(1, DetailSlot::ID_Empty);
		DS_empty.w_id(2, DetailSlot::ID_Empty);
		DS_empty.w_id(3, DetailSlot::ID_Empty);
		return DS_empty;
	}
}

void CDetailManager::InvalidateCache()
{
	MT.Enter();

	cache_task.clear();

	for (int z = 0; z < dm_cache_line; z++)
	{
		for (int x = 0; x < dm_cache_line; x++)
		{
			Slot* S = cache[z][x];
			if (S->type != stPending && !S->empty)
			{
				for (u32 i = 0; i < dm_obj_in_slot; i++)
				{
					// Очищаем основной пул айтемов
					for (u32 clr = 0; clr < S->G[i].items.size(); clr++)
					{
						pool_lock.Enter();
						poolSI.destroy(S->G[i].items[clr]);
						pool_lock.Leave();
					}
					S->G[i].items.clear();

					// === FIX: Очищаем ОБА буфера рендера ===
					// Чтобы не осталось висячих ссылок при смене уровня или настроек
					for (int buf = 0; buf < 2; ++buf)
					{
						S->G[i].r_items[buf][0].clear_not_free();
						S->G[i].r_items[buf][1].clear_not_free();
						S->G[i].r_items[buf][2].clear_not_free();
					}
				}
				S->vis.clear();
			}

			// Перезапускаем слот в очередь задач
			int gx = w2cg_X(S->sx);
			int gz = w2cg_Z(S->sz);
			if (gx >= 0 && gx < dm_cache_line && gz >= 0 && gz < dm_cache_line)
			{
				cache_Task(gx, gz, S);
			}
		}
	}

	// Сброс видимости 1 уровня
	for (int _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
	{
		for (int _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
		{
			cache_level1[_mz1][_mx1].empty = TRUE;
			cache_level1[_mz1][_mx1].vis.clear();
		}
	}

	MT.Leave();
}

//--------------------------------------------------- Decompression
IC float Interpolate(float* base, u32 x, u32 y, u32 size)
{
	float f = float(size);
	float fx = float(x) / f;
	float ifx = 1.f - fx;
	float fy = float(y) / f;
	float ify = 1.f - fy;

	float c01 = base[0] * ifx + base[1] * fx;
	float c23 = base[2] * ifx + base[3] * fx;
	float c02 = base[0] * ify + base[2] * fy;
	float c13 = base[1] * ify + base[3] * fy;

	float cx = ify * c01 + fy * c23;
	float cy = ifx * c02 + fx * c13;
	return (cx + cy) / 2;
}

IC bool InterpolateAndDither(float* alpha255, u32 x, u32 y, u32 sx, u32 sy, u32 size, int dither[16][16])
{
	clamp(x, (u32)0, size - 1);
	clamp(y, (u32)0, size - 1);
	int c = iFloor(Interpolate(alpha255, x, y, size) + .5f);
	clamp(c, 0, 255);

	u32 row = (y + sy) % 16;
	u32 col = (x + sx) % 16;
	return c > dither[col][row];
}

// Оптимизированная интерполяция без лишних делений
IC float InterpolateOptimized(float c0, float c1, float ratio)
{
	return c0 * (1.f - ratio) + c1 * ratio;
}

// Структура для кеширования треугольников
struct TriCache
{
	Fvector v0, v1, v2;
	float min_x, max_x, min_z, max_z;
};

void CDetailManager::cache_Decompress(Slot* S, xrXRC& local_xrc)
{
	PROFILE_FUNCTION();

	VERIFY(S);
	Slot& D = *S;
	D.type = stReady;
	if (D.empty)
		return;

	DetailSlot& DS = QueryDB(D.sx, D.sz);

	Fvector bC, bD;
	D.vis.box.get_CD(bC, bD);

#ifdef _EDITOR
	// Editor code omitted for brevity
#else
	local_xrc.box_options(CDB::OPT_FULL_TEST);
	local_xrc.box_query(g_pGameLevel->ObjectSpace.GetStaticModel(), bC, bD);
	u32 triCount = local_xrc.r_count();
	CDB::TRI* tris = g_pGameLevel->ObjectSpace.GetStaticTris();
	Fvector* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();
#endif

	if (0 == triCount)
		return;

	float alpha255[dm_obj_in_slot][4];
	const float k_alpha = 255.f / 15.f;
	for (int i = 0; i < dm_obj_in_slot; i++)
	{
		alpha255[i][0] = k_alpha * float(DS.palette[i].a0);
		alpha255[i][1] = k_alpha * float(DS.palette[i].a1);
		alpha255[i][2] = k_alpha * float(DS.palette[i].a2);
		alpha255[i][3] = k_alpha * float(DS.palette[i].a3);
	}

	// === ИСПОЛЬЗУЕМ НАСТРОЙКИ ===
	float density = ps_r_Detail_density;
	// ============================

	float jitter = density / 1.7f;
	u32 d_size = iCeil(dm_slot_size / density);
	float inv_d_size = 1.0f / float(d_size);

	svector<int, dm_obj_in_slot> selected;
	u32 p_rnd = D.sx * D.sz;
	CRandom r_selection(0x12071980 ^ p_rnd);
	CRandom r_Jitter(0x12071980 ^ p_rnd);
	CRandom r_yaw(0x12071980 ^ p_rnd);
	CRandom r_scale(0x12071980 ^ p_rnd);

	Fbox Bounds;
	Bounds.invalidate();

	// Кеш треугольников
	const u32 MAX_TRIS_CACHE = 64;
	TriCache t_cache[MAX_TRIS_CACHE];
	u32 cached_tris_count = _min(triCount, MAX_TRIS_CACHE);

#ifndef _EDITOR
	for (u32 t = 0; t < cached_tris_count; ++t)
	{
		CDB::TRI& T = tris[local_xrc.r_begin()[t].id];
		t_cache[t].v0 = verts[T.verts[0]];
		t_cache[t].v1 = verts[T.verts[1]];
		t_cache[t].v2 = verts[T.verts[2]];
		t_cache[t].min_x = _min(t_cache[t].v0.x, _min(t_cache[t].v1.x, t_cache[t].v2.x));
		t_cache[t].max_x = _max(t_cache[t].v0.x, _max(t_cache[t].v1.x, t_cache[t].v2.x));
		t_cache[t].min_z = _min(t_cache[t].v0.z, _min(t_cache[t].v1.z, t_cache[t].v2.z));
		t_cache[t].max_z = _max(t_cache[t].v0.z, _max(t_cache[t].v1.z, t_cache[t].v2.z));
	}
#endif

	for (u32 z = 0; z <= d_size; z++)
	{
		float fz = float(z) * inv_d_size;
		float rz_base = fz * dm_slot_size + D.vis.box.min.z;
		float ify = 1.f - fz;
		float c_y_0[4], c_y_1[4];
		for (int i = 0; i < 4; ++i)
		{
			c_y_0[i] = alpha255[i][0] * ify + alpha255[i][2] * fz;
			c_y_1[i] = alpha255[i][1] * ify + alpha255[i][3] * fz;
		}

		for (u32 x = 0; x <= d_size; x++)
		{
			float fx = float(x) * inv_d_size;
			float ifx = 1.f - fx;
			u32 shift_x = r_Jitter.randI(16);
			u32 shift_z = r_Jitter.randI(16);
			u32 d_row = (z + shift_z) % 16;
			u32 d_col = (x + shift_x) % 16;
			int dither_val = dither[d_col][d_row];

			selected.clear();
			if (DS.id0 != DetailSlot::ID_Empty && (int(ifx * c_y_0[0] + fx * c_y_1[0] + 0.5f) > dither_val))
				selected.push_back(0);
			if (DS.id1 != DetailSlot::ID_Empty && (int(ifx * c_y_0[1] + fx * c_y_1[1] + 0.5f) > dither_val))
				selected.push_back(1);
			if (DS.id2 != DetailSlot::ID_Empty && (int(ifx * c_y_0[2] + fx * c_y_1[2] + 0.5f) > dither_val))
				selected.push_back(2);
			if (DS.id3 != DetailSlot::ID_Empty && (int(ifx * c_y_0[3] + fx * c_y_1[3] + 0.5f) > dither_val))
				selected.push_back(3);

			if (selected.empty())
				continue;

			u32 index = (selected.size() == 1) ? selected[0] : selected[r_selection.randI(selected.size())];
			CDetail* Dobj = objects[DS.r_id(index)];

			SlotItem* ItemP;
			{
				pool_lock.Enter();
				ItemP = poolSI.create();
				pool_lock.Leave();
			}
			SlotItem& Item = *ItemP;

			float rx = fx * dm_slot_size + D.vis.box.min.x;
			Fvector Item_P;
			Item_P.set(rx + r_Jitter.randFs(jitter), D.vis.box.max.y, rz_base + r_Jitter.randFs(jitter));

			float y = D.vis.box.min.y - 5;
			Fvector dir;
			dir.set(0, -1, 0);
			float r_u, r_v, r_range;

			for (u32 tid = 0; tid < cached_tris_count; tid++)
			{
				TriCache& TC = t_cache[tid];
				if (Item_P.x < TC.min_x || Item_P.x > TC.max_x || Item_P.z < TC.min_z || Item_P.z > TC.max_z)
					continue;
				Fvector Tv[3] = {TC.v0, TC.v1, TC.v2};
				if (CDB::TestRayTri(Item_P, dir, Tv, r_u, r_v, r_range, TRUE))
				{
					if (r_range >= 0)
					{
						float y_test = Item_P.y - r_range;
						if (y_test > y)
							y = y_test;
					}
				}
			}

			if (y < D.vis.box.min.y)
				continue;

			// === ИСПОЛЬЗУЕМ НАСТРОЙКИ ===
			Item_P.y = y + ps_r_Detail_height; // Высота

			float base_scale = r_scale.randF(Dobj->m_fMinScale * 0.5f, Dobj->m_fMaxScale * 0.9f);
			Item.scale = base_scale * ps_r_Detail_scale; // Масштаб
			// ============================

			Fmatrix mScale, mXform;
			Fbox ItemBB;
			Item.mRotY.rotateY(r_yaw.randF(0, PI_MUL_2));
			Item.mRotY.translate_over(Item_P);
			mScale.scale(Item.scale, Item.scale, Item.scale);
			mXform.mul_43(Item.mRotY, mScale);
			ItemBB.xform(Dobj->bv_bb, mXform);
			Bounds.merge(ItemBB);

			Item.c_hemi = DS.r_qclr(DS.c_hemi, 15);
			Item.c_sun = DS.r_qclr(DS.c_dir, 15);

			if (Dobj->m_Flags.is(DO_NO_WAVING))
				Item.vis_ID = 0;
			else
				Item.vis_ID = (::Random.randI(0, 3) == 0) ? 2 : 1;

			D.G[index].items.push_back(ItemP);
		}
	}

	D.vis.clear();
	D.vis.box.set(Bounds);
	D.vis.box.getsphere(D.vis.sphere.P, D.vis.sphere.R);
}

void CDetailManager::ClearVisible()
{
	// Метод оставлен пустым, так как очистка видимости теперь происходит
	// в начале кадра внутри PrepareToCalc для swap-buffer логики.
}
