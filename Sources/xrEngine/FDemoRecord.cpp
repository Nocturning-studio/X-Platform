// CDemoRecord.cpp: implementation of the CDemoRecord class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "igame_level.h"
#include "x_ray.h"

#include "gamefont.h"
#include "fDemoRecord.h"
#include "xr_ioconsole.h"
#include "xr_input.h"
#include "xr_object.h"
#include "render.h"
#include "CustomHUD.h"
#include "IGame_Persistent.h"
#include "CameraManager.h"

#include "demo_common.h"
//////////////////////////////////////////////////////////////////////
#ifdef DEBUG
#define DEBUG_DEMO_RECORD
#endif
//////////////////////////////////////////////////////////////////////
CDemoRecord* xrDemoRecord = 0;
CDemoRecord::force_position CDemoRecord::g_position = {false, {0, 0, 0}};
//////////////////////////////////////////////////////////////////////
void CDemoRecord::update_whith_timescale(Fvector& v, const Fvector& v_delta)
{
	VERIFY(!fis_zero(Device.time_factor()));
	float scale = 1.f / Device.time_factor();
	v.mad(v, v_delta, scale);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CDemoRecord::CDemoRecord(const char* name, float life_time) : CEffectorCam(cefDemo, life_time)
{
	strconcat(sizeof(demo_file_name), demo_file_name, name, "");

	g_position.set_position = false;
	IR_Capture(); // capture input
	m_Camera.invert(Device.mView);

	// parse yaw
	Fvector& dir = m_Camera.k;
	Fvector DYaw;
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

	m_vVelocity.set(0, 0, 0);
	m_vAngularVelocity.set(0, 0, 0);
	iCount = 0;

	m_bGlobalHudDraw = psHUD_Flags.test(HUD_DRAW);
	psHUD_Flags.set(HUD_DRAW, false);

	m_bGlobalCrosshairDraw = psHUD_Flags.test(HUD_CROSSHAIR);
	psHUD_Flags.set(HUD_CROSSHAIR, false);

	m_vT.set(0, 0, 0);
	m_vR.set(0, 0, 0);
	m_bMakeCubeMap = FALSE;
	m_bMakeScreenshot = FALSE;
	m_bMakeLevelMap = FALSE;

	m_fSpeed0 = pSettings->r_float("demo_record", "speed0");
	m_fSpeed1 = pSettings->r_float("demo_record", "speed1");
	m_fSpeed2 = pSettings->r_float("demo_record", "speed2");
	m_fSpeed3 = pSettings->r_float("demo_record", "speed3");
	m_fAngSpeed0 = pSettings->r_float("demo_record", "ang_speed0");
	m_fAngSpeed1 = pSettings->r_float("demo_record", "ang_speed1");
	m_fAngSpeed2 = pSettings->r_float("demo_record", "ang_speed2");
	m_fAngSpeed3 = pSettings->r_float("demo_record", "ang_speed3");

	m_bNeedDisableInterpolation = false;

	m_bShowInputInfo = true;

	SetDefaultParameters();
}

CDemoRecord::~CDemoRecord()
{
	SaveAllFramesDataToIni(iCount);

	IR_Release(); // release input

	ResetParameters();

	psHUD_Flags.set(HUD_DRAW, m_bGlobalHudDraw);
	psHUD_Flags.set(HUD_CROSSHAIR, m_bGlobalCrosshairDraw);
	Close();
}

void CDemoRecord::Close()
{
	//g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
	fLifeTime = -1;
}

//								+X,				-X,				+Y,				-Y,			+Z,				-Z
Fvector CDemoRecord::cmNorm[6] = {{0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, -1.f},
							{0.f, 0.f, 1.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}};
Fvector CDemoRecord::cmDir[6] = {{1.f, 0.f, 0.f},  {-1.f, 0.f, 0.f}, {0.f, 1.f, 0.f},
						   {0.f, -1.f, 0.f}, {0.f, 0.f, 1.f},  {0.f, 0.f, -1.f}};

Flags32 CDemoRecord::s_hud_flag = {0};
Flags32 CDemoRecord::s_dev_flags = {0};

void CDemoRecord::MakeScreenshotFace()
{
	switch (m_Stage)
	{
	case 0:
		s_hud_flag.assign(psHUD_Flags);
		psHUD_Flags.assign(0);
		break;
	case 1:
		Render->Screenshot();
		psHUD_Flags.assign(s_hud_flag);
		m_bMakeScreenshot = FALSE;
		break;
	}
	m_Stage++;
}

INT g_bDR_LM_UsePointsBBox = 0;
INT g_bDR_LM_4Steps = 0;
INT g_iDR_LM_Step = 0;
Fvector g_DR_LM_Min, g_DR_LM_Max;

void GetLM_BBox(Fbox& bb, INT Step)
{
	float half_x = bb.min.x + (bb.max.x - bb.min.x) / 2;
	float half_z = bb.min.z + (bb.max.z - bb.min.z) / 2;
	switch (Step)
	{
	case 0: {
		bb.max.x = half_x;
		bb.min.z = half_z;
	}
	break;
	case 1: {
		bb.min.x = half_x;
		bb.min.z = half_z;
	}
	break;
	case 2: {
		bb.max.x = half_x;
		bb.max.z = half_z;
	}
	break;
	case 3: {
		bb.min.x = half_x;
		bb.max.z = half_z;
	}
	break;
	default: {
	}
	break;
	}
};

void CDemoRecord::MakeLevelMapProcess()
{
	switch (m_Stage)
	{
	case 0:
		s_dev_flags = psDeviceFlags;
		psDeviceFlags.zero();
		psDeviceFlags.set(rsClearBB | rsDrawStatic, TRUE);
		psDeviceFlags.set(rsFullscreen, s_dev_flags.test(rsFullscreen));
		break;
	case DEVICE_RESET_PRECACHE_FRAME_COUNT + 1: {
		s_hud_flag.assign(psHUD_Flags);
		psHUD_Flags.assign(0);

		Fbox bb = g_pGameLevel->ObjectSpace.GetBoundingVolume();

		if (g_bDR_LM_UsePointsBBox)
		{
			bb.max.x = g_DR_LM_Max.x;
			bb.max.z = g_DR_LM_Max.z;

			bb.min.x = g_DR_LM_Min.x;
			bb.min.z = g_DR_LM_Min.z;
		}
		if (g_bDR_LM_4Steps)
			GetLM_BBox(bb, g_iDR_LM_Step);
		// build camera matrix
		bb.getcenter(Device.vCameraPosition);

		Device.vCameraDirection.set(0.f, -1.f, 0.f);
		Device.vCameraTop.set(0.f, 0.f, 1.f);
		Device.vCameraRight.set(1.f, 0.f, 0.f);
		Device.mView.build_camera_dir(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);

		bb.xform(Device.mView);
		// build project matrix
		Device.mProject.build_projection_ortho(bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.min.z, bb.max.z);
	}
	break;
	case DEVICE_RESET_PRECACHE_FRAME_COUNT + 2: {
		string_path tmp;
		Fbox bb = g_pGameLevel->ObjectSpace.GetBoundingVolume();

		if (g_bDR_LM_UsePointsBBox)
		{
			bb.max.x = g_DR_LM_Max.x;
			bb.max.z = g_DR_LM_Max.z;

			bb.min.x = g_DR_LM_Min.x;
			bb.min.z = g_DR_LM_Min.z;
		}
		if (g_bDR_LM_4Steps)
			GetLM_BBox(bb, g_iDR_LM_Step);

		sprintf_s(tmp, sizeof(tmp), "%s_[%3.3f, %3.3f]-[%3.3f, %3.3f]", *g_pGameLevel->name(), bb.min.x, bb.min.z,
				  bb.max.x, bb.max.z);
		Render->Screenshot(IRender_interface::SM_FOR_LEVELMAP, tmp);
		psHUD_Flags.assign(s_hud_flag);
		psDeviceFlags = s_dev_flags;
		m_bMakeLevelMap = FALSE;
	}
	break;
	}
	m_Stage++;
}

void CDemoRecord::MakeCubeMapFace(Fvector& D, Fvector& N)
{
	Console->Execute("r_disable_postprocess on");

	string32 buf;
	switch (m_Stage)
	{
	case 0:
		N.set(cmNorm[m_Stage]);
		D.set(cmDir[m_Stage]);
		s_hud_flag.assign(psHUD_Flags);
		psHUD_Flags.assign(0);
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
		psHUD_Flags.assign(s_hud_flag);
		m_bMakeCubeMap = FALSE;
		break;
	}
	m_Stage++;

	Console->Execute("r_disable_postprocess off");
}

void CDemoRecord::SwitchShowInputInfo()
{
	if (m_bShowInputInfo == true)
	{
		m_bShowInputInfo = false;
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchShowInputInfo - method change m_bShowInputInfo to state enabled");
#endif
	}
	else
	{
		m_bShowInputInfo = true;
#ifdef DEBUG_DEMO_RECORD
		Msg("CDemoRecord::SwitchShowInputInfo - method change m_bShowInputInfo to state disabled");
#endif
	}
}

void CDemoRecord::ShowInfo()
{
	Engine.FontManager.GetSystemFont()->SetColor(color_rgba(215, 195, 170, 255));
	Engine.FontManager.GetSystemFont()->SetAligment(CGameFont::alLeft);
	Engine.FontManager.GetSystemFont()->OutSetI(-1.0, -1.0f);

	Engine.FontManager.GetSystemFont()->OutNext("Key frames count: %d", iCount);
	Engine.FontManager.GetSystemFont()->OutNext("DOF fStop: %f", g_fDOF.z);
	Engine.FontManager.GetSystemFont()->OutNext("DOF Focal depth: %f", g_fDOF.x);
	Engine.FontManager.GetSystemFont()->OutNext("DOF Focal length: %f", g_fDOF.y);
	Engine.FontManager.GetSystemFont()->OutNext("FOV: %f", g_fFov);
}

void CDemoRecord::ShowInputInfo()
{
	Engine.FontManager.GetSystemFont()->SetColor(color_rgba(215, 195, 170, 255));

	Engine.FontManager.GetSystemFont()->SetAligment(CGameFont::alLeft);
	Engine.FontManager.GetSystemFont()->OutSetI(-0.2f, -.25f);
	Engine.FontManager.GetSystemFont()->OutNext("TAB");
	Engine.FontManager.GetSystemFont()->OutNext("SPACE");
	Engine.FontManager.GetSystemFont()->OutNext("LEFT ALT");
	Engine.FontManager.GetSystemFont()->OutNext("BACKSPACE");
	Engine.FontManager.GetSystemFont()->OutNext("NUMPAD MINUS");
	Engine.FontManager.GetSystemFont()->OutNext("ESC");
	Engine.FontManager.GetSystemFont()->OutNext("F11");
	Engine.FontManager.GetSystemFont()->OutNext("F12");
	Engine.FontManager.GetSystemFont()->OutNext("G + Mouse Wheel");
	Engine.FontManager.GetSystemFont()->OutNext("T + Mouse Wheel");
	Engine.FontManager.GetSystemFont()->OutNext("R + Mouse Wheel");
	Engine.FontManager.GetSystemFont()->OutNext("F + Mouse Wheel");
	Engine.FontManager.GetSystemFont()->OutNext("H");
	Engine.FontManager.GetSystemFont()->OutNext("V");
	Engine.FontManager.GetSystemFont()->OutNext("B");

#ifndef MASTER_GOLD
	Engine.FontManager.GetSystemFont()->OutNext("1, 2, ..., 0");
	Engine.FontManager.GetSystemFont()->OutNext("ENTER");
#endif

	Engine.FontManager.GetSystemFont()->SetAligment(CGameFont::alLeft);
	Engine.FontManager.GetSystemFont()->OutSetI(0, -.25f);
	Engine.FontManager.GetSystemFont()->OutNext("= Draw this help");
	Engine.FontManager.GetSystemFont()->OutNext("= Append Key With Interpolation");
	Engine.FontManager.GetSystemFont()->OutNext("= Append Key Without Interpolation");
	Engine.FontManager.GetSystemFont()->OutNext("= Delete key");
	Engine.FontManager.GetSystemFont()->OutNext("= Cube Map");
	Engine.FontManager.GetSystemFont()->OutNext("= Quit");
	Engine.FontManager.GetSystemFont()->OutNext("= Level Map ScreenShot");
	Engine.FontManager.GetSystemFont()->OutNext("= ScreenShot");
	Engine.FontManager.GetSystemFont()->OutNext("= Depth of field Focal length");
	Engine.FontManager.GetSystemFont()->OutNext("= Depth of field Focal depth");
	Engine.FontManager.GetSystemFont()->OutNext("= Depth of field FStop");
	Engine.FontManager.GetSystemFont()->OutNext("= Field of view");
	Engine.FontManager.GetSystemFont()->OutNext("= Autofocus");
	Engine.FontManager.GetSystemFont()->OutNext("= Grid");
	Engine.FontManager.GetSystemFont()->OutNext("= Cinema borders");

#ifndef MASTER_GOLD
	Engine.FontManager.GetSystemFont()->OutNext("= Render debugging");
	Engine.FontManager.GetSystemFont()->OutNext("= Actor teleportation");
#endif
}

void CDemoRecord::Screenshot(SCamEffectorInfo& info)
{
	MakeScreenshotFace();
	// update camera
	info.n.set(m_Camera.j);
	info.d.set(m_Camera.k);
	info.p.set(m_Camera.c);
	info.fFov = g_fFov;
}

void CDemoRecord::MakeCubemap(SCamEffectorInfo& info)
{
	info.fFov = 90.0f;
	MakeCubeMapFace(info.d, info.n);
	info.p.set(m_Camera.c);
	info.fAspect = 1.f;
}

void CDemoRecord::Update(SCamEffectorInfo& info)
{
	ShowInfo();

	if (m_bShowInputInfo == true)
		ShowInputInfo();

	m_vVelocity.lerp(m_vVelocity, m_vT, 0.3f);
	m_vAngularVelocity.lerp(m_vAngularVelocity, m_vR, 0.3f);

	float speed = m_fSpeed1;
	float ang_speed = m_fAngSpeed1;

	if (IR_GetKeyState(DIK_LSHIFT))
	{
		speed = m_fSpeed0;
		ang_speed = m_fAngSpeed0;
	}
	else if (IR_GetKeyState(DIK_Z))
	{
		speed = m_fSpeed0 * 0.5f;
		ang_speed = m_fAngSpeed0 * 0.5f;
	}
	else if (IR_GetKeyState(DIK_LCONTROL))
	{
		speed = m_fSpeed3;
		ang_speed = m_fAngSpeed3;
	}

	m_vT.mul(m_vVelocity, Device.fTimeDelta * speed);
	m_vR.mul(m_vAngularVelocity, Device.fTimeDelta * ang_speed);

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

	Fvector vmove;
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

	m_Camera.setHPB(m_HPB.x, m_HPB.y, m_HPB.z);
	m_Camera.translate_over(m_Position);

	info.n.set(m_Camera.j);
	info.d.set(m_Camera.k);
	info.p.set(m_Camera.c);

	fLifeTime -= Device.fTimeDelta;

	m_vT.set(0, 0, 0);
	m_vR.set(0, 0, 0);

	info.fFov = g_fFov;

	double x = 43.266615300557;
	g_fDOF.y = (x / (2 * tan(M_PI * g_fFov / 360.f)));
	g_pGamePersistent->SetBaseDof(g_fDOF);
}

BOOL CDemoRecord::ProcessCam(SCamEffectorInfo& info)
{
	if (m_bMakeScreenshot)
		Screenshot(info);
	else if (m_bMakeLevelMap)
		MakeLevelMapProcess();
	else if (m_bMakeCubeMap)
		MakeCubemap(info);
	else
		Update(info);

	return TRUE;
}

void CDemoRecord::SwitchAutofocusState()
{
	if (g_bAutofocusEnabled == false)
	{
		g_bAutofocusEnabled = true;
	}
	else
	{
		g_bAutofocusEnabled = false;
	}

	g_pGamePersistent->SetPickableEffectorDOF(g_bAutofocusEnabled);
}

// Решение действовать через консоль чудовищное, в будущем нужно заменить смену флага через консоль на смену через флаг
// общих команд для всех рендеров
void CDemoRecord::SwitchGridState()
{
	if (g_bGridEnabled == false)
	{
		g_bGridEnabled = true;
		Console->Execute("r_photo_grid on");
	}
	else
	{
		g_bGridEnabled = false;
		Console->Execute("r_photo_grid off");
	}
}

// Решение действовать через консоль чудовищное, в будущем нужно заменить смену флага через консоль на смену через флаг
// общих команд для всех рендеров
void CDemoRecord::SwitchCinemaBordersState()
{
	if (g_bBordersEnabled == false)
	{
		g_bBordersEnabled = true;
		Console->Execute("r_cinema_borders on");
	}
	else
	{
		g_bBordersEnabled = false;
		Console->Execute("r_cinema_borders off");
	}
}

void CDemoRecord::SwitchWatermarkVisibility()
{
	if (g_bWatermarkEnabled == false)
	{
		g_bWatermarkEnabled = true;
		Console->Execute("r_watermark on");
	}
	else
	{
		g_bWatermarkEnabled = false;
		Console->Execute("r_watermark off");
	}
}

void CDemoRecord::IR_OnKeyboardPress(int dik)
{
	if (dik == DIK_ESCAPE)
		Close();

	if (dik == DIK_GRAVE)
		Console->Show();

	if (dik == DIK_SPACE)
		RecordKey(LINEAR_INTERPOLATION_TYPE);

	if ((dik == DIK_LALT))
		RecordKey(DISABLE_INTERPOLATION);

	if (dik == DIK_MINUS)
		SetNeedMakeCubemap();

	if (dik == DIK_BACK)
		DeleteKey();

	if (dik == DIK_F11)
		MakeLevelMapScreenshot();

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

#ifndef MASTER_GOLD
#pragma todo("NSDeathman to all: Переделать быструю отладку рендера под удобный вид")
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

void CDemoRecord::IR_OnKeyboardHold(int dik)
{
	Fvector vT_delta{}, vR_delta{};
	switch (dik)
	{
	case DIK_A:
	case DIK_NUMPAD1:
	case DIK_LEFT:
		vT_delta.x -= 1.0f;
		break; // Slide Left
	case DIK_D:
	case DIK_NUMPAD3:
	case DIK_RIGHT:
		vT_delta.x += 1.0f;
		break; // Slide Right
	case DIK_S:
		vT_delta.y -= 1.0f;
		break; // Slide Down
	case DIK_W:
		vT_delta.y += 1.0f;
		break; // Slide Up
	// rotate
	case DIK_NUMPAD2:
		vR_delta.x -= 1.0f;
		break; // Pitch Down
	case DIK_NUMPAD8:
		vR_delta.x += 1.0f;
		break; // Pitch Up
	case DIK_E:
	case DIK_NUMPAD6:
		vR_delta.y += 1.0f;
		break; // Turn Left
	case DIK_Q:
	case DIK_NUMPAD4:
		vR_delta.y -= 1.0f;
		break; // Turn Right
	case DIK_NUMPAD9:
		vR_delta.z -= 2.0f;
		break; // Turn Right
	case DIK_NUMPAD7:
		vR_delta.z += 2.0f;
		break; // Turn Right
	case DIK_LWIN:
		m_bNeedDisableInterpolation = true;
	}
	update_whith_timescale(m_vT, vT_delta);
	update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnMouseMove(int dx, int dy)
{
	float scale = .5f; // psMouseSens;
	Fvector vR_delta{};
	if (dx || dy)
	{
		vR_delta.y += float(dx) * scale;												// heading
		vR_delta.x += ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * (3.f / 4.f); // pitch
	}
	update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnMouseHold(int btn)
{
	Fvector vT_delta{};
	switch (btn)
	{
	case 0:
		vT_delta.z += 1.0f;
		break; // Move Backward
	case 1:
		vT_delta.z -= 1.0f;
		break; // Move Forward
	}
	update_whith_timescale(m_vT, vT_delta);
}

void CDemoRecord::ChangeDepthOfFieldFocalDepth(int direction)
{
	Fvector3 dof_params_old;
	Fvector3 dof_params_actual;

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

	g_fDOF = dof_params_actual;
	g_pGamePersistent->SetBaseDof(g_fDOF);
}

void CDemoRecord::ChangeDepthOfFieldFocalLength(int direction)
{
	Fvector3 dof_params_old;
	Fvector3 dof_params_actual;

	g_pGamePersistent->GetCurrentDof(dof_params_old);

	dof_params_actual = dof_params_old;

	float X = dof_params_old.y * 0.25f;

	if (direction > 0)
		dof_params_actual.y = dof_params_old.y + X;
	else
		dof_params_actual.y = dof_params_old.y - X;

	//dof_params_actual.y = dof_params_actual.x + 10.0f;

	// if (dof_params_actual.x <= 0.1f)
	//	dof_params_actual.x = 0.1f;

	g_fDOF = dof_params_actual;
	g_pGamePersistent->SetBaseDof(g_fDOF);
}

void CDemoRecord::ChangeDepthOfFieldFStop(int direction)
{
	Fvector3 dof_params_old;
	Fvector3 dof_params_actual;

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

	g_fDOF = dof_params_actual;
	g_pGamePersistent->SetBaseDof(g_fDOF);
}

void CDemoRecord::ChangeFieldOfView(int direction)
{
	float g_fFov_actual = Device.fFOV;

	float X = g_fFov_actual * 0.05f;

	if (direction > 0)
		g_fFov = g_fFov_actual + X;
	else
		g_fFov = g_fFov_actual - X;

	if (g_fFov <= 2.28f)
		g_fFov = 2.28f;
	else if (g_fFov >= 113.001f)
		g_fFov = 113.0f;
}

void CDemoRecord::IR_OnMouseWheel(int direction)
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

void CDemoRecord::DeleteKey()
{
	if (iCount == 0)
		return;

	iCount = iCount - 1;

	Msg("iCount = %d", iCount);

	g_fDOF = FramesArray[iCount].DOF;
	g_pGamePersistent->SetBaseDof(g_fDOF);

	g_fFov = FramesArray[iCount].Fov;

	g_bBordersEnabled = FramesArray[iCount].UseCinemaBorders;
	g_bWatermarkEnabled = FramesArray[iCount].UseWatermark;

	m_HPB.set(FramesArray[iCount].HPB);
	m_Position.set(FramesArray[iCount].Position);
}

void CDemoRecord::RecordKey(u32 IterpolationType)
{
	FramesArray[iCount].HPB.set(m_HPB);
	FramesArray[iCount].Position.set(m_Position);
	FramesArray[iCount].InterpolationType = IterpolationType;

	FramesArray[iCount].DOF = g_fDOF;

	FramesArray[iCount].Fov = g_fFov;

	FramesArray[iCount].UseCinemaBorders = g_bBordersEnabled;
	FramesArray[iCount].UseWatermark = g_bWatermarkEnabled;

	iCount++;
}

void CDemoRecord::SetNeedMakeCubemap()
{
	m_bMakeCubeMap = TRUE;
	m_Stage = 0;
}

void CDemoRecord::MakeScreenshot()
{
	m_bMakeScreenshot = TRUE;
	m_Stage = 0;
}

void CDemoRecord::MakeLevelMapScreenshot()
{
	m_bMakeLevelMap = TRUE;
	m_Stage = 0;
}
