///////////////////////////////////////////////////////////////
// Created: 16.01.2025
// Author: NS_NSDeathman
// ImGui implementation
///////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "imgui_api.h"
///////////////////////////////////////////////////////////////
void CImguiAPI::Initialize()
{
	Msg("Initializing ImGuiAPI...");

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	m_pImGuiInputOutputParams = ImGui::GetIO();
	(void)m_pImGuiInputOutputParams;
	m_pImGuiInputOutputParams.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	m_pImGuiInputOutputParams.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;	 // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(Engine.WindowManager.GetHandle());
	ImGui_ImplDX9_Init(HW.pDevice);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use
	// ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your
	// application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
	// ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double
	// backslash \\ !
	// io.Fonts->AddFontDefault();
	// io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	// ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr,
	// io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != nullptr);

	string_path LettericaPath;
	FS.update_path(LettericaPath, "$fonts$", "letterica-3.ttf");

	font_letterica = m_pImGuiInputOutputParams.Fonts->AddFontFromFileTTF(LettericaPath, 14.0f);
	font_letterica_small = m_pImGuiInputOutputParams.Fonts->AddFontFromFileTTF(LettericaPath, 10.0f);
	font_letterica_medium = m_pImGuiInputOutputParams.Fonts->AddFontFromFileTTF(LettericaPath, 14.0f);
	font_letterica_big = m_pImGuiInputOutputParams.Fonts->AddFontFromFileTTF(LettericaPath, 18.0f);

	string_path MavenProBlackPath;
	FS.update_path(MavenProBlackPath, "$fonts$", "MavenPro-Black.ttf");
	font_maven_pro_back = m_pImGuiInputOutputParams.Fonts->AddFontFromFileTTF(MavenProBlackPath, 14.0f);

	string_path MavenProBoldPath;
	FS.update_path(MavenProBoldPath, "$fonts$", "MavenPro-Bold.ttf");
	font_maven_pro_bold = m_pImGuiInputOutputParams.Fonts->AddFontFromFileTTF(MavenProBoldPath, 14.0f);

	string_path MavenProMediumPath;
	FS.update_path(MavenProMediumPath, "$fonts$", "MavenPro-Medium.ttf");
	font_maven_pro_medium = m_pImGuiInputOutputParams.Fonts->AddFontFromFileTTF(MavenProMediumPath, 14.0f);

	string_path MavenProRegularPath;
	FS.update_path(MavenProRegularPath, "$fonts$", "MavenPro-Regular.ttf");
	font_maven_pro_regular = m_pImGuiInputOutputParams.Fonts->AddFontFromFileTTF(MavenProRegularPath, 14.0f);
}

void CImguiAPI::OnFrameBegin()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void CImguiAPI::RenderFrame()
{
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void CImguiAPI::OnFrameEnd()
{
}

void CImguiAPI::OnResetBegin()
{
	Msg("Invalidating ImGuiAPI Device objects...");
	ImGui_ImplDX9_InvalidateDeviceObjects();
}

void CImguiAPI::OnResetEnd()
{
	Msg("Creating ImGuiAPI Device objects...");
	ImGui_ImplDX9_CreateDeviceObjects();
}

void CImguiAPI::Destroy()
{
	Msg("Destroying ImGuiAPI...");
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
///////////////////////////////////////////////////////////////
