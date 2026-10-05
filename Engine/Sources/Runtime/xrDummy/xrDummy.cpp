////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "resource.h"
////////////////////////////////////////////////////////////////////////////////
#include <xrMath/xrMath.h>
#include <xrCore/xrCore.h>
#include <xrRHI/xrRHI.h>
#include <xrRenderBackend/xrRenderBackend.h>
#include <Runtime/xrDummy/resource.h>
////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// Vertex layout для тестового треугольника
// ============================================================================
struct STriangleVertex
{
	float x, y, z;
	float r, g, b, a;
};

// ============================================================================
// Размеры
// ============================================================================
static constexpr uint32_t kWindowWidth = 1280;
static constexpr uint32_t kWindowHeight = 720;
static constexpr uint32_t kOffscreenWidth = 512;
static constexpr uint32_t kOffscreenHeight = 512;

////////////////////////////////////////////////////////////////////////////////
// WndProc
////////////////////////////////////////////////////////////////////////////////

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcA(hWnd, msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// WinMain
////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
	Core.Initialize("X-Ray Dummy", "dummy", (LogCallback)0, TRUE, "game_filesystem.ltx", FALSE);

	// ========================================================================
	// Окно
	// ========================================================================
	WNDCLASSA wc{};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hIcon = LoadIconA(hInst, MAKEINTRESOURCEA(IDI_ICON1));
	wc.lpszClassName = "xrDummy";
	RegisterClassA(&wc);

	RECT rc{ 0, 0, (LONG)kWindowWidth, (LONG)kWindowHeight };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hWnd = CreateWindowA("xrDummy", "X-Ray Fullscreen Test",
							  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
							  rc.right - rc.left, rc.bottom - rc.top,
							  nullptr, nullptr, hInst, nullptr);
	if (!hWnd)
		return 1;

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// ========================================================================
	// Backend
	// ========================================================================
	CRenderBackend backend;

	RHI_PresentationParams params{};
	params.Windowed = TRUE;
	params.BackBufferCount = 2;
	params.SyncInterval = 0;
	params.FullscreenRefreshHz = 0;

	if (!backend.CreateDevice(hWnd, RHI_BackendType::DirectX9Ex, params))
	{
		R_ERROR("! [Test] CreateDevice failed");
		return 1;
	}

	IRenderBackend* rhi = backend.GetRHI();
	if (!rhi)
	{
		R_ERROR("! [Test] backend.GetRHI() returned null");
		return 1;
	}

	// ========================================================================
	// Offscreen render target (512x512), куда будем рисовать треугольник
	// ========================================================================
	ref_texture offscreen = backend.CreateRenderTarget(kOffscreenWidth, kOffscreenHeight, RHI_Format::RGBA8_UNORM);
	if (!offscreen)
	{
		R_ERROR("! [Test] CreateRenderTarget failed");
		return 1;
	}

	const RHI_RenderTargetView offscreenRTV = backend.CreateRTV(offscreen->GetRHIHandle());
	if (!offscreenRTV.IsValid())
	{
		R_ERROR("! [Test] CreateRTV failed");
		return 1;
	}

	// ========================================================================
	// Геометрия треугольника
	// ========================================================================
	const STriangleVertex verts[] =
	{
		{ 0.0f,  0.6f, 0.5f, 1, 0, 0, 1 },
		{ 0.5f, -0.4f, 0.5f, 0, 1, 0, 1 },
		{-0.5f, -0.4f, 0.5f, 0, 0, 1, 1 },
	};
	const uint16_t indices[] = { 0, 1, 2 };

	RHI_BufferDesc vbDesc = RHI_BufferDesc::Vertex(sizeof(verts), sizeof(STriangleVertex), RHI_BufferUsage_Immutable);
	vbDesc.debugName = "test.triangle.vb";
	ref_vertexbuffer vb = backend.CreateVertexBuffer(vbDesc, verts);
	if (!vb)
	{
		R_ERROR("! [Test] CreateVertexBuffer failed");
		return 1;
	}

	RHI_BufferDesc ibDesc = RHI_BufferDesc::Index(sizeof(indices), RHI_IndexFormat::UInt16, RHI_BufferUsage_Immutable);
	ibDesc.debugName = "test.triangle.ib";
	ref_indexbuffer ib = backend.CreateIndexBuffer(ibDesc, indices);
	if (!ib)
	{
		R_ERROR("! [Test] CreateIndexBuffer failed");
		return 1;
	}

	RHI_InputLayoutDesc layout;
	layout.elements.push_back({ 0, offsetof(STriangleVertex, x),
								RHI_VertexElementType::Float3, RHI_VertexElementSemantic::Position,
								0, RHI_VertexInputRate::PerVertex, 0 });
	layout.elements.push_back({ 0, offsetof(STriangleVertex, r),
								RHI_VertexElementType::Float4, RHI_VertexElementSemantic::Color,
								0, RHI_VertexInputRate::PerVertex, 0 });

	ref_vertexdecl vdecl = backend.CreateVertexDeclaration(layout);
	if (!vdecl)
	{
		R_ERROR("! [Test] CreateVertexDeclaration failed");
		return 1;
	}

	ref_geometry triangle = backend.CreateGeometry();
	if (!triangle)
	{
		R_ERROR("! [Test] CreateGeometry failed");
		return 1;
	}
	triangle->SetVertexDeclaration(vdecl);
	triangle->SetVertexBuffer(0, vb, 0, 0);
	triangle->SetIndexBuffer(ib);
	triangle->SetTopology(RHI_Topology::TriangleList);

	// ========================================================================
	// Pass 1: цветной треугольник
	// ========================================================================
	CShaderPass passTriangle;
	passTriangle.SetVertexShader("test\\test.hlsl", "vs_triangle");
	passTriangle.SetPixelShader("test\\test.hlsl", "ps_triangle");
	if (!passTriangle.Compile(backend))
	{
		R_ERROR("! [Test] passTriangle compile failed");
		return 1;
	}

	// ========================================================================
	// Pass 2: fullscreen-копирование
	// ========================================================================
	CShaderPass passFullscreen;
	passFullscreen.SetVertexShader("test\\test.hlsl", "vs_fullscreen");
	passFullscreen.SetPixelShader("test\\test.hlsl", "ps_fullscreen");
	if (!passFullscreen.Compile(backend))
	{
		R_ERROR("! [Test] passFullscreen compile failed");
		return 1;
	}

	if (!passFullscreen.SetTexture("s_src", offscreen))
	{
		R_ERROR("! [Test] passFullscreen: sampler 's_src' not found " "(check HLSL declaration)");
		return 1;
	}

	Msg("* [Test] Init OK - window=%ux%u, offscreen=%ux%u", kWindowWidth, kWindowHeight, kOffscreenWidth, kOffscreenHeight);

	// ========================================================================
	// Main loop
	// ========================================================================
	MSG msg{};
	bool running = true;
	while (running)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (!running)
			break;

		if (!backend.IsReady())
			continue;

		backend.BeginFrame();

		// ------------------------------------------------------------
		// Pass 1: треугольник → offscreen RT
		// ------------------------------------------------------------
		{
			RHI_RenderTargetView rtvs[1] = { offscreenRTV };
			rhi->SetRenderTargets(rtvs, 1, RHI_DepthStencilView{});

			RHI_Viewport vp{ 0, 0, kOffscreenWidth, kOffscreenHeight, 0.0f, 1.0f };
			rhi->SetViewport(vp);

			rhi->ClearRenderTarget(offscreenRTV, fvec4{ 0.05f, 0.05f, 0.4f, 1.0f });

			passTriangle.Apply(backend);
			passTriangle.ApplySamplers(backend);
			backend.DrawGeometry(triangle);
		}

		// ------------------------------------------------------------
		// Pass 2: fullscreen copy → back buffer
		// ------------------------------------------------------------
		{
			const RHI_RenderTargetView bb = rhi->GetBackBufferRTV();
			const RHI_DepthStencilView ds = rhi->GetBackBufferDSV();

			RHI_RenderTargetView rtvs[1] = { bb };
			rhi->SetRenderTargets(rtvs, 1, ds);

			RHI_Viewport vp{ 0, 0, rhi->GetBackBufferWidth(), rhi->GetBackBufferHeight(), 0.0f, 1.0f };
			rhi->SetViewport(vp);

			rhi->ClearRenderTarget(bb, fvec4{ 0.02f, 0.02f, 0.02f, 1.0f });
			rhi->ClearDepthStencil(ds, 1.0f, 0);

			passFullscreen.Apply(backend);
			passFullscreen.ApplySamplers(backend);

			backend.DrawFullscreen();
		}

		backend.EndFrame();
		backend.Present();
	}

	// ========================================================================
	// Shutdown — строгий порядок
	// ========================================================================

	passTriangle.Release();
	passFullscreen.Release();

	backend.DestroyRTV(offscreenRTV);

	triangle.Clear();

	offscreen.Clear();
	vdecl.Clear();
	ib.Clear();
	vb.Clear();

	backend.DestroyDevice();

	if (hWnd)
		DestroyWindow(hWnd);

	return 0;
}
////////////////////////////////////////////////////////////////////////////////
