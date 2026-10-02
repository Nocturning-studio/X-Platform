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

struct STriangleVertex
{
	float x, y, z;
	float r, g, b, a;
};

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

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
	const int width = 1280;
	const int height = 720;

	Core.Initialize("X-Ray Dummy", "dummy", (LogCallback)0, TRUE, "game_filesystem.ltx", FALSE);

	WNDCLASSA wc{};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hIcon = LoadIconA(hInst, MAKEINTRESOURCEA(IDI_ICON1));
	wc.lpszClassName = "xrDummy";
	RegisterClassA(&wc);

	RECT rc{ 0, 0, width, height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hWnd = CreateWindowA("xrDummy", "X-Ray Test",
							  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
							  rc.right - rc.left, rc.bottom - rc.top,
							  nullptr, nullptr, hInst, nullptr);
	if (!hWnd)
		return 1;

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	CRenderBackend backend;

	RHI_PresentationParams params{};
	params.Windowed = TRUE;
	params.BackBufferCount = 2;
	params.SyncInterval = 1;
	params.FullscreenRefreshHz = 60;

	if (!backend.CreateDevice(hWnd, RHI_BackendType::DirectX9Ex, params))
		R_ERROR("! [Test] CreateDevice failed");

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

	RHI_BufferDesc ibDesc = RHI_BufferDesc::Index(sizeof(indices), RHI_IndexFormat::UInt16, RHI_BufferUsage_Immutable);
	ibDesc.debugName = "test.triangle.ib";
	ref_indexbuffer ib = backend.CreateIndexBuffer(ibDesc, indices);

	RHI_InputLayoutDesc layout;
	layout.elements.push_back({ 0, offsetof(STriangleVertex, x),
								RHI_VertexElementType::Float3, RHI_VertexElementSemantic::Position,
								0, RHI_VertexInputRate::PerVertex, 0 });
	layout.elements.push_back({ 0, offsetof(STriangleVertex, r),
								RHI_VertexElementType::Float4, RHI_VertexElementSemantic::Color,
								0, RHI_VertexInputRate::PerVertex, 0 });
	ref_vertexdecl vdecl = backend.CreateVertexDeclaration(layout);

	ref_geometry triangle = backend.CreateGeometry();
	triangle->SetVertexDeclaration(vdecl);
	triangle->SetVertexBuffer(0, vb, 0, 0);
	triangle->SetIndexBuffer(ib);
	triangle->SetTopology(RHI_Topology::TriangleList);

	CShaderPass pass;
	pass.SetVertexShader("test\\test.hlsl", "vs_main");
	pass.SetPixelShader("test\\test.hlsl", "ps_main");
	if (!pass.Compile(backend))
		R_ERROR("! [Test] shader compile failed");

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

		IRenderBackend* rhi = backend.GetRHI();
		if (!rhi)
			continue;

		backend.BeginFrame();

		const RHI_RenderTargetView bb = rhi->GetBackBufferRTV();
		const RHI_DepthStencilView ds = rhi->GetBackBufferDSV();

		RHI_RenderTargetView rtvs[1] = { bb };
		rhi->SetRenderTargets(rtvs, 1, ds);

		RHI_Viewport vp{ 0, 0, rhi->GetBackBufferWidth(), rhi->GetBackBufferHeight(), 0.0f, 1.0f };
		rhi->SetViewport(vp);

		rhi->ClearRenderTarget(bb, fvec4{ 0.05f, 0.1f, 0.5f, 1.0f });
		rhi->ClearDepthStencil(ds, 1.0f, 0);

		pass.Apply(backend);
		pass.ApplySamplers(backend);
		backend.DrawGeometry(triangle);

		backend.EndFrame();
		backend.Present();
	}

	pass.Release();
	triangle.Clear();
	backend.DestroyDevice();

	if (hWnd)
		DestroyWindow(hWnd);

	return 0;
}
////////////////////////////////////////////////////////////////////////////////
