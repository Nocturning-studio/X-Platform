#pragma once
#ifndef DetailManagerH
#define DetailManagerH

#include "xrpool.h"
#include "detailformat.h"
#include "detailmodel.h"
#include <ppl.h> // PPL для многопоточности

#ifdef _EDITOR
#include "ESceneClassList.h"
#endif

// === НАСТРОЙКИ БУФЕРА ===
// Увеличиваем размер кеша, чтобы физически поддерживать большой радиус.
// 128 слотов * 2 метра = 256 метров максимальный радиус.
const int dm_size = 128;

const int dm_max_decompress = 7;
const int dm_cache1_count = 4;
const int dm_cache1_line = dm_size * 2 / dm_cache1_count;
const int dm_max_objects = 64;
const int dm_obj_in_slot = 4;
const int dm_cache_line = dm_size + 1 + dm_size;
const int dm_cache_size = dm_cache_line * dm_cache_line;
const float dm_slot_size = DETAIL_SLOT_SIZE;

// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ (EXTERN) ===
// Это позволяет DetailManager.cpp видеть переменные, объявленные в xrRender_console.cpp
extern float ps_r_Detail_density;
extern float ps_r_Detail_radius;
extern float ps_r_Detail_scale;
extern float ps_r_Detail_height;
extern u32 ps_r_Detail_quality;

enum class DetailsRenderMode
{
	Default,
	DepthOnly
};

class CDetailManager
{
  public:
	// Структура для передачи параметров рендеринга
	struct SDetailRenderContext
	{
		DetailsRenderMode mode;
		const fmat4x4* cullMatrix;
		const CFrustum* cullFrustum;
		fvec3 c_sun;
		fvec3 c_ambient;
		fvec3 c_hemi;
		float minX, maxX;
		float minZ, maxZ;
		bool useAABB;

		SDetailRenderContext()
			: mode(DetailsRenderMode::Default), cullMatrix(nullptr), cullFrustum(nullptr), minX(0), maxX(0), minZ(0),
			  maxZ(0), useAABB(false)
		{
		}
	};

	// Индексы для списков видимости m_visibles[id][INDEX]
	enum EDetailVisibilityList
	{
		DVL_Static = 0,
		DVL_Wave1 = 1,
		DVL_Wave2 = 2
	};

	// Тип шейдера (для выбора внутри шейдерного элемента объекта)
	enum EDetailShaderType
	{
		DST_Animated = 0,
		DST_Static = 1
	};

	// Структуры
	// Выравниваем структуру для SSE операций
	struct __declspec(align(16)) SlotItem
	{
		fmat4x4 mRotY;			// 64 bytes
		float scale;			// 4
		float scale_calculated; // 4
		float c_hemi;			// 4
		float c_sun;			// 4
		u32 vis_ID;				// 4
		u32 vis_ID_backup;		// 4
		u32 _pad;				// padding to 88 bytes or optimize order later
	};
	// Используем xr_vector с объектами
	typedef xr_vector<SlotItem> SlotItemVec;
	typedef SlotItemVec::iterator SlotItemVecIt;

	struct InstanceData
	{
		fvec4 Mat0;
		fvec4 Mat1;
		fvec4 Mat2;
		fvec4 Color;
	};

	struct DetailBatch
	{
		xr_vector<fvec3> positions;	   // Для CPU (Culling)
		xr_vector<InstanceData> instances; // Для GPU (Memcpy)

		void clear_not_free()
		{
			positions.clear_not_free();
			instances.clear_not_free();
		}

		bool empty() const
		{
			return instances.empty();
		}
	};
	typedef DetailBatch DetailRenderVec;

	struct SlotPart
	{
		u32 id;
		SlotItemVec items;			   // Исходные айтемы (для логики)
		DetailRenderVec r_items[2][3]; // Готовые данные [buffer][wave]
	};

	typedef xr_vector<xr_vector<DetailRenderVec*>> vis_list;

	enum SlotType
	{
		stReady = 0,
		stPending,
		stFORCEDWORD = 0xffffffff
	};

	struct Slot
	{
		struct
		{
			u32 empty : 1;
			u32 type : 1;
			u32 frame : 30;
		};
		int sx, sz;
		vis_data vis;
		SlotPart G[dm_obj_in_slot];
		Slot()
		{
			frame = 0;
			empty = 1;
			type = stReady;
			sx = sz = 0;
			vis.clear();
		}
	};

	struct CacheSlot1
	{
		u32 empty;
		vis_data vis;
		Slot** slots[dm_cache1_count * dm_cache1_count];
		CacheSlot1()
		{
			empty = 1;
			vis.clear();
		}
	};

	// === ИЗМЕНЕНИЕ: Двойной буфер ===
	// [2] - два набора данных
	// [3] - три волны (Static, Wave1, Wave2)
	vis_list m_visibles[2][3];

	u32 m_vis_render_id; // Индекс буфера, который сейчас рисуем
	u32 m_vis_calc_id;	 // Индекс буфера, который сейчас считаем

	// Сохраненная позиция камеры для расчета в потоке (чтобы не было гонок данных с Device)
	fvec3 m_vCameraPos_calc; 
	fmat4x4 m_mFullTransform_calc;
	typedef svector<CDetail*, dm_max_objects> DetailVec;
	typedef DetailVec::iterator DetailIt;
	typedef poolSS<SlotItem, 4096> PSS;

  private:
	// Внутренние методы рендеринга (Single Responsibility)
	void ExecuteRenderPasses(const SDetailRenderContext& ctx);
	void ProcessObjects(const SDetailRenderContext& ctx, EDetailVisibilityList visListType,
						EDetailShaderType shaderType);

	// Хелпер для отрисовки батча
	void FlushBatch(CDetail& Object, u32 instanceCount, u32& vOffset, u32& iOffset);

	// Хелпер для выбора шейдера
	ref_selement SelectShader(CDetail& Object, DetailsRenderMode mode, EDetailShaderType shaderType);

  public:
	int dither[16][16];
	IReader* dtFS;
	DetailHeader dtH;
	DetailSlot* dtSlots;
	DetailSlot DS_empty;

	DetailVec objects;

	IDirect3DVertexBuffer9* hw_InstanceVB;
	u32 hw_MaxInstances;
	u32 hw_BatchOffset;

#ifndef _EDITOR
	xrXRC xrc; // Глобальный XRC (не для потоков)
#endif

	CacheSlot1 cache_level1[dm_cache1_line][dm_cache1_line];
	Slot* cache[dm_cache_line][dm_cache_line];
	svector<Slot*, dm_cache_size> cache_task;
	Slot cache_pool[dm_cache_size];
	int cache_cx;
	int cache_cz;

	void UpdateVisibility();

#ifdef _EDITOR
	virtual ObjectList* GetSnapList() = 0;
#endif

	// Hard
	ref_geom hw_Geom;
	u32 hw_BatchSize;
	ref_constant hwc_array;
	ref_constant hwc_s_array;
	IDirect3DVertexBuffer9* hw_VB;
	IDirect3DIndexBuffer9* hw_IB;
	void hw_Load();
	void hw_Unload();
	void Render(DetailsRenderMode Mode, fmat4x4* pCullMatrix = nullptr, const CFrustum* pExternalCull = nullptr);

	DetailSlot& QueryDB(int sx, int sz);

	void cache_Initialize();
	void cache_Update(int sx, int sz, fvec3& view, int limit);
	void cache_Task(int gx, int gz, Slot* D);
	Slot* cache_Query(int sx, int sz);

	// Decompress принимает локальный XRC
	void cache_Decompress(Slot* D, xrXRC& local_xrc);

	// Метод сброса кеша при смене настроек
	void InvalidateCache();

	BOOL cache_Validate();

	int cg2w_X(int x)
	{
		return cache_cx - dm_size + x;
	}
	int cg2w_Z(int z)
	{
		return cache_cz - dm_size + (dm_cache_line - 1 - z);
	}
	int w2cg_X(int x)
	{
		return x - cache_cx + dm_size;
	}
	int w2cg_Z(int z)
	{
		return cache_cz - dm_size + (dm_cache_line - 1 - z);
	}

	void Load();
	void Unload();
	void PrepareToCalc();
	void ClearVisible();

	xrCriticalSection MT;
	volatile u32 m_frame_calc;
	volatile u32 m_frame_rendered;

	void __stdcall MT_CALC();

	CDetailManager();
	virtual ~CDetailManager();
};
#endif
