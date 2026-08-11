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
	hw_CurrentVB = 0;

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
	R_CHK(RenderBackend.GetDevice()->CreateVertexBuffer(dwVerts * vSize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &hw_VB, 0));
	R_CHK(RenderBackend.GetDevice()->CreateIndexBuffer(dwIndices * 2, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &hw_IB, 0));

	// Create Instance VB (DYNAMIC !!!)
	for (int i = 0; i < 3; ++i)
	{
		R_CHK(RenderBackend.GetDevice()->CreateVertexBuffer(hw_MaxInstances * sizeof(InstanceData),
															D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
															0, D3DPOOL_DEFAULT, &hw_InstanceVB[i], 0));
	}

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
	// Освобождаем буферы инстансов
	for (int i = 0; i < 3; ++i)
		_RELEASE(hw_InstanceVB[i]);
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
		RenderBackend.set_CullMode(CULL_DISABLE);
		RenderBackend.set_transform_world(Fidentity);
		RenderBackend.set_Geometry(hw_Geom);

		// 3. Запуск проходов
		ExecuteRenderPasses(ctx);

		RenderBackend.set_CullMode(CULL_BACKFACE);
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
	Engine.Statistic->RenderDUMP_DT_Count = 0;
	vis_per_wave& list = m_visibles[m_vis_render_id][visListType];

	// ------------------ ПРОХОД 1: сбор видимых моделей и подсчёт инстансов ------------------
	struct ModelBatch
	{
		CDetail* object;
		DetailBatch* batch;
		u32 vOffset;
		u32 iOffset;
		u32 instanceOffset;  // будет заполнен во время копирования
		u32 instanceCount;
		ref_selement shader;
	};
	xr_vector<ModelBatch> visibleModels;
	visibleModels.reserve(objects.size());

	u32 vOffset = 0;
	u32 iOffset = 0;
	u32 totalInstances = 0;

	for (u32 O = 0; O < objects.size(); O++)
	{
		CDetail& Object = *objects[O];
		DetailBatch& batch = list[O];

		if (Object.number_indices == 0 || batch.empty())
		{
			vOffset += Object.number_vertices;
			iOffset += Object.number_indices;
			continue;
		}

		// CPU куллинг целой модели (теневой проход)
		if (ctx.useAABB)
		{
			// используем агрегированный bbox батча
			if (batch.bbox.max.x < ctx.minX || batch.bbox.min.x > ctx.maxX ||
				batch.bbox.max.z < ctx.minZ || batch.bbox.min.z > ctx.maxZ)
			{
				vOffset += Object.number_vertices;
				iOffset += Object.number_indices;
				continue;
			}
			if (ctx.cullFrustum && !ctx.cullFrustum->testAABB_dirty(batch.bbox))
			{
				vOffset += Object.number_vertices;
				iOffset += Object.number_indices;
				continue;
			}
		}

		ModelBatch mb;
		mb.object = &Object;
		mb.batch = &batch;
		mb.vOffset = vOffset;
		mb.iOffset = iOffset;
		mb.instanceCount = (u32)batch.instances.size();
		mb.instanceOffset = totalInstances; // начало в будущем непрерывном массиве
		mb.shader = SelectShader(Object, ctx.mode, shaderType);
		visibleModels.push_back(mb);

		totalInstances += mb.instanceCount;
		vOffset += Object.number_vertices;
		iOffset += Object.number_indices;
	}

	if (visibleModels.empty()) return;

	// ------------------ ПРОХОД 2: заливка буфера инстансов ------------------
	// Если буфер не вмещает всё, потребуется несколько циклов (для простоты предположим, что вмещает)
	VERIFY(totalInstances <= hw_MaxInstances); // при необходимости добавить обработку

	// Один Lock (DISCARD) на всю волну
	hw_CurrentVB = (hw_CurrentVB + 1) % 3;
	IDirect3DVertexBuffer9* pCurrentVB = hw_InstanceVB[hw_CurrentVB];
	hw_BatchOffset = 0; // начинаем с начала
	void* ptr = nullptr;
	HRESULT hr = pCurrentVB->Lock(0, totalInstances * sizeof(InstanceData), &ptr, D3DLOCK_DISCARD);
	if (FAILED(hr)) return;

	InstanceData* pDest = (InstanceData*)ptr;

	// Копирование всех инстансов подряд с использованием non-temporal writes
	for (u32 i = 0; i < visibleModels.size(); i++)
	{
		ModelBatch& mb = visibleModels[i];
		const InstanceData* src = mb.batch->instances.data();
		u32 count = mb.instanceCount;
		mb.instanceOffset = u32(pDest - (InstanceData*)ptr); // актуальное смещение

		// SSE2 streaming copy (разворачиваем по 4 инстанса за цикл)
		const __m128i* pSrc = (const __m128i*)src;
		__m128i* pDst = (__m128i*)pDest;
		u32 simdCount = count * 4; // 4 регистра на инстанс (64 байта)

		for (u32 j = 0; j < simdCount; j += 4)
		{
			_mm_stream_si128(pDst + 0, _mm_loadu_si128(pSrc + 0));
			_mm_stream_si128(pDst + 1, _mm_loadu_si128(pSrc + 1));
			_mm_stream_si128(pDst + 2, _mm_loadu_si128(pSrc + 2));
			_mm_stream_si128(pDst + 3, _mm_loadu_si128(pSrc + 3));
			pSrc += 4;
			pDst += 4;
		}

		pDest += count;
	}

	_mm_sfence(); // гарантируем видимость записи GPU
	pCurrentVB->Unlock();

	// ------------------ ПРОХОД 3: отрисовка ------------------
	for (u32 i = 0; i < visibleModels.size(); i++)
	{
		ModelBatch& mb = visibleModels[i];
		RenderBackend.set_Element(mb.shader);

		// Установка stream source с нужным смещением
		u32 offsetInBytes = mb.instanceOffset * sizeof(InstanceData);
		RenderBackend.GetDevice()->SetStreamSource(1, pCurrentVB, offsetInBytes, sizeof(InstanceData));
		RenderBackend.GetDevice()->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | mb.instanceCount);
		RenderBackend.GetDevice()->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1);

		u32 primCount = mb.object->number_indices / 3;
		RenderBackend.Render(D3DPT_TRIANGLELIST, mb.vOffset, 0, mb.object->number_vertices,
			mb.iOffset, primCount);

		Engine.Statistic->RenderDUMP_DT_Count += mb.instanceCount;
		RenderBackend.stat.r.s_details.add(mb.instanceCount * mb.object->number_vertices);
	}

	// Сброс буфера не требуется, следующий проход начнёт с DISCARD
}
