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
void CDetailManager::hw_Load()
{
	// Настраиваем максимальное количество инстансов за один вызов
	// 32768 * 64 байта (размер InstanceData) = 2 MБ буфер. Это нормально.
	hw_MaxInstances = 32768;

	// Pre-process objects
	u32 dwVerts = 0;
	u32 dwIndices = 0;

	// Считаем общее количество вершин и индексов БЕЗ умножения на BatchSize
	// Нам нужна только ОДНА копия каждой модельки
	for (u32 o = 0; o < objects.size(); o++)
	{
		CDetail& D = *objects[o];
		dwVerts += D.number_vertices;
		dwIndices += D.number_indices;
	}

	u32 vSize = sizeof(vertHW);
	Msg("* [DETAILS] Instancing enabled. V(%d), P(%d)", dwVerts, dwIndices / 3);

	// Create VB/IB for Geometry
	// D3DPOOL_MANAGED лучше для статики в DX9, но DEFAULT тоже ок
	R_CHK(HW.pDevice->CreateVertexBuffer(dwVerts * vSize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &hw_VB, 0));
	R_CHK(HW.pDevice->CreateIndexBuffer(dwIndices * 2, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &hw_IB, 0));

	// === НОВОЕ: Создаем динамический буфер для инстансов ===
	// D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY обязателен для частого обновления
	R_CHK(HW.pDevice->CreateVertexBuffer(hw_MaxInstances * sizeof(InstanceData), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
										 0, D3DPOOL_DEFAULT, &hw_InstanceVB, 0));

	// Fill Geometry VB
	{
		// Lock buffer
		vertHW* pV;
		R_CHK(hw_VB->Lock(0, 0, (void**)&pV, 0));

		for (u32 o = 0; o < objects.size(); o++)
		{
			CDetail& D = *objects[o];

			// 1. Вычисляем высоту модели
			float fMinY = D.bv_bb.min.y;
			float fHeight = D.bv_bb.max.y - fMinY;

			// Защита от деления на ноль для плоских объектов
			if (fHeight < EPS_S)
				fHeight = EPS_S;

			D.bv_bb.min.y -= fMinY;
			D.bv_bb.max.y -= fMinY;
			D.bv_sphere.P.y -= fMinY;

			for (u32 v = 0; v < D.number_vertices; v++)
			{
				Fvector& vP = D.vertices[v].P;

				pV->x = vP.x;

				// 2. ИСПРАВЛЕНИЕ ПАРЕНИЯ:
				// Сдвигаем вершину так, чтобы самая нижняя точка модели всегда была на 0.0
				pV->y = vP.y - fMinY;

				pV->z = vP.z;

				pV->u = QC(D.vertices[v].u);
				pV->v = QC(D.vertices[v].v);

				// 3. ИСПРАВЛЕНИЕ ГРАДИЕНТА ВЕТРА (t):
				// Градиент должен быть от 0 (низ) до 1 (верх).
				// Раньше формула была vP.y / Height, что давало ошибку, если fMinY != 0.
				pV->t = QC((vP.y - fMinY) / fHeight);

				pV->mid = 0;
				pV++;
			}
		}
		R_CHK(hw_VB->Unlock());
	}

	// Fill Geometry IB
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

	// Declare geometry
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

void CalculateCullAABB(const Fmatrix& viewProj, float& minX, float& maxX, float& minZ, float& maxZ)
{
	// Инвертируем матрицу ViewProj, чтобы перевести куб NDC (-1..1) в мировые координаты
	Fmatrix inv;
	inv.invert(viewProj);

	// 8 углов NDC куба
	Fvector corners[8] = {{-1, -1, 0}, {-1, -1, 1}, {-1, 1, 0}, {-1, 1, 1},
						  {1, -1, 0},  {1, -1, 1},	{1, 1, 0},	{1, 1, 1}};

	minX = minZ = FLT_MAX;
	maxX = maxZ = -FLT_MAX;

	for (int i = 0; i < 8; ++i)
	{
		// Трансформируем точку из NDC в World Space
		Fvector& p = corners[i];
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

void CDetailManager::Render(DetailsRenderMode Mode, Fmatrix* pCullMatrix, const CFrustum* pExternalCull)
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
	Device.Statistic->RenderDUMP_DT_Render.Begin();
	{
		RenderBackend.set_CullMode(CULL_DISABLE);
		RenderBackend.set_xform_world(Fidentity);
		RenderBackend.set_Geometry(hw_Geom);

		// 3. Запуск проходов
		ExecuteRenderPasses(ctx);

		RenderBackend.set_CullMode(CULL_BACKFACE);
	}
	Device.Statistic->RenderDUMP_DT_Render.End();
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
	Device.Statistic->RenderDUMP_DT_Count = 0;
	vis_list& list = m_visibles[m_vis_render_id][visListType];

	u32 vOffset = 0;
	u32 iOffset = 0;

	for (u32 O = 0; O < objects.size(); O++)
	{
		CDetail& Object = *objects[O];
		u32 primCount = Object.number_indices / 3;
		xr_vector<DetailRenderVec*>& vis = list[O]; // Вектор векторов PrecalculatedData

		if (primCount == 0 || vis.empty())
		{
			vOffset += Object.number_vertices;
			iOffset += Object.number_indices;
			continue;
		}

		RenderBackend.set_Element(SelectShader(Object, ctx.mode, shaderType));
		RenderImplementation.apply_lmaterial();

		u32 currentInstanceCount = 0;
		InstanceData* pInstances = nullptr;
		bool bBufferLocked = false;

		// --- ЦИКЛ ПО СЛОТАМ (vis содержит указатели на вектора PrecalculatedData) ---
		for (auto slotIt = vis.begin(); slotIt != vis.end(); ++slotIt)
		{
			DetailRenderVec* items = *slotIt;
			if (items->empty())
				continue;

			// 1. === FAST CULLING (AABB) ===
			// Теперь мы читаем pos из линейного массива PrecalculatedData[0].pos.
			// Это намного быстрее, чем лезть в SlotItem->mRotY.c
			if (ctx.useAABB)
			{
				// Берем первый элемент (он представитель слота)
				const Fvector& pos = (*items)[0].pos;

				if (pos.x < ctx.minX || pos.x > ctx.maxX || pos.z < ctx.minZ || pos.z > ctx.maxZ)
				{
					continue;
				}

				if (!ctx.cullFrustum->testSphere_dirty(const_cast<Fvector&>(pos), 2.0f))
					continue;
			}

			// Лочим буфер лениво
			if (!bBufferLocked)
			{
				HRESULT hr = hw_InstanceVB->Lock(0, hw_MaxInstances * sizeof(InstanceData), (void**)&pInstances,
												 D3DLOCK_DISCARD);
				if (FAILED(hr))
					return;
				bBufferLocked = true;
			}

			// === 2. БЫСТРОЕ КОПИРОВАНИЕ (MEMCPY) ===
			// Вместо цикла с расчетами мы просто копируем данные пачками

			const PrecalculatedData* srcData = items->data();
			u32 itemsCount = (u32)items->size();
			u32 processed = 0;

			while (processed < itemsCount)
			{
				// Если буфер полон — сбрасываем
				if (currentInstanceCount >= hw_MaxInstances)
				{
					hw_InstanceVB->Unlock();
					FlushBatch(Object, currentInstanceCount, vOffset, iOffset);

					// Ре-лок
					currentInstanceCount = 0;
					HRESULT hr = hw_InstanceVB->Lock(0, hw_MaxInstances * sizeof(InstanceData), (void**)&pInstances,
													 D3DLOCK_DISCARD);
					if (FAILED(hr))
						return;
				}

				u32 available = hw_MaxInstances - currentInstanceCount;
				u32 toCopy = (itemsCount - processed) < available ? (itemsCount - processed) : available;

				// Копируем данные из precalculated cache прямо в вершинный буфер
				// Но у нас структура PrecalculatedData {pos, data}. Нам нужно копировать только data.
				// К сожалению, прямой memcpy всего массива не выйдет из-за поля 'pos',
				// но копирование в цикле без математики всё равно будет сверхбыстрым.

				// Оптимизированный цикл копирования
				for (u32 k = 0; k < toCopy; ++k)
				{
					pInstances[currentInstanceCount + k] = srcData[processed + k].data;
				}

				/*
				// АЛЬТЕРНАТИВА (если пожертвовать Culling-ом):
				// Если убрать 'pos' из PrecalculatedData и хранить 'pos' в отдельном параллельном векторе,
				// то здесь можно было бы сделать один memcpy:
				// memcpy(pInstances + currentInstanceCount, src_data_ptr, toCopy * sizeof(InstanceData));
				// Но пока цикл копирования структур — это отлично.
				*/

				currentInstanceCount += toCopy;
				processed += toCopy;
			}
		}

		if (bBufferLocked)
		{
			hw_InstanceVB->Unlock();
		}

		if (currentInstanceCount > 0)
		{
			FlushBatch(Object, currentInstanceCount, vOffset, iOffset);
		}

		if (bBufferLocked || currentInstanceCount > 0)
		{
			HW.pDevice->SetStreamSource(1, NULL, 0, 0);
			HW.pDevice->SetStreamSourceFreq(0, 1);
			HW.pDevice->SetStreamSourceFreq(1, 1);
		}

		vOffset += Object.number_vertices;
		iOffset += Object.number_indices;
	}
}

void CDetailManager::FlushBatch(CDetail& Object, u32 instanceCount, u32& vOffset, u32& iOffset)
{
	// Привязываем геометрию (на случай, если она отвалилась, хотя set_Geometry был выше)
	RenderBackend.set_Geometry(hw_Geom);

	// Stream 0: Геометрия (Indexed Data, частота делителя = instanceCount, но в DX9 это работает иначе для Instancing)
	// В DX9 Instancing: Stream 0 - геометрия (Vertex Data), Stream 1 - Instance Data.
	// D3DSTREAMSOURCE_INDEXEDDATA | instanceCount — говорит, сколько раз рисовать геометрию.

	HW.pDevice->SetStreamSource(1, hw_InstanceVB, 0, sizeof(InstanceData));
	HW.pDevice->SetStreamSourceFreq(0, (D3DSTREAMSOURCE_INDEXEDDATA | instanceCount));
	HW.pDevice->SetStreamSourceFreq(1, (D3DSTREAMSOURCE_INSTANCEDATA | 1));

	u32 primCount = Object.number_indices / 3;

	RenderBackend.Render(D3DPT_TRIANGLELIST, vOffset, 0, Object.number_vertices, iOffset, primCount);

	// Статистика
	Device.Statistic->RenderDUMP_DT_Count += instanceCount;
	RenderBackend.stat.r.s_details.add(instanceCount * Object.number_vertices);
}
