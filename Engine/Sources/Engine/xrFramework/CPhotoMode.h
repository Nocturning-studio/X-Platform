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
		fvec3 p;
	} g_position;

	static fvec3 cmNorm[6];
	static fvec3 cmDir[6];

	static Flags32 s_hud_flag;
	static Flags32 s_dev_flags;

	fvec3 m_HPB;
	fvec3 m_Position;
	fmat4x4 m_Camera;
	u32 m_Stage;

	fvec3 m_ActorPosition;

	fvec3 m_vT;
	fvec3 m_vR;
	fvec3 m_vVelocity;
	fvec3 m_vAngularVelocity;
	float m_fFov;
	float m_fGlobalFov;
	float m_fFovNeeded;
	fvec3 m_fDOF;
	fvec3 m_vGlobalDepthOfFieldParameters;
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

	void MakeCubeMapFace(fvec3& D, fvec3& N);
	void MakeScreenshotFace();
	void MakeCubemap();
	void MakeScreenshot();
	void ShowInputInfo();

	ref_sound music;

  public:
	void update_whith_timescale(fvec3& v, const fvec3& v_delta);
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

	static void SetGlobalPosition(const fvec3& p)
	{
		g_position.p.set(p), g_position.set_position = true;
	}

	static void GetGlobalPosition(fvec3& p)
	{
		p.set(g_position.p);
	}
};
//////////////////////////////////////////////////////////////////////
