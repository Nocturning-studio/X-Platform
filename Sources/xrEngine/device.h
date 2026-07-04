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
#include "xr_effgamma.h"
#include "shader.h"
#include "R_Backend.h"
#include <mutex>

#define VIEWPORT_NEAR 0.2f
#define VIEWPORT_NEAR_HUD 0.01f

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

// refs
class ENGINE_API CRenderDevice
{
  private:
	// Main objects used for creating and rendering the 3D scene
	u32 m_dwWindowStyle;
	RECT m_rcWindowBounds;
	RECT m_rcWindowClient;

	void _Create(LPCSTR shName);
	void _Destroy(BOOL bKeepTextures);
	void _SetupStates();

  public:
	u32 dwPrecacheFrame;
	u32 dwPrecacheTotal;

	u32 dwWidth, dwHeight;
	float fWidth_2, fHeight_2;
	BOOL b_is_Ready;
	BOOL b_is_Active;
	void OnWM_Activate(WPARAM wParam, LPARAM lParam);

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
	// Dependent classes
	CGammaControl Gamma;

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
	void PreCache(u32 frames);
	void PreCache();
	BOOL Begin();
	void Clear();
	void End();

	// Mode control
	void DumpFlags();

	// Creation & Destroying
	void Create(void);
	void PrepareEventLoop();
	void RenderFrame();
	void EndEventLoop();
	void Destroy(void);
	void Reset(bool precache = true);

	void Initialize(void);
};

extern ENGINE_API CRenderDevice Device;

#endif
