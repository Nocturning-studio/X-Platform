////////////////////////////////////////////////////////////////////////////////
// Created: 22.10.2026 22:57:00
// Author: NS_Deathman
// File: LoadingScreen.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrFramework/ILoadingScreen.h>
////////////////////////////////////////////////////////////////////////////////
class CLoadingScreen final : public ILoadingScreen
{
  public:
	CLoadingScreen();
	~CLoadingScreen() override;

	// ILoadingScreen
	void Show() override;
	void Hide() override;
	void ForceRender() override;
	void SetTitle(const char*) override;
	void UpdateLevelLogo() override;
	void Destroy() override;

  private:
	void InitializeFont();
	u32 CalcProgressColor(u32 idx, u32 total, int stage, int max_stage);
	void DrawInternal();

	// --- render resources ---
	ref_shader hLevelLogo;
	ref_shader sh_progress;
	ref_geom ll_hGeom;
	ref_geom ll_hGeom2;

	// --- font / text ---
	CGameFont* pFontSystem;
	string256 app_title;

	// --- state ---
	bool bIsActive; // (не используется, оставлено для совместимости)
	int load_stage;
	int max_load_stage;
	u32 ll_dwReference; // рефкаунтер вложенных Show/Hide

	CTimer phase_timer;
};
////////////////////////////////////////////////////////////////////////////////
