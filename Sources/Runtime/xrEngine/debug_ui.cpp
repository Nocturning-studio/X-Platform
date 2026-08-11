///////////////////////////////////////////////////////////////
// Created: 18.01.2025
// Author: NS_NSDeathman
// User interface class
///////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "debug_ui.h"
///////////////////////////////////////////////////////////////
#include "imgui_api.h"
///////////////////////////////////////////////////////////////
#include "xr_input.h"
#include "../xrGame/xr_level_controller.h"
///////////////////////////////////////////////////////////////
CImguiAPI* ImguiAPI = nullptr;
///////////////////////////////////////////////////////////////
/*
void CDebugUI::Initialize()
{
	ImguiAPI = new CImguiAPI();
	ImguiAPI->Initialize();

	m_bNeedDraw = true;
}

void CDebugUI::Destroy()
{
	ImguiAPI->Destroy();
	delete ImguiAPI;
}

void CDebugUI::OnFrameBegin()
{
	ImguiAPI->OnFrameBegin();
}

void CDebugUI::OnFrame()
{

}

void CDebugUI::OnFrameEnd()
{
	ImguiAPI->RenderFrame();
}

void CDebugUI::DrawUI()
{
	ImGuiIO& InputOutputParams = ImGui::GetIO();
	InputOutputParams.MouseDrawCursor = false;

	//ImGui::PushFont(ImguiAPI->font_letterica_big);
	ImGui::Begin("ImGui Window");
	//ImGui::PopFont();

	//ImGui::PushFont(ImguiAPI->font_letterica_medium);

	ImGui::Text("1");

	//ImGui::Button("asdlfn");

	ImGui::SmallButton("123");

	//if (ImGui::Button("Test btn"))
	//	Sleep(0);

	//ImGui::PopFont();

	ImGui::End();
}

void CDebugUI::OnResetBegin()
{
	ImguiAPI->OnResetBegin();
}

void CDebugUI::OnResetEnd()
{
	ImguiAPI->OnResetEnd();
}

void CDebugUI::IR_OnKeyboardPress(int key)
{
	//Msg("Keyboard pressed");
}

void CDebugUI::IR_OnKeyboardRelease(int key)
{
	//Msg("Keyboard released");
}

void CDebugUI::IR_OnKeyboardHold(int key)
{
}

void CDebugUI::IR_OnMouseMove(int key)
{
}

void CDebugUI::IR_OnMouseWheel(int key)
{
}

void CDebugUI::IR_OnMouseHold(int key)
{
}

void CDebugUI::IR_OnMousePress(int key)
{
	ImGuiIO& InputOutputParams = ImGui::GetIO();

	switch (key)
	{
	case MOUSE_1:
		InputOutputParams.MouseDown[0] = true;
		break;
	case MOUSE_2:
		InputOutputParams.MouseDown[1] = true;
		break;
	case MOUSE_3:
		InputOutputParams.MouseDown[2] = true;
		break;
	}
}

void CDebugUI::IR_OnMouseRelease(int key)
{
	//Msg("Mouse released");

	ImGuiIO& InputOutputParams = ImGui::GetIO();

	switch (key)
	{
	case MOUSE_1:
		InputOutputParams.MouseDown[0] = false;
		break;
	case MOUSE_2:
		InputOutputParams.MouseDown[1] = false;
		break;
	case MOUSE_3:
		InputOutputParams.MouseDown[2] = false;
		break;
	}
}
*/
///////////////////////////////////////////////////////////////
