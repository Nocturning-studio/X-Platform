// CDemoPlay.h: interface for the CDemoPlay class.
//
//////////////////////////////////////////////////////////////////////
#pragma once
//////////////////////////////////////////////////////////////////////
#include "effector.h"
#include "iinputreceiver.h"
//////////////////////////////////////////////////////////////////////
class ENGINE_API CDemoPlay : public CEffectorCam, public IInputReceiver
{
	float fFPS, fFPS_min, fFPS_max, fFPS_avg, fAllFrames;

	int m_frames_count;
	float fStartTime;
	float fSpeed;
	u32 dwCyclesLeft;

	bool m_bGlobalHudDraw;
	bool m_bGlobalCrosshairDraw;

	float4x4 m_OriginalViewMatrix;

	bool m_bBenchmarkMode;

	// statistics
	BOOL stat_started;
	bool bNeedDrawResults;
	bool bNeedToTakeStatsResoultScreenShot;
	u32 uTimeToQuit;
	u32 uTimeToScreenShot;

	CTimer stat_Timer_frame;
	CTimer stat_Timer_total;
	u32 stat_StartFrame;
	xr_vector<float> stat_table;

	float4x4 m_Camera;

	bool m_bIsFirstFrame;

	void Update(SCamEffectorInfo& info);
	void ResetPerFrameStatistic();
	void ShowPerFrameStatistic();
	void ChooseTextColor(float FPSValue);
	void MoveCameraSpline(float InterpolationFactor, int frame0, int frame1, int frame2, int frame3);
	void MoveCameraLinear(float InterpolationFactor, int frame0, int frame1);
	void MoveCamera(u32 frame, float interpolation_factor, int interpolation_type);
	void Screenshot();

	void PrintSummaryBenchmarkStatistic();
	void EnableBenchmarkResultPrint();
	void SaveBenchmarkResults();

	float4x4 GetFrameMatrix(int frame);
	bool IsCut(int frame);

	void Close();

	void IR_OnKeyboardPress(int dik);

  public:
	virtual BOOL ProcessCam(SCamEffectorInfo& info);
	CDemoPlay(const char* name, float ms, u32 cycles, float life_time = 60 * 60 * 1000);
	virtual ~CDemoPlay();
};
//////////////////////////////////////////////////////////////////////
