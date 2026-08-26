#ifndef xr_device
#define xr_device
#pragma once

// Note:
// ZNear - always 0.0f
// ZFar  - always 1.0f

class ENGINE_API CGammaControl;

#include "pure.h"
#include "ftimer.h"
#include "stats.h"
#include "shader.h"
#include "R_Backend.h"
#include <mutex>

#define VIEWPORT_NEAR 0.2f
#define VIEWPORT_NEAR_HUD 0.01f

// refs
class ENGINE_API CRenderDevice
{
  private:
	// Main objects used for creating and rendering the 3D scene
	u32 m_dwWindowStyle;
	RECT m_rcWindowBounds;
	RECT m_rcWindowClient;

  public:
	u32 dwWidth, dwHeight;
	float fWidth_2, fHeight_2;
	BOOL b_is_Ready;
	bool b_is_Active;
	void SetActivate(bool bActive);

	fvec2 GetScreenResolution()
	{
		fvec2 Resolution;

		Resolution.x = (float)dwWidth;
		Resolution.y = (float)dwHeight;

		return Resolution;
	}

	void SetScreenResolution(ivec2 Resolution)
	{
		dwWidth = Resolution.x;
		dwHeight = Resolution.y;
	}

	void SetScreenResolution(u32 ResolutionX, u32 ResolutionY)
	{
		dwWidth = ResolutionX;
		dwHeight = ResolutionY;
	}

  public:
	ref_shader m_WireShader;
	ref_shader m_SelectionShader;

	BOOL m_bNearer;
	void SetNearer(BOOL enabled);

  public:
	CRenderDevice()
#ifdef PROFILE_CRITICAL_SECTIONS
		: mt_csEnter(MUTEX_PROFILE_ID(CRenderDevice::mt_csEnter)),
		  mt_csLeave(MUTEX_PROFILE_ID(CRenderDevice::mt_csLeave))
#endif // PROFILE_CRITICAL_SECTIONS
	{
		b_is_Active = FALSE;
		b_is_Ready = FALSE;
		m_bNearer = FALSE;
	};

	void Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
	BOOL Paused();

public:

	// Scene control
	void Begin();
	void End();

	// Creation & Destroying
	void Initialize();
	void RenderFrame();
	void Destroy();
	void Reset();
};

extern ENGINE_API CRenderDevice Device;

#endif
