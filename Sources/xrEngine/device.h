#ifndef xr_device
#define xr_device
#pragma once

// Note:
// ZNear - always 0.0f
// ZFar  - always 1.0f

class ENGINE_API CResourceManager;
class ENGINE_API CGammaControl;

#include "pure.h"
#include "hw.h"
#include "ftimer.h"
#include "stats.h"
#include "xr_effgamma.h"
#include "shader.h"
#include "R_Backend.h"
#include <mutex>
#include "../xrCore/Event.hpp"

// Thread Id's
extern DWORD gMainThreadId;
extern DWORD gSecondaryThreadId;

#define VIEWPORT_NEAR 0.2f
#define VIEWPORT_NEAR_HUD 0.01f

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

// refs
class ENGINE_API CRenderDevice
{
  public:
	u32 Timer_MM_Delta;
	CTimer_paused Timer;
	CTimer_paused TimerGlobal;
	CTimer TimerMM;
	CTimer frame_timer;

  private:
	// Main objects used for creating and rendering the 3D scene
	u32 m_dwWindowStyle;
	RECT m_rcWindowBounds;
	RECT m_rcWindowClient;

	void _Create(LPCSTR shName);
	void _Destroy(BOOL bKeepTextures);
	void _SetupStates();

  public:
	HWND m_hWnd;
	LRESULT MsgProc(HWND, UINT, WPARAM, LPARAM);

	u32 dwFrame;
	u32 dwPrecacheFrame;
	u32 dwPrecacheTotal;

	u32 dwWidth, dwHeight;
	float fWidth_2, fHeight_2;
	BOOL b_is_Ready;
	BOOL b_is_Active;
	void OnWM_Activate(WPARAM wParam, LPARAM lParam);

	Fvector2 GetScreenResolution()
	{
		Fvector2 Resolution;

		Resolution.x = (float)dwWidth;
		Resolution.y = (float)dwHeight;

		return Resolution;
	}

	void SetScreenResolution(Ivector2 Resolution)
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
	void SetNearer(BOOL enabled)
	{
		if (enabled && !m_bNearer)
		{
			m_bNearer = TRUE;
			mProject._43 -= EPS_L;
		}
		else if (!enabled && m_bNearer)
		{
			m_bNearer = FALSE;
			mProject._43 += EPS_L;
		}
		RenderBackend.set_xform_project(mProject);
	}

  public:
	// Registrators
	CRegistrator<pureRender> seqRender;
	CRegistrator<pureAppActivate> seqAppActivate;
	CRegistrator<pureAppDeactivate> seqAppDeactivate;
	CRegistrator<pureAppStart> seqAppStart;
	CRegistrator<pureAppEnd> seqAppEnd;
	CRegistrator<pureFrame> seqFrame;
	CRegistrator<pureFrame> seqFrameMT;
	CRegistrator<pureDeviceReset> seqDeviceReset;
	xr_vector<fastdelegate::FastDelegate0<>> seqParallel;

	// Dependent classes
	CResourceManager* Resources;
	CStats* Statistic;
	CGammaControl Gamma;

	// Engine flow-control
	float fTimeDelta;
	float fTimeGlobal;
	u32 dwTimeDelta;
	u32 dwTimeGlobal;
	u32 dwTimeContinual;

	// Cameras & projection
	Fvector vCameraPosition;
	Fvector vCameraPosition_saved;
	Fvector vCameraDirection;
	Fvector vCameraTop;
	Fvector vCameraRight;
	Fmatrix mView;
	Fmatrix mProject;
	Fmatrix mProject_hud;
	Fmatrix mFullTransform;
	Fmatrix mInvFullTransform;
	Fmatrix mFullTransform_saved;
	float fFOV;
	float fASPECT;

	CRenderDevice()
#ifdef PROFILE_CRITICAL_SECTIONS
		: mt_csEnter(MUTEX_PROFILE_ID(CRenderDevice::mt_csEnter)),
		  mt_csLeave(MUTEX_PROFILE_ID(CRenderDevice::mt_csLeave))
#endif // PROFILE_CRITICAL_SECTIONS
	{
		m_hWnd = NULL;
		b_is_Active = FALSE;
		b_is_Ready = FALSE;
		Timer.Start();
		m_bNearer = FALSE;
	};

	void Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
	BOOL Paused();

private:
	static void SecondaryThreadProc(void* context);
	static void RenderThreadProc(void* context);

public:

	// Scene control
	void PreCache(u32 frames);
	BOOL Begin();
	void Clear();
	void End();
	void FrameMove();

	void overdrawBegin();
	void overdrawEnd();

	// Mode control
	void DumpFlags();
	IC CTimer_paused* GetTimerGlobal()
	{
		return &TimerGlobal;
	}
	u32 TimerAsync()
	{
		return TimerGlobal.GetElapsed_ms();
	}
	u32 TimerAsync_MMT()
	{
		return TimerMM.GetElapsed_ms() + Timer_MM_Delta;
	}

	// Creation & Destroying
	void Create(void);
	void PrepareEventLoop();
	void StartEventLoop(void);
	void EndEventLoop();
	void Destroy(void);
	void Reset(bool precache = true);

	void Initialize(void);
	void ShutDown(void);

  public:
	void time_factor(const float& time_factor);

	IC const float& time_factor() const;

	void stop_time();

	// Multi-threading
	std::recursive_mutex mt_csEnter;
	std::recursive_mutex mt_csLeave;

private:
	Event syncProcessFrame, syncFrameDone, syncThreadExit;		 // Secondary thread events
	Event renderProcessFrame, renderFrameDone, renderThreadExit; // Render thread events

public:

	volatile BOOL mt_bMustExit;

	ICF void remove_from_seq_parallel(const fastdelegate::FastDelegate0<>& delegate)
	{
		xr_vector<fastdelegate::FastDelegate0<>>::iterator I =
			std::find(seqParallel.begin(), seqParallel.end(), delegate);
		if (I != seqParallel.end())
			seqParallel.erase(I);
	}
	IC u32 frame_elapsed()
	{
		return frame_timer.GetElapsed_ms();
	}
};

extern ENGINE_API CRenderDevice Device;

typedef fastdelegate::FastDelegate0<bool> LOADING_EVENT;
extern ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

#include "R_Backend_Runtime.h"

#endif
