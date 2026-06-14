#include "stdafx.h"
#pragma hdrstop
#include "detailmanager.h"
#ifdef _EDITOR
#include "igame_persistent.h"
#include "environment.h"
#else
#include "..\xrEngine\igame_persistent.h"
#include "..\xrEngine\environment.h"
#endif
const int quant = 16384;
// === ИЗМЕНЕНИЕ 1: Декларация вершин ===
// Stream 0: Геометрия (Model)
// Stream 1: Данные инстансов (Matrix + Color)
static D3DVERTEXELEMENT9 dwDecl_Details[] = {
	// Stream 0 - Геометрия
	{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},  // pos
	{0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0}, // uv (packed)
	// Stream 1 - Инстансинг (пер-инстанс данные)
	// D3DDECLUSAGE_TEXCOORD 1-4 соответствуют InstanceData в шейдере
	{1, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},  // Mat0
	{1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2}, // Mat1
	{1, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3}, // Mat2
	{1, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4}, // Color

	D3DDECL_END()};
#pragma pack(push, 1)
struct vertHW
{
	float x, y, z;
	short u, v, t, mid; // mid больше не нужен по факту, но оставим для выравнивания или u/v packing
};
#pragma pack(pop)
short QC(float v)
{
	int t = iFloor(v * float(quant));
	clamp(t, -32768, 32767);
	return short(t & 0xffff);
}
#include "stdafx.h"
#pragma hdrstop
#include "detailmanager.h"

// ... (Оставляем начальные инклуды и структуры вершин без изменений) ...

void CDetailManager::hw_Load()
{
	// Увеличиваем буфер. 128k * 64 байта = 8 МБ.
	// Это гарантирует, что мы сможем отрисовать огромное количество травы
	// без частых сбросов (DISCARD), что уберет "фризы" CPU.
	hw_MaxInstances = 128 * 1024;
	hw_BatchOffset = 0; // Сброс оффсета в начало

	// Pre-process objects
	u32 dwVerts = 0;
	u32 dwIndices = 0;

	for (u32 o = 0; o < objects.size(); o++)
	{
		CDetail& D = *objects[o];
		dwVerts += D.number_vertices;
		dwIndices += D.number_indices;
	}

	u32 vSize = sizeof(vertHW);
	Msg("* [DETAILS] Instancing enabled. V(%d), P(%d), BufferSize(%d items)", dwVerts, dwIndices / 3, hw_MaxInstances);

	// Create VB/IB for Geometry
	R_CHK(HW.GetDevice()->CreateVertexBuffer(dwVerts * vSize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &hw_VB, 0));
	R_CHK(HW.GetDevice()->CreateIndexBuffer(dwIndices * 2, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &hw_IB, 0));

	// Create Instance VB (DYNAMIC !!!)
	R_CHK(HW.GetDevice()->CreateVertexBuffer(hw_MaxInstances * sizeof(InstanceData), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
										 0, D3DPOOL_DEFAULT, &hw_InstanceVB, 0));

	// Заполнение геометрии (без изменений)
	{
		vertHW* pV;
		R_CHK(hw_VB->Lock(0, 0, (void**)&pV, 0));
		for (u32 o = 0; o < objects.size(); o++)
		{
			CDetail& D = *objects[o];
			float fMinY = D.bv_bb.min.y;
			float fHeight = D.bv_bb.max.y - fMinY;
			if (fHeight < EPS_S)
				fHeight = EPS_S;

			D.bv_bb.min.y -= fMinY;
			D.bv_bb.max.y -= fMinY;
			D.bv_sphere.P.y -= fMinY;

			for (u32 v = 0; v < D.number_vertices; v++)
			{
				fvec3& vP = D.vertices[v].P;
				pV->x = vP.x;
				pV->y = vP.y - fMinY;
				pV->z = vP.z;
				pV->u = QC(D.vertices[v].u);
				pV->v = QC(D.vertices[v].v);
				pV->t = QC((vP.y - fMinY) / fHeight);
				pV->mid = 0;
				pV++;
			}
		}
		R_CHK(hw_VB->Unlock());
	}

	// Заполнение индексов (без изменений)
	{
		u16* pI;
		R_CHK(hw_IB->Lock(0, 0, (void**)(&pI), 0));
		for (u32 o = 0; o < objects.size(); o++)
		{
			CDetail& D = *objects[o];
			for (u32 i = 0; i < u32(D.number_indices); i++)
				*pI++ = u16(D.indices[i]);
		}
		R_CHK(hw_IB->Unlock());
	}

	hw_Geom.create(dwDecl_Details, hw_VB, hw_IB);
}

void CDetailManager::hw_Unload()
{
	hwc_array = nullptr;
	hwc_s_array = nullptr;

	hw_Geom.destroy();
	_RELEASE(hw_IB);
	_RELEASE(hw_VB);
	// Освобождаем буфер инстансов
	_RELEASE(hw_InstanceVB);
}

void CalculateCullAABB(const fmat4x4& viewProj, float& minX, float& maxX, float& minZ, float& maxZ)
{
	// Инвертируем матрицу ViewProj, чтобы перевести куб NDC (-1..1) в мировые координаты
	fmat4x4 inv;
	inv.invert(viewProj);

	// 8 углов NDC куба
	fvec3 corners[8] = {{-1, -1, 0}, {-1, -1, 1}, {-1, 1, 0}, {-1, 1, 1},
						  {1, -1, 0},  {1, -1, 1},	{1, 1, 0},	{1, 1, 1}};

	minX = minZ = FLT_MAX;
	maxX = maxZ = -FLT_MAX;

	for (int i = 0; i < 8; ++i)
	{
		// Трансформируем точку из NDC в World Space
		fvec3& p = corners[i];
		inv.transform(p);

		// Находим экстремумы
		if (p.x < minX)
			minX = p.x;
		if (p.x > maxX)
			maxX = p.x;
		if (p.z < minZ)
			minZ = p.z;
		if (p.z > maxZ)
			maxZ = p.z;
	}
}

void CDetailManager::Render(DetailsRenderMode Mode, fmat4x4* pCullMatrix, const CFrustum* pExternalCull)
{
	PROFILE_FUNCTION();

#ifndef _EDITOR
	if (0 == dtFS)
		return;
	if (!psDeviceFlags.is(rsDetails))
		return;
#endif

	// 1. Подготовка контекста
	SDetailRenderContext ctx;
	ctx.mode = Mode;
	ctx.cullMatrix = pCullMatrix;

	CFrustum localFrustum;

	// === ВЫБОР ФРУСТУМА ОТСЕЧЕНИЯ ===
	// Логика:
	// 1. Если передан pExternalCull (из r_sun.cpp) — используем его.
	//    Это точное пересечение конуса камеры и объема света. Это решает проблему 3ms.
	// 2. Если нет, но есть матрица (pCullMatrix) — строим фрустум по ней (старый метод).
	// 3. Если нет ни того, ни другого — куллинг по фрустуму не выполняется (рисуем всё, что в списке видимости).

	if (pExternalCull)
	{
		ctx.cullFrustum = pExternalCull;
	}
	else if (pCullMatrix)
	{
		localFrustum.CreateFromMatrix(*pCullMatrix, FRUSTUM_P_ALL);
		ctx.cullFrustum = &localFrustum;
	}

	// === FAST REJECT (AABB) ===
	// Даже если мы используем внешний фрустум, нам все равно полезно знать
	// границы проекции (текстуры) света, чтобы быстро отсечь объекты, выходящие за края шэдоу-мапы.
	if (pCullMatrix)
	{
		CalculateCullAABB(*pCullMatrix, ctx.minX, ctx.maxX, ctx.minZ, ctx.maxZ);
		ctx.useAABB = true;

		// Добавляем запас (padding) равный размеру слота (2м) + запас на наклон травы,
		// чтобы не обрезать траву на границе кадра
		const float cullingPadding = 2.5f;
		ctx.minX -= cullingPadding;
		ctx.minZ -= cullingPadding;
		ctx.maxX += cullingPadding;
		ctx.maxZ += cullingPadding;
	}

	// Кэшируем цвета окружения
	CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
	ctx.c_sun.set(desc->sun_color.x, desc->sun_color.y, desc->sun_color.z).mul(0.5f);
	ctx.c_ambient.set(desc->ambient.x, desc->ambient.y, desc->ambient.z);
	ctx.c_hemi.set(desc->hemi_color.x, desc->hemi_color.y, desc->hemi_color.z);

	// 2. Настройка глобального состояния рендера
	Engine.Statistic->RenderDUMP_DT_Render.Begin();
	{
		RenderBackendLegacy.set_CullMode(CULL_DISABLE);
		RenderBackendLegacy.set_transform_world(Fidentity);
		RenderBackendLegacy.set_Geometry(hw_Geom);

		// 3. Запуск проходов
		ExecuteRenderPasses(ctx);

		RenderBackendLegacy.set_CullMode(CULL_BACKFACE);
	}
	Engine.Statistic->RenderDUMP_DT_Render.End();
}

void CDetailManager::ExecuteRenderPasses(const SDetailRenderContext& ctx)
{
	// Pass 1: Wave 1 (Animated)
	{
		OPTICK_EVENT("Details: Wave 1");
		ProcessObjects(ctx, DVL_Wave1, DST_Animated);
	}

	// Pass 2: Wave 2 (Animated)
	{
		OPTICK_EVENT("Details: Wave 2");
		ProcessObjects(ctx, DVL_Wave2, DST_Animated);
	}

	// Pass 3: Static (Still)
	{
		OPTICK_EVENT("Details: Static");
		ProcessObjects(ctx, DVL_Static, DST_Static);
	}
}

ref_selement CDetailManager::SelectShader(CDetail& Object, DetailsRenderMode mode, EDetailShaderType shaderType)
{
	int id = 0;
	switch (mode)
	{
	case DetailsRenderMode::Default:
		id = (shaderType == DST_Animated) ? SE_DETAIL_NORMAL_ANIMATED : SE_DETAIL_NORMAL_STATIC;
		break;
	case DetailsRenderMode::DepthOnly:
		id = (shaderType == DST_Animated) ? SE_DETAIL_SHADOW_DEPTH_ANIMATED : SE_DETAIL_SHADOW_DEPTH_STATIC;
		break;
	}
	return Object.shader->E[id];
}

void CDetailManager::ProcessObjects(const SDetailRenderContext& ctx, EDetailVisibilityList visListType,
									EDetailShaderType shaderType)
{
	// Сбрасываем счетчик статистики
	Engine.Statistic->RenderDUMP_DT_Count = 0;

	vis_list& list = m_visibles[m_vis_render_id][visListType];
	u32 vOffset = 0;
	u32 iOffset = 0;

	for (u32 O = 0; O < objects.size(); O++)
	{
		CDetail& Object = *objects[O];
		u32 primCount = Object.number_indices / 3;
		xr_vector<DetailBatch*>& vis = list[O];

		if (primCount == 0 || vis.empty())
		{
			vOffset += Object.number_vertices;
			iOffset += Object.number_indices;
			continue;
		}

		// Установка шейдера
		RenderBackendLegacy.set_Element(SelectShader(Object, ctx.mode, shaderType));
		RenderImplementation.apply_lmaterial();

		// Переменные для батчинга внутри одного объекта
		u32 currentBatchCount = 0;
		InstanceData* pLockedData = nullptr;
		bool bLocked = false;

		// --- ЦИКЛ ПО БАТЧАМ ---
		for (auto slotIt = vis.begin(); slotIt != vis.end(); ++slotIt)
		{
			DetailBatch* batch = *slotIt;
			if (batch->empty())
				continue;

			// 1. AABB/Frustum Culling (CPU)
			// Читаем из отдельного вектора positions, чтобы не грузить в кэш лишние данные
			if (ctx.useAABB)
			{
				const fvec3& pos = batch->positions[0];
				if (pos.x < ctx.minX || pos.x > ctx.maxX || pos.z < ctx.minZ || pos.z > ctx.maxZ)
					continue;

				// Точный Frustum тест (если передан внешний фрустум)
				if (ctx.cullFrustum && !ctx.cullFrustum->testSphere_dirty(const_cast<fvec3&>(pos), 2.0f))
					continue;
			}

			u32 itemsInBatch = (u32)batch->instances.size();
			const InstanceData* srcData = batch->instances.data();
			u32 processed = 0;

			// Обработка данных (копирование частями, если батч огромен или мы у края буфера)
			while (processed < itemsInBatch)
			{
				// Логика блокировки буфера: Dynamic Buffer Pattern
				if (!bLocked)
				{
					// Проверяем, влезем ли мы в остаток буфера?
					// Если мы у края буфера — сбрасываем в начало.
					if (hw_BatchOffset >= hw_MaxInstances)
					{
						hw_BatchOffset = 0;
					}

					// Если пишем в начало - DISCARD (говорим драйверу "старое не нужно, дай новую память").
					// Если пишем дальше - NOOVERWRITE (говорим "мы пишем в конец, не трогая то, что GPU рисует из
					// начала").
					u32 dwLockFlags = (hw_BatchOffset == 0) ? D3DLOCK_DISCARD : D3DLOCK_NOOVERWRITE;

					// Лочим "хвост" буфера.
					void* ptr = nullptr;
					HRESULT hr = hw_InstanceVB->Lock(hw_BatchOffset * sizeof(InstanceData),
													 (hw_MaxInstances - hw_BatchOffset) * sizeof(InstanceData), &ptr,
													 dwLockFlags);

					if (FAILED(hr))
						return; // Критическая ошибка

					pLockedData = (InstanceData*)ptr;
					bLocked = true;
					currentBatchCount = 0; // Сколько мы записали за этот конкретный Lock
				}

				// Сколько места есть до конца физического буфера?
				u32 availableSpace = hw_MaxInstances - (hw_BatchOffset + currentBatchCount);
				u32 toCopy = (itemsInBatch - processed);

				if (toCopy > availableSpace)
				{
					// Если места не хватает даже для части данных — нужно сбросить буфер и начать сначала.

					// 1. Анлок и отрисовка того, что уже накопили (если есть)
					if (currentBatchCount > 0)
					{
						hw_InstanceVB->Unlock();
						// Рисуем накопленное
						FlushBatch(Object, currentBatchCount, vOffset, iOffset);
						// Сдвигаем глобальный оффсет, чтобы следующий Lock был корректным
						hw_BatchOffset += currentBatchCount;
					}
					else
					{
						// Если мы только что залочили и места нет (странная ситуация, но возможная), просто анлочим
						hw_InstanceVB->Unlock();
					}

					// 2. Сброс состояния для следующей итерации (начнет с D3DLOCK_DISCARD)
					hw_BatchOffset = 0;
					bLocked = false;
					pLockedData = nullptr;
					currentBatchCount = 0;

					// Переходим на следующую итерацию while, где сработает if (!bLocked) с флагом DISCARD
					continue;
				}

				// InstanceData = 64 байта (4 вектора по 16 байт). Идеально для SSE.
				// Используем _mm_stream_si128 для записи мимо кэша CPU.

				const __m128i* pSrcSimd = (const __m128i*)(srcData + processed);
				__m128i* pDestSimd = (__m128i*)(pLockedData + currentBatchCount);

				// Разворачиваем цикл для скорости
				u32 i = 0;
				// Обрабатываем по 1 инстансу (64 байта) за итерацию,
				// внутри 4 инструкции stream.
				for (; i < toCopy; ++i)
				{
					// Загружаем из обычной памяти (cached)
					__m128i r0 = _mm_loadu_si128(pSrcSimd + 0);
					__m128i r1 = _mm_loadu_si128(pSrcSimd + 1);
					__m128i r2 = _mm_loadu_si128(pSrcSimd + 2);
					__m128i r3 = _mm_loadu_si128(pSrcSimd + 3);

					// Стримим в видеопамять (write-combined, bypass cache)
					_mm_stream_si128(pDestSimd + 0, r0);
					_mm_stream_si128(pDestSimd + 1, r1);
					_mm_stream_si128(pDestSimd + 2, r2);
					_mm_stream_si128(pDestSimd + 3, r3);

					pSrcSimd += 4; // Сдвиг на 4 регистра (64 байта)
					pDestSimd += 4;
				}

				// Барьер памяти не обязателен на x86 для видимости GPU после Unlock,
				// но _mm_sfence() желателен перед Unlock, если мы используем WC память.
				_mm_sfence();

				currentBatchCount += toCopy;
				processed += toCopy;
			}
		}

		// Завершаем работу с объектом (рисуем остаток накопленного)
		if (bLocked)
		{
			hw_InstanceVB->Unlock();

			if (currentBatchCount > 0)
			{
				FlushBatch(Object, currentBatchCount, vOffset, iOffset);
				// Сдвигаем глобальный оффсет для следующего объекта
				hw_BatchOffset += currentBatchCount;
			}
		}

		vOffset += Object.number_vertices;
		iOffset += Object.number_indices;
	}
}

void CDetailManager::FlushBatch(CDetail& Object, u32 instanceCount, u32& vOffset, u32& iOffset)
{
	RenderBackendLegacy.set_Geometry(hw_Geom);

	// Устанавливаем стрим инстансинга.
	// Stream 1 (Instance Data) стартует с байтового смещения hw_BatchOffset.
	// hw_BatchOffset указывает на начало данных, которые мы только что скопировали для этого вызова.
	u32 offsetInBytes = hw_BatchOffset * sizeof(InstanceData);

	HW.GetDevice()->SetStreamSource(1, hw_InstanceVB, offsetInBytes, sizeof(InstanceData));

	// Настройка Hardware Instancing для DX9
	HW.GetDevice()->SetStreamSourceFreq(0, (D3DSTREAMSOURCE_INDEXEDDATA | instanceCount));
	HW.GetDevice()->SetStreamSourceFreq(1, (D3DSTREAMSOURCE_INSTANCEDATA | 1));

	u32 primCount = Object.number_indices / 3;
	RenderBackendLegacy.Render(D3DPT_TRIANGLELIST, vOffset, 0, Object.number_vertices, iOffset, primCount);

	// Обновляем статистику
	Engine.Statistic->RenderDUMP_DT_Count += instanceCount;
	RenderBackendLegacy.stat.r.s_details.add(instanceCount * Object.number_vertices);
}
