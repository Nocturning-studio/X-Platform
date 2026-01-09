///////////////////////////////////////////////////////////////
// Created: 18.01.2025
// Author: NS_NSDeathman
// User interface class
///////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////
#include "stdafx.h"
///////////////////////////////////////////////////////////////
class ENGINE_API CDebugUI
{
  private:
	bool m_bNeedDraw;

  public:
	CDebugUI() = default;
	~CDebugUI() = default;

	void Initialize(){};
	void OnFrameBegin(){};
	void OnFrame(){};
	void OnFrameEnd(){};
	void DrawUI(){};
	void OnResetBegin(){};
	void OnResetEnd(){};
	void IR_OnKeyboardPress(int key){};
	void IR_OnKeyboardRelease(int key){};
	void IR_OnKeyboardHold(int key){};
	void IR_OnMouseMove(int key){};
	void IR_OnMouseWheel(int key){};
	void IR_OnMouseHold(int key){};
	void IR_OnMousePress(int key){};
	void IR_OnMouseRelease(int key){};
	void Destroy(){};
};
///////////////////////////////////////////////////////////////
