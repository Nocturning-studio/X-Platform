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
void CDetailManager::hw_Render(DetailsRenderMode Mode)
{
	// Setup geometry
	// Внимание: hw_Geom.create привязал hw_VB как Stream 0.
	// Stream 1 (инстансы) мы привяжем вручную.
	RenderBackend.set_Geometry(hw_Geom);

	// Рендерим статику и волны (разные LOD-ы)
	{
		OPTICK_EVENT("Wave 1");
		hw_Render_dump(1, 0, Mode); // Wave 1
	}
	{
		OPTICK_EVENT("Wave 2");
		hw_Render_dump(2, 0, Mode); // Wave 1
	}
	{
		OPTICK_EVENT("Still (Static)");
		hw_Render_dump(0, 1, Mode); // Still (Static)
	}
}
void CDetailManager::hw_Render_dump(u32 var_id, u32 lod_id, DetailsRenderMode Mode)
{
	Device.Statistic->RenderDUMP_DT_Count = 0;

	u32 vOffset = 0;
	u32 iOffset = 0;

	// === ГЛАВНОЕ ИЗМЕНЕНИЕ ===
	// Читаем из буфера РЕНДЕРА, который гарантированно не меняется сейчас
	vis_list& list = m_visibles[m_vis_render_id][var_id];
	// =========================

	CEnvDescriptor* desc = g_pGamePersistent->Environment().CurrentEnv;
	Fvector c_sun, c_ambient, c_hemi;
	c_sun.set(desc->sun_color.x, desc->sun_color.y, desc->sun_color.z);
	c_sun.mul(.5f);
	c_ambient.set(desc->ambient.x, desc->ambient.y, desc->ambient.z);
	c_hemi.set(desc->hemi_color.x, desc->hemi_color.y, desc->hemi_color.z);

	RenderBackend.set_Geometry(hw_Geom);

	for (u32 O = 0; O < objects.size(); O++)
	{
		CDetail& Object = *objects[O];
		u32 primCount = Object.number_indices / 3;
		if (primCount == 0)
		{
			vOffset += Object.number_vertices;
			iOffset += Object.number_indices;
			continue;
		}

		xr_vector<SlotItemVec*>& vis = list[O];

		if (!vis.empty())
		{
			int id = 0;
			
			switch (Mode)
			{
			case DetailsRenderMode::Default:
				id = (lod_id == 0) ? SE_DETAIL_NORMAL_ANIMATED : SE_DETAIL_NORMAL_STATIC;
				break;
			case DetailsRenderMode::DepthOnly:
				id = (lod_id == 0) ? SE_DETAIL_SHADOW_DEPTH_ANIMATED : SE_DETAIL_SHADOW_DEPTH_STATIC;
				break;
			}

			RenderBackend.set_Element(Object.shader->E[id]);
			RenderImplementation.apply_lmaterial();

			u32 currentInstanceCount = 0;
			InstanceData* pInstances = nullptr;

			HRESULT hr =
				hw_InstanceVB->Lock(0, hw_MaxInstances * sizeof(InstanceData), (void**)&pInstances, D3DLOCK_DISCARD);
			if (FAILED(hr))
				return;

			auto _vI = vis.begin();
			auto _vE = vis.end();

			for (; _vI != _vE; ++_vI)
			{
				SlotItemVec* items = *_vI;
				auto _iI = items->begin();
				auto _iE = items->end();

				for (; _iI != _iE; ++_iI)
				{
					if (currentInstanceCount >= hw_MaxInstances)
					{
						hw_InstanceVB->Unlock();
						RenderBackend.set_Geometry(hw_Geom);
						HW.pDevice->SetStreamSource(1, hw_InstanceVB, 0, sizeof(InstanceData));
						HW.pDevice->SetStreamSourceFreq(0, (D3DSTREAMSOURCE_INDEXEDDATA | currentInstanceCount));
						HW.pDevice->SetStreamSourceFreq(1, (D3DSTREAMSOURCE_INSTANCEDATA | 1));
						RenderBackend.Render(D3DPT_TRIANGLELIST, vOffset, 0, Object.number_vertices, iOffset,
											 primCount);

						Device.Statistic->RenderDUMP_DT_Count += currentInstanceCount;
						RenderBackend.stat.r.s_details.add(currentInstanceCount * Object.number_vertices);

						currentInstanceCount = 0;
						hw_InstanceVB->Lock(0, hw_MaxInstances * sizeof(InstanceData), (void**)&pInstances,
											D3DLOCK_DISCARD);
					}

					SlotItem& Instance = **_iI;
					float scale = Instance.scale_calculated;
					Fmatrix& M = Instance.mRotY;

					pInstances[currentInstanceCount].Mat0.set(M._11 * scale, M._21 * scale, M._31 * scale, M._41);
					pInstances[currentInstanceCount].Mat1.set(M._12 * scale, M._22 * scale, M._32 * scale, M._42);
					pInstances[currentInstanceCount].Mat2.set(M._13 * scale, M._23 * scale, M._33 * scale, M._43);

					float h = Instance.c_hemi;
					float s = Instance.c_sun;
					pInstances[currentInstanceCount].Color.set(s, s, s, h);

					currentInstanceCount++;
				}
			}
			hw_InstanceVB->Unlock();

			if (currentInstanceCount > 0)
			{
				RenderBackend.set_Geometry(hw_Geom);
				HW.pDevice->SetStreamSource(1, hw_InstanceVB, 0, sizeof(InstanceData));
				HW.pDevice->SetStreamSourceFreq(0, (D3DSTREAMSOURCE_INDEXEDDATA | currentInstanceCount));
				HW.pDevice->SetStreamSourceFreq(1, (D3DSTREAMSOURCE_INSTANCEDATA | 1));
				RenderBackend.Render(D3DPT_TRIANGLELIST, vOffset, 0, Object.number_vertices, iOffset, primCount);
				Device.Statistic->RenderDUMP_DT_Count += currentInstanceCount;
				RenderBackend.stat.r.s_details.add(currentInstanceCount * Object.number_vertices);
			}

			HW.pDevice->SetStreamSource(1, NULL, 0, 0);
			HW.pDevice->SetStreamSourceFreq(0, 1);
			HW.pDevice->SetStreamSourceFreq(1, 1);
		}

		vOffset += Object.number_vertices;
		iOffset += Object.number_indices;
	}
}
