// HOM.h: interface for the CHOM class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "../xrEngine/IGame_Persistent.h"
#include <occRasterizer.h>

class occTri;

class CHOM
#ifdef DEBUG
	: public pureRender
#endif
{
  private:
	xrXRC xrc;
	CDB::MODEL* m_pModel;
	occTri* m_pTris;
	BOOL bEnabled;
	float4x4 m_transform;
	float4x4 m_transform_01;
#ifdef DEBUG
	u32 tris_in_frame_visible;
	u32 tris_in_frame;
#endif

	xrCriticalSection MT;
	volatile u32 MT_frame_rendered;

	// Двойная буферизация растеризатора
	// 0 - читаем (Main Thread), 1 - пишем (Worker Thread), и наоборот
	occRasterizer m_Raster[2];
	u32 m_idx_read;	 // Индекс буфера, из которого читает visible()
	u32 m_idx_write; // Индекс буфера, в который пишет MT_RENDER()

	void ProcessTriangle(CDB::RESULT* it, u32 _frame, const float3& COP, CFrustum& clip);

	void Render_DB(CFrustum& base);

  public:
	void Load();
	void Unload();
	void Render(CFrustum& base);

	void occlude(Fbox2& space)
	{
	}
	void Disable();
	void Enable();

	void StartFrame();

	void __stdcall MT_RENDER();
	ICF void MT_SYNC()
	{
	}

	BOOL visible(vis_data& vis);
	BOOL visible(Fbox3& B);
	BOOL visible(sPoly& P);
	BOOL visible(Fbox2& B, float depth); // viewport-space (0..1)

	CHOM();
	~CHOM();

#ifdef DEBUG
	virtual void OnRender();
	void stats();
#endif
};
