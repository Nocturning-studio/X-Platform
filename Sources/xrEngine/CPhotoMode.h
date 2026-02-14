//////////////////////////////////////////////////////////////////////
// CPhotoMode.h: interface for the CPhotoMode class
//////////////////////////////////////////////////////////////////////
#pragma once
//////////////////////////////////////////////////////////////////////
#include "iinputreceiver.h"
#include "effector.h"
//////////////////////////////////////////////////////////////////////
class ENGINE_API CPhotoMode : public CEffectorCam, public IInputReceiver
{
  protected:
	CObject* Actor;

  private:
	static struct force_position
	{
		bool set_position;
		float3 p;
	} g_position;

	static float3 cmNorm[6];
	static float3 cmDir[6];

	static Flags32 s_hud_flag;
	static Flags32 s_dev_flags;

	float3 m_HPB;
	float3 m_Position;
	float4x4 m_Camera;
	u32 m_Stage;

	float3 m_ActorPosition;

	float3 m_vT;
	float3 m_vR;
	float3 m_vVelocity;
	float3 m_vAngularVelocity;
	float m_fFov;
	float m_fGlobalFov;
	float m_fFovNeeded;
	float3 m_fDOF;
	float3 m_vGlobalDepthOfFieldParameters;
	float m_fGlobalTimeFactor;

	bool m_bAutofocusEnabled;
	bool m_bGridEnabled;
	bool m_bBordersEnabled;
	bool m_bWatermarkEnabled;
	bool m_bShowInputInfo;
	bool m_bGlobalHudDraw;
	bool m_bGlobalCrosshairDraw;

	bool m_bActorShowState;

	BOOL m_bMakeCubeMap;
	BOOL m_bMakeScreenshot;
	int m_iLMScreenshotFragment;

	float m_fSpeed0;
	float m_fSpeed1;
	float m_fSpeed2;
	float m_fSpeed3;
	float m_fAngSpeed0;
	float m_fAngSpeed1;
	float m_fAngSpeed2;
	float m_fAngSpeed3;

	void MakeCubeMapFace(float3& D, float3& N);
	void MakeScreenshotFace();
	void MakeCubemap();
	void MakeScreenshot();
	void ShowInputInfo();

	ref_sound music;

  public:
	void update_whith_timescale(float3& v, const float3& v_delta);
	CPhotoMode(float life_time = 60 * 60 * 1000);
	virtual ~CPhotoMode();

	void ResetParameters();

	void ChangeDepthOfFieldFocalLength(int direction);
	void ChangeDepthOfFieldFocalDepth(int direction);
	void ChangeDepthOfFieldFStop(int direction);
	void ChangeFieldOfView(int direction);
	void SwitchAutofocusState();
	void SwitchGridState();
	void SwitchCinemaBordersState();
	void SwitchWatermarkVisibility();
	void SwitchActorVisibility();
	void SwitchShowInputInfo();

	void ShowInfo();

	virtual void IR_OnKeyboardPress(int dik);
	virtual void IR_OnKeyboardHold(int dik);
	virtual void IR_OnMouseMove(int dx, int dy);
	virtual void IR_OnMouseHold(int btn);
	virtual void IR_OnMouseWheel(int direction);

	virtual BOOL ProcessCam(SCamEffectorInfo& info);

	static void SetGlobalPosition(const float3& p)
	{
		g_position.p.set(p), g_position.set_position = true;
	}

	static void GetGlobalPosition(float3& p)
	{
		p.set(g_position.p);
	}
};
//////////////////////////////////////////////////////////////////////
