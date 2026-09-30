////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
////////////////////////////////////////////////////////////////////////////////
#include <xrMath/xrMath.h>
#include <xrCore/xrCore.h>
#include <xrRHI/xrRHI.h>
#include <xrRenderBackend/xrRenderBackend.h>
////////////////////////////////////////////////////////////////////////////////
namespace
{
LRESULT CALLBACK TestWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND CreateTestWindow(HINSTANCE hInst, int w, int h)
{
	WNDCLASSA wc{};
	wc.lpfnWndProc = TestWndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = "xrDummy";
	RegisterClassA(&wc);

	RECT rc{0, 0, w, h};
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	return CreateWindowA(
		"xrDummy", "X-Ray Test",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		nullptr, nullptr, hInst, nullptr);
}
} // namespace

struct STestVertex
{
	float x, y, z;
	float r, g, b, a;
};

class CBackendTest
{
  public:
	bool Init(HINSTANCE hInst, int width, int height);
	void Frame();
	void Shutdown();

  private:
	bool CreateTriangle();
	bool HandleDeviceLost();

	HWND m_hWnd = nullptr;
	CRenderBackend m_backend;

	ref_geometry m_triangle;
	CShaderPass m_shader;
	bool m_shaderReady = false;
};

bool CBackendTest::Init(HINSTANCE hInst, int width, int height)
{
	m_hWnd = CreateTestWindow(hInst, width, height);
	if(!m_hWnd)
		return false;
	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);

	RHI_PresentationParams params{};
	params.Windowed = TRUE;
	params.BackBufferCount = 2;
	params.SyncInterval = 1;
	params.FullscreenRefreshHz = 60;

	if(!m_backend.CreateDevice(m_hWnd, RHI_BackendType::DirectX9, params))
	{
		R_ASSERT2(false, "! [Test] CreateDevice failed");
	}

	m_shader.SetVertexShader("test\\test.hlsl", "vs_main");
	m_shader.SetPixelShader("test\\test.hlsl", "ps_main");
	if(!m_shader.Compile(m_backend))
		R_ASSERT2(false, "! [Test] shader compile failed");
	m_shaderReady = true;

	if(!CreateTriangle())
		return false;

	Msg("* [Test] Init OK - %ux%u", width, height);
	return true;
}

bool CBackendTest::CreateTriangle()
{
	const STestVertex verts[] =
		{
			{0.0f, 0.6f, 0.5f, 1, 0, 0, 1},
			{0.5f, -0.4f, 0.5f, 0, 1, 0, 1},
			{-0.5f, -0.4f, 0.5f, 0, 0, 1, 1},
		};

	CVertexBufferDesc vbDesc{};
	vbDesc.sizeBytes = sizeof(verts);
	vbDesc.stride = sizeof(STestVertex);
	vbDesc.usage = BufferUsage_Immutable;
	vbDesc.debugName = "test.triangle.vb";

	ref_vertexbuffer vb = m_backend.CreateVertexBuffer(vbDesc, verts);
	R_ASSERT2(vb, "! [Test] CreateVertexBuffer failed");

	const uint16_t indices[] = {0, 1, 2};
	CIndexBufferDesc ibDesc{};
	ibDesc.sizeBytes = sizeof(indices);
	ibDesc.format = EIndexFormat::UInt16;
	ibDesc.usage = BufferUsage_Immutable;
	ibDesc.debugName = "test.triangle.ib";

	ref_indexbuffer ib = m_backend.CreateIndexBuffer(ibDesc, indices);
	R_ASSERT2(ib, "! [Test] CreateIndexBuffer failed");

	// --- vertex layout: POSITION.xyz + COLOR.rgba ---
	CVertexLayoutDesc layout;
	layout.elements.push_back({0, offsetof(STestVertex, x),
							   EVertexElementType::Float3, EVertexElementSemantic::Position,
							   0, EVertexInputRate::PerVertex, 0});
	layout.elements.push_back({0, offsetof(STestVertex, r),
							   EVertexElementType::Float4, EVertexElementSemantic::Color,
							   0, EVertexInputRate::PerVertex, 0});

	ref_vertexdecl vdecl = m_backend.CreateVertexDeclaration(layout);
	R_ASSERT2(vdecl, "! [Test] CreateVertexDeclaration failed");

	// --- geometry ---
	m_triangle = m_backend.CreateGeometry();
	R_ASSERT2(m_triangle, "! [Test] CreateGeometry failed");

	m_triangle->SetVertexDeclaration(vdecl);
	m_triangle->SetVertexBuffer(0, vb, 0, 0);
	m_triangle->SetIndexBuffer(ib);
	m_triangle->SetTopology(EPrimitiveTopology::TriangleList);

	Msg("* [Test] Triangle ready (vb=%u B, ib=%u idx)", vbDesc.sizeBytes, (uint32_t)(sizeof(indices) / sizeof(indices[0])));
	return true;
}

bool CBackendTest::HandleDeviceLost()
{
	if(!m_backend.NeedReset())
		return true;

	Msg("* [Test] device lost → need reset");
	m_backend.OnDeviceLost();
	m_shader.OnDeviceLost();

	m_shaderReady = false;
	return false;
}

void CBackendTest::Frame()
{
	if(!m_backend.IsReady())
	{
		if(!HandleDeviceLost())
			return;
	}

	m_backend.BeginFrame();

	m_backend.Clear(D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF102030, 1.0f, 0);

	if(m_shaderReady && m_triangle._get())
	{
		m_shader.Apply(m_backend);		   // SetVertexShader / SetPixelShader
		m_shader.ApplySamplers(m_backend); // SetTexture + sampler states
		m_backend.DrawGeometry(m_triangle);
	}

	m_backend.EndFrame();
	m_backend.Present();
}

void CBackendTest::Shutdown()
{
	// Даём менеджеру ресурсов «догнать» отложенные удаления.
	for(int i = 0; i < 3; ++i)
		m_backend.Resources().OnFrameEnd();

	m_triangle.Clear();
	m_shader.OnDeviceLost();

	m_backend.DestroyDevice();

	if(m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
}

//------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
	Core.Initialize("X-Ray Dummy", "dummy", (LogCallback)0, TRUE, "game_filesystem.ltx", FALSE);

	CBackendTest test;
	if(!test.Init(hInst, 1280, 720))
	{
		test.Shutdown();
		return 1;
	}

	MSG msg{};
	bool running = true;
	while(running)
	{
		while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(running)
			test.Frame();
	}

	test.Shutdown();
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
