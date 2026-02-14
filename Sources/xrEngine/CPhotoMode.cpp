// CPhotoMode.cpp: implementation of the CPhotoMode class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "igame_level.h"
#include "Engine.h"

#include "gamefont.h"
#include "CPhotoMode.h"
#include "xr_ioconsole.h"
#include "xr_input.h"
#include "xr_object.h"
#include "render.h"
#include "CustomHUD.h"
#include "IGame_Persistent.h"
//////////////////////////////////////////////////////////////////////
CPhotoMode* xrPhotoMode = 0;

CPhotoMode::force_position 
CPhotoMode::g_position = 
{
	false, 
	{0.0f, 0.0f, 0.0f}
};

// +X, -X, +Y, -Y, +Z, -Z
float3 CPhotoMode::cmNorm[6] = 
{
	{0.f, 1.f, 0.f}, 
	{0.f, 1.f, 0.f}, 
	{0.f, 0.f, -1.f},	 
	{0.f, 0.f, 1.f}, 
	{0.f, 1.f, 0.f}, 
	{0.f, 1.f, 0.f}
};

float3 CPhotoMode::cmDir[6] = 
{
	{1.f, 0.f, 0.f},  
	{-1.f, 0.f, 0.f}, 
	{0.f, 1.f, 0.f},
	{0.f, -1.f, 0.f}, 
	{0.f, 0.f, 1.f},	
	{0.f, 0.f, -1.f}
};

Flags32 CPhotoMode::s_hud_flag = { NULL };
Flags32 CPhotoMode::s_dev_flags = { NULL };
//////////////////////////////////////////////////////////////////////
void CPhotoMode::update_whith_timescale(float3& v, const float3& v_delta)
{
	VERIFY(!fis_zero(Engine.TimeManager.GetTimeFactor()));
	float scale = 1.f / Engine.TimeManager.GetTimeFactor();
	v.mad(v, v_delta, scale);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////s
CPhotoMode::CPhotoMode(float life_time) : CEffectorCam(cefDemo, life_time)
{
	g_position.set_position = false;
	IR_Capture(); // capture input
	m_Camera.invert(Engine.RenderView.View);

	// parse yaw
	float3& dir = m_Camera.k;
	float3 DYaw;
	DYaw.set(dir.x, 0.f, dir.z);
	DYaw.normalize_safe();

	if (DYaw.x < 0)
		m_HPB.x = acosf(DYaw.z);
	else
		m_HPB.x = 2 * PI - acosf(DYaw.z);

	// parse pitch
	dir.normalize_safe();
	m_HPB.y = asinf(dir.y);
	m_HPB.z = 0;

	m_Position.set(m_Camera.c);
	m_ActorPosition.set(m_Position);

	m_vVelocity.set(0, 0, 0);
	m_vAngularVelocity.set(0, 0, 0);

	m_fGlobalFov = Engine.RenderView.Fov;
	m_fFov = m_fGlobalFov;

	m_fGlobalTimeFactor = Engine.TimeManager.GetTimeFactor();
	Engine.TimeManager.StopTime();

	g_pGamePersistent->GetCurrentDof(m_vGlobalDepthOfFieldParameters);
	m_fDOF.set(m_vGlobalDepthOfFieldParameters);
	m_fDOF.z = 2.0f;
	m_fDOF.x = 10.0f;
	g_pGamePersistent->SetBaseDof(m_fDOF);

	m_bAutofocusEnabled = false;
	m_bGridEnabled = false;
	m_bBordersEnabled = false;
	m_bShowInputInfo = true;
	m_bWatermarkEnabled = false;

	m_bGlobalHudDraw = psHUD_Flags.test(HUD_DRAW);
	psHUD_Flags.set(HUD_DRAW, false);

	m_bGlobalCrosshairDraw = psHUD_Flags.test(HUD_CROSSHAIR);
	psHUD_Flags.set(HUD_CROSSHAIR, false);

	s_hud_flag.assign(psHUD_Flags);
	psHUD_Flags.assign(0);

	m_vT.set(0, 0, 0);
	m_vR.set(0, 0, 0);
	m_bMakeCubeMap = FALSE;
	m_bMakeScreenshot = FALSE;

	m_fSpeed0 = pSettings->r_float("demo_record", "speed0");
	m_fSpeed1 = pSettings->r_float("demo_record", "speed1");
	m_fSpeed2 = pSettings->r_float("demo_record", "speed2");
	m_fSpeed3 = pSettings->r_float("demo_record", "speed3");
	m_fAngSpeed0 = pSettings->r_float("demo_record", "ang_speed0");
	m_fAngSpeed1 = pSettings->r_float("demo_record", "ang_speed1");
	m_fAngSpeed2 = pSettings->r_float("demo_record", "ang_speed2");
	m_fAngSpeed3 = pSettings->r_float("demo_record", "ang_speed3");

	m_Stage = 0;
	m_iLMScreenshotFragment = 0;

	//TODO - replace with UI Sounds
	music.create("photo_mode_dbg_sound", st_Music, sg_Undefined);
	music.play_at_pos(0, Engine.RenderView.Position, sm_NoPitch);

	Actor = g_pGameLevel->CurrentViewEntity();
	m_bActorShowState = true;
	Actor->setVisible(TRUE);
}

CPhotoMode::~CPhotoMode()
{
	IR_Release();

	music.stop();
	music.destroy();

	ResetParameters();

	psHUD_Flags.assign(s_hud_flag);

	psHUD_Flags.set(HUD_DRAW, m_bGlobalHudDraw);
	psHUD_Flags.set(HUD_CROSSHAIR, m_bGlobalCrosshairDraw);

	Engine.TimeManager.SetTimeFactor(m_fGlobalTimeFactor);
}

void CPhotoMode::ResetParameters()
{
	m_fDOF = m_vGlobalDepthOfFieldParameters;
	g_pGamePersistent->SetBaseDof(m_fDOF);
	g_pGamePersistent->SetPickableEffectorDOF(false);

	m_fFov = m_fGlobalFov;

	Console->Execute("r_photo_grid off");
	Console->Execute("r_cinema_borders off");
	Console->Execute("r_watermark off");

#ifndef MASTER_GOLD
	Console->Execute("r_debug_render disabled");
#endif
}

void CPhotoMode::MakeScreenshotFace()
{
	switch (m_Stage)
	{
	case 0:
		break;
	case 1:
		Render->Screenshot();
		m_bMakeScreenshot = FALSE;
		break;
	}
	m_Stage++;
}

void CPhotoMode::MakeCubeMapFace(float3& D, float3& N)
{
	string32 buf;
	switch (m_Stage)
	{
	case 0:
		N.set(cmNorm[m_Stage]);
		D.set(cmDir[m_Stage]);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		N.set(cmNorm[m_Stage]);
		D.set(cmDir[m_Stage]);
		Render->Screenshot(IRender_interface::SM_FOR_CUBEMAP, itoa(m_Stage, buf, 10));
		break;
	case 6:
		Render->Screenshot(IRender_interface::SM_FOR_CUBEMAP, itoa(m_Stage, buf, 10));
		N.set(m_Camera.j);
		D.set(m_Camera.k);
		m_bMakeCubeMap = FALSE;
		break;
	}
	m_Stage++;
}

void CPhotoMode::SwitchShowInputInfo()
{
	if (m_bShowInputInfo == true)
		m_bShowInputInfo = false;
	else
		m_bShowInputInfo = true;
}

void CPhotoMode::ShowInfo()
{
	Engine.FontManager.GetSystemFont()->SetColor(color_rgba(215, 195, 170, 255));
	Engine.FontManager.GetSystemFont()->SetAligment(CGameFont::alLeft);
	Engine.FontManager.GetSystemFont()->OutSetI(-1.0, -1.0f);

	Engine.FontManager.GetSystemFont()->OutNext("DOF fStop: %f", m_fDOF.z);
	Engine.FontManager.GetSystemFont()->OutNext("DOF Focal depth: %f", m_fDOF.x);
	Engine.FontManager.GetSystemFont()->OutNext("DOF Focal length: %f", m_fDOF.y);
	Engine.FontManager.GetSystemFont()->OutNext("FOV: %f", m_fFov);
}

void CPhotoMode::ShowInputInfo()
{
	Engine.FontManager.GetSystemFont()->SetAligment(CGameFont::alLeft);
	Engine.FontManager.GetSystemFont()->OutSetI(-0.2f, -.25f);
	Engine.FontManager.GetSystemFont()->OutNext("TAB");
	Engine.FontManager.GetSystemFont()->OutNext("BACK");
	Engine.FontManager.GetSystemFont()->OutNext("ESC");
	Engine.FontManager.GetSystemFont()->OutNext("F12");
	Engine.FontManager.GetSystemFont()->OutNext("G + Mouse Wheel");
	Engine.FontManager.GetSystemFont()->OutNext("T + Mouse Wheel");
	Engine.FontManager.GetSystemFont()->OutNext("R + Mouse Wheel");
	Engine.FontManager.GetSystemFont()->OutNext("F + Mouse Wheel");
	Engine.FontManager.GetSystemFont()->OutNext("H");
	Engine.FontManager.GetSystemFont()->OutNext("V");
	Engine.FontManager.GetSystemFont()->OutNext("B");
	Engine.FontManager.GetSystemFont()->OutNext("P");
	Engine.FontManager.GetSystemFont()->OutNext("DELETE");

#ifndef MASTER_GOLD
	Engine.FontManager.GetSystemFont()->OutNext("1, 2, ..., 0");
	Engine.FontManager.GetSystemFont()->OutNext("ENTER");
#endif

	Engine.FontManager.GetSystemFont()->SetAligment(CGameFont::alLeft);
	Engine.FontManager.GetSystemFont()->OutSetI(0, -.25f);
	Engine.FontManager.GetSystemFont()->OutNext("= Draw this help");
	Engine.FontManager.GetSystemFont()->OutNext("= Cube Map");
	Engine.FontManager.GetSystemFont()->OutNext("= Quit");
	Engine.FontManager.GetSystemFont()->OutNext("= ScreenShot");
	Engine.FontManager.GetSystemFont()->OutNext("= Depth of field Focal length");
	Engine.FontManager.GetSystemFont()->OutNext("= Depth of field Focal depth");
	Engine.FontManager.GetSystemFont()->OutNext("= Depth of field FStop");
	Engine.FontManager.GetSystemFont()->OutNext("= Field of view");
	Engine.FontManager.GetSystemFont()->OutNext("= Autofocus");
	Engine.FontManager.GetSystemFont()->OutNext("= Grid");
	Engine.FontManager.GetSystemFont()->OutNext("= Cinema borders");
	Engine.FontManager.GetSystemFont()->OutNext("= Switch show actor");
	Engine.FontManager.GetSystemFont()->OutNext("= Reset parameters");

#ifndef MASTER_GOLD
	Engine.FontManager.GetSystemFont()->OutNext("= Render debugging");
	Engine.FontManager.GetSystemFont()->OutNext("= Actor teleportation");
#endif
}

BOOL CPhotoMode::ProcessCam(SCamEffectorInfo& info)
{
	if (m_bMakeScreenshot)
	{
		MakeScreenshotFace();

		// update camera
		info.n.set(m_Camera.j);
		info.d.set(m_Camera.k);
		info.p.set(m_Camera.c);

		info.fFov = m_fFov;
	}
	else
	{
		ShowInfo();

		if (m_bShowInputInfo == true)
			ShowInputInfo();

		m_vVelocity.lerp(m_vVelocity, m_vT, 0.3f);
		m_vAngularVelocity.lerp(m_vAngularVelocity, m_vR, 0.3f);

		float speed = m_fSpeed1, ang_speed = m_fAngSpeed1;

		if (IR_GetKeyState(DIK_LSHIFT))
		{
			speed = m_fSpeed0;
			ang_speed = m_fAngSpeed0;
		}
		else if (IR_GetKeyState(DIK_LALT))
		{
			speed = m_fSpeed2;
			ang_speed = m_fAngSpeed2;
		}
		else if (IR_GetKeyState(DIK_LCONTROL))
		{
			speed = m_fSpeed3;
			ang_speed = m_fAngSpeed3;
		}

		m_vT.mul(m_vVelocity, Engine.TimeManager.GetDeltaTime() * speed);
		m_vR.mul(m_vAngularVelocity, Engine.TimeManager.GetDeltaTime() * ang_speed);

		m_HPB.x -= m_vR.y;
		m_HPB.y -= m_vR.x;
		m_HPB.z += m_vR.z;

		if (g_position.set_position)
		{
			m_Position.set(g_position.p);
			g_position.set_position = false;
		}
		else
		{
			g_position.p.set(m_Position);
		}

		float3 vmove;
		vmove.set(m_Camera.k);
		vmove.normalize_safe();
		vmove.mul(m_vT.z);
		m_Position.add(vmove);

		vmove.set(m_Camera.i);
		vmove.normalize_safe();
		vmove.mul(m_vT.x);
		m_Position.add(vmove);

		vmove.set(m_Camera.j);
		vmove.normalize_safe();
		vmove.mul(m_vT.y);
		m_Position.add(vmove);

		#pragma todo(NSDeathman to NSDeathman: ƒобавить ограничение дальности полета дл€ избегани€ читинга)
		m_Camera.setHPB(m_HPB.x, m_HPB.y, m_HPB.z);
		m_Camera.translate_over(m_Position);

		// update camera
		info.n.set(m_Camera.j);
		info.d.set(m_Camera.k);
		info.p.set(m_Camera.c);

		fLifeTime -= Engine.TimeManager.GetDeltaTime();

		m_vT.set(0, 0, 0);
		m_vR.set(0, 0, 0);

		info.fFov = m_fFov;

		double x = 43.266615300557;
		m_fDOF.y = (x / (2 * tan(PI * m_fFov / 360.f)));
		g_pGamePersistent->SetBaseDof(m_fDOF);
	}

	return TRUE;
}

void CPhotoMode::SwitchAutofocusState()
{
	if (m_bAutofocusEnabled == false)
		m_bAutofocusEnabled = true;
	else
		m_bAutofocusEnabled = false;

	g_pGamePersistent->SetPickableEffectorDOF(m_bAutofocusEnabled);
}

// –ешение действовать через консоль чудовищное, в будущем нужно заменить смену флага через консоль на смену через флаг
// общих команд дл€ всех рендеров
void CPhotoMode::SwitchGridState()
{
	if (m_bGridEnabled == false)
	{
		m_bGridEnabled = true;
		Console->Execute("r_photo_grid on");
	}
	else
	{
		m_bGridEnabled = false;
		Console->Execute("r_photo_grid off");
	}
}

// –ешение действовать через консоль чудовищное, в будущем нужно заменить смену флага через консоль на смену через флаг
// общих команд дл€ всех рендеров
void CPhotoMode::SwitchCinemaBordersState()
{
	if (m_bBordersEnabled == false)
	{
		m_bBordersEnabled = true;
		Console->Execute("r_cinema_borders on");
	}
	else
	{
		m_bBordersEnabled = false;
		Console->Execute("r_cinema_borders off");
	}
}

void CPhotoMode::SwitchWatermarkVisibility()
{
	if (m_bWatermarkEnabled == false)
	{
		m_bWatermarkEnabled = true;
		Console->Execute("r_watermark on");
	}
	else
	{
		m_bWatermarkEnabled = false;
		Console->Execute("r_watermark off");
	}
}

void CPhotoMode::SwitchActorVisibility()
{
	if (m_bActorShowState == false)
	{
		m_bActorShowState = true;
		Actor->setVisible(TRUE);
	}
	else
	{
		m_bActorShowState = false;
		Actor->setVisible(FALSE);
	}
}

void CPhotoMode::ChangeDepthOfFieldFocalDepth(int direction)
{
	float3 dof_params_old;
	float3 dof_params_actual;

	g_pGamePersistent->GetCurrentDof(dof_params_old);

	dof_params_actual = dof_params_old;

	float X = dof_params_old.x * 0.1f;

	if (direction > 0)
		dof_params_actual.x = dof_params_old.x + X;
	else
		dof_params_actual.x = dof_params_old.x - X;

	if (dof_params_actual.x <= 0.5f)
		dof_params_actual.x = 0.5f;

	if (dof_params_actual.x >= 100.0f)
		dof_params_actual.x = 100.0f;

	m_fDOF = dof_params_actual;
	g_pGamePersistent->SetBaseDof(m_fDOF);
}

void CPhotoMode::ChangeDepthOfFieldFocalLength(int direction)
{
	float3 dof_params_old;
	float3 dof_params_actual;

	g_pGamePersistent->GetCurrentDof(dof_params_old);

	dof_params_actual = dof_params_old;

	float X = dof_params_old.y * 0.25f;

	if (direction > 0)
		dof_params_actual.y = dof_params_old.y + X;
	else
		dof_params_actual.y = dof_params_old.y - X;

	// dof_params_actual.y = dof_params_actual.x + 10.0f;

	// if (dof_params_actual.x <= 0.1f)
	//	dof_params_actual.x = 0.1f;

	m_fDOF = dof_params_actual;
	g_pGamePersistent->SetBaseDof(m_fDOF);
}

void CPhotoMode::ChangeDepthOfFieldFStop(int direction)
{
	float3 dof_params_old;
	float3 dof_params_actual;

	g_pGamePersistent->GetCurrentDof(dof_params_old);

	dof_params_actual = dof_params_old;

	float X = dof_params_old.z * 0.1f;

	if (direction > 0)
		dof_params_actual.z = dof_params_old.z + X;
	else
		dof_params_actual.z = dof_params_old.z - X;

	if (dof_params_actual.z <= 0.1f)
		dof_params_actual.z = 0.1f;

	if (dof_params_actual.z >= 100.0f)
		dof_params_actual.z = 100.0f;

	m_fDOF = dof_params_actual;
	g_pGamePersistent->SetBaseDof(m_fDOF);
}

void CPhotoMode::ChangeFieldOfView(int direction)
{
	float m_fFov_actual = Engine.RenderView.Fov;

	if (direction > 0)
		m_fFov = m_fFov_actual + 0.5f;
	else
		m_fFov = m_fFov_actual - 0.5f;

	if (m_fFov <= 2.28f)
		m_fFov = 2.28f;
	else if (m_fFov >= 113.001f)
		m_fFov = 113.0f;
}

void CPhotoMode::MakeCubemap()
{
	m_bMakeCubeMap = TRUE;
	m_Stage = 0;
}

void CPhotoMode::MakeScreenshot()
{
	m_bMakeScreenshot = TRUE;
	m_Stage = 0;
}

void CPhotoMode::IR_OnKeyboardPress(int dik)
{
	if (dik == DIK_GRAVE)
		Console->Show();

	if (dik == DIK_BACK)
		MakeCubemap();

	if (dik == DIK_F12)
		MakeScreenshot();

	if (dik == DIK_ESCAPE)
		fLifeTime = -1;

	if (dik == DIK_DELETE)
		ResetParameters();

	if (dik == DIK_H)
		SwitchAutofocusState();

	if (dik == DIK_V)
		SwitchGridState();

	if (dik == DIK_B)
		SwitchCinemaBordersState();

	if (dik == DIK_TAB)
		SwitchShowInputInfo();

	if (dik == DIK_N)
		SwitchWatermarkVisibility();

	if (dik == DIK_P)
		SwitchActorVisibility();

	if (dik == DIK_RETURN)
	{
#ifndef MASTER_GOLD
			Actor->ForceTransform(m_Camera);
#endif
			fLifeTime = -1;
	}

	//if (dik == DIK_PAUSE)
	//	Device.Pause(!Device.Paused(), TRUE, TRUE, "photo_mode");

#ifndef MASTER_GOLD
#pragma todo("NSDeathman to all: ѕеределать быструю отладку рендера под удобный вид")
	if (dik == DIK_1)
		Console->Execute("r_debug_render gbuffer_albedo");
	if (dik == DIK_2)
		Console->Execute("r_debug_render gbuffer_position");
	if (dik == DIK_3)
		Console->Execute("r_debug_render gbuffer_normal");
	if (dik == DIK_4)
		Console->Execute("r_debug_render gbuffer_roughness");
	if (dik == DIK_5)
		Console->Execute("r_debug_render gbuffer_matallness");
	if (dik == DIK_6)
		Console->Execute("r_debug_render gbuffer_lightmap_ao");
	if (dik == DIK_7)
		Console->Execute("r_debug_render direct_light");
	if (dik == DIK_8)
		Console->Execute("r_debug_render indirect_light");
	if (dik == DIK_9)
		Console->Execute("r_debug_render real_time_ao");
	if (dik == DIK_0)
		Console->Execute("r_debug_render disabled");
#endif
}

void CPhotoMode::IR_OnKeyboardHold(int dik)
{
	float3 vT_delta{}, vR_delta{};
	switch (dik)
	{
	case DIK_A:
	case DIK_NUMPAD1:
		vT_delta.x -= 1.0f;
		break; // Slide Left
	case DIK_D:
	case DIK_NUMPAD3:
		vT_delta.x += 1.0f;
		break; // Slide Right
	case DIK_DOWN:
	case DIK_Q:
		vT_delta.y -= 1.0f;
		break; // Slide Down
	case DIK_UP:
	case DIK_E:
		vT_delta.y += 1.0f;
		break; // Slide Up
	// rotate
	case DIK_NUMPAD2:
		vR_delta.x -= 1.0f;
		break; // Pitch Down
	case DIK_NUMPAD8:
		vR_delta.x += 1.0f;
		break; // Pitch Up
	case DIK_NUMPAD6:
	case DIK_LEFT:
		vR_delta.y += 1.0f;
		break; // Turn Left
	case DIK_NUMPAD4:
	case DIK_RIGHT:
		vR_delta.y -= 1.0f;
		break; // Turn Right
	case DIK_NUMPAD9:
		vR_delta.z -= 2.0f;
		break; // Turn Right
	case DIK_NUMPAD7:
		vR_delta.z += 2.0f;
		break; // Turn Right
	case DIK_W:
		vT_delta.z += 1.0f;
		break; // Move Backward
	case DIK_S:
		vT_delta.z -= 1.0f;
		break; // Move Forward
	}
	update_whith_timescale(m_vT, vT_delta);
	update_whith_timescale(m_vR, vR_delta);
}

void CPhotoMode::IR_OnMouseMove(int dx, int dy)
{
	float scale = .5f; // psMouseSens;
	float3 vR_delta{};
	if (dx || dy)
	{
		vR_delta.y += float(dx) * scale;													// heading
		vR_delta.x += ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * (3.f / 4.f); // pitch
	}
	update_whith_timescale(m_vR, vR_delta);
}

void CPhotoMode::IR_OnMouseHold(int btn)
{

}

void CPhotoMode::IR_OnMouseWheel(int direction)
{
	if (IR_GetKeyState(DIK_G))
	{
		ChangeDepthOfFieldFocalLength(direction);
	}
	else if (IR_GetKeyState(DIK_T))
	{
		ChangeDepthOfFieldFocalDepth(direction);
	}
	else if (IR_GetKeyState(DIK_R))
	{
		ChangeDepthOfFieldFStop(direction);
	}
	else if (IR_GetKeyState(DIK_F))
	{
		ChangeFieldOfView(direction);
	}
}


