////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
////////////////////////////////////////////////////////////////////////////////
#include <xrMath/xrMath.h>
#include <xrCore/xrCore.h>
#include <xrRHI/xrRHI.h>
#include <xrRenderBackend/xrRenderBackend.h>
////////////////////////////////////////////////////////////////////////////////

struct STriangleVertex
{
	float x, y, z;
	float r, g, b, a;
};

struct SQuadVertex
{
	float x, y, z;
	float u, v;
};

class CBackendTest
{
  public:
	bool Init(HINSTANCE hInst, int width, int height);
	void Frame();
	void Shutdown();

  private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if(msg == WM_DESTROY)
		{
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProcA(hWnd, msg, wParam, lParam);
	}

	bool CreateOffscreenTargets();
	bool CreateTriangleGeometry();
	bool CreateQuadGeometry();

	void DrawPass1_Offscreen(IRenderBackend& rhi);
	void DrawPass2_Display(IRenderBackend& rhi);

	HWND m_hWnd = nullptr;
	CRenderBackend m_backend;

	// --- Offscreen render targets ---
	// Текстуры держим как ref_texture — так их можно передать в CShaderPass.
	// RTV/DSV создаются отдельно и живут до тех пор, пока мы их не уничтожим.
	ref_texture m_offscreenColor;
	ref_texture m_offscreenDepth;
	RHI_RenderTargetView m_offscreenRTV{};
	RHI_DepthStencilView m_offscreenDSV{};

	// --- Pass 1: colored triangle into offscreen ---
	ref_geometry m_triangle;
	CShaderPass m_passOffscreen;
	bool m_passOffscreenReady = false;

	// --- Pass 2: full-screen quad sampling offscreen texture ---
	ref_geometry m_quad;
	CShaderPass m_passDisplay;
	bool m_passDisplayReady = false;

	static constexpr uint32_t kOffscreenSize = 512;
};

//------------------------------------------------------------------------------

bool CBackendTest::Init(HINSTANCE hInst, int width, int height)
{
	// --- окно ---
	WNDCLASSA wc{};
	wc.lpfnWndProc = &CBackendTest::WndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = "xrDummy";
	RegisterClassA(&wc);

	RECT rc{0, 0, width, height};
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	m_hWnd = CreateWindowA("xrDummy", "X-Ray Test",
						   WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
						   rc.right - rc.left, rc.bottom - rc.top,
						   nullptr, nullptr, hInst, nullptr);
	if(!m_hWnd)
		return false;

	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);

	// --- устройство ---
	RHI_PresentationParams params{};
	params.Windowed = TRUE;
	params.BackBufferCount = 2;
	params.SyncInterval = 1;
	params.FullscreenRefreshHz = 60;

	R_ASSERT2(m_backend.CreateDevice(m_hWnd, RHI_BackendType::DirectX9Ex, params), "! [Test] CreateDevice failed");

	// --- offscreen render targets ---
	R_ASSERT2(CreateOffscreenTargets(), "! [Test] CreateOffscreenTargets failed");

	// --- Pass 1: цветной треугольник ---
	m_passOffscreen.SetVertexShader("test\\test.hlsl", "vs_main");
	m_passOffscreen.SetPixelShader("test\\test.hlsl", "ps_main");
	R_ASSERT2(m_passOffscreen.Compile(m_backend), "! [Test] pass 1 compile failed");
	m_passOffscreenReady = true;

	R_ASSERT2(CreateTriangleGeometry(), "! [Test] triangle geometry failed");

	// --- Pass 2: показ offscreen-текстуры ---
	m_passDisplay.SetVertexShader("test\\test.hlsl", "vs_quad");
	m_passDisplay.SetPixelShader("test\\test.hlsl", "ps_texture");
	R_ASSERT2(m_passDisplay.Compile(m_backend), "! [Test] pass 2 compile failed");
	m_passDisplayReady = true;

	R_ASSERT2(CreateQuadGeometry(), "! [Test] quad geometry failed");

	if(!m_passDisplay.SetTexture("s_tex", m_offscreenColor))
	{
		Msg("! [Test] Failed to bind offscreen texture to display pass "
			"(sampler 's_tex' not found in compiled shader)");
		return false;
	}

	Msg("* [Test] Init OK - %ux%u (offscreen=%ux%u)", width, height, kOffscreenSize, kOffscreenSize);
	return true;
}

//------------------------------------------------------------------------------

bool CBackendTest::CreateOffscreenTargets()
{
	IRenderBackend* rhi = m_backend.GetRHI();
	if(!rhi)
	{
		Msg("! [Test] CreateOffscreenTargets: RHI is null");
		return false;
	}

	m_offscreenColor = m_backend.CreateRenderTarget(kOffscreenSize, kOffscreenSize, RHI_Format::RGBA8_UNORM);
	if(!m_offscreenColor)
	{
		Msg("! [Test] CreateRenderTarget (offscreen color) failed");
		return false;
	}

	m_offscreenDepth = m_backend.CreateDepthStencil(kOffscreenSize, kOffscreenSize, RHI_Format::D24_UNORM_S8_UINT);
	if(!m_offscreenDepth)
	{
		Msg("! [Test] CreateDepthStencil (offscreen depth) failed");
		return false;
	}

	m_offscreenRTV = m_offscreenColor->CreateRTV();
	if(!m_offscreenRTV.IsValid())
	{
		Msg("! [Test] CreateRTV failed");
		return false;
	}

	m_offscreenDSV = m_offscreenDepth->CreateDSV();
	if(!m_offscreenDSV.IsValid())
	{
		Msg("! [Test] CreateDSV failed");
		return false;
	}

	Msg("* [Test] Offscreen targets ready: %ux%u rtv=%u dsv=%u", kOffscreenSize, kOffscreenSize, m_offscreenRTV.id, m_offscreenDSV.id);
	return true;
}

//------------------------------------------------------------------------------

bool CBackendTest::CreateTriangleGeometry()
{
	const STriangleVertex verts[] =
		{
			{0.0f, 0.6f, 0.5f, 1, 0, 0, 1},
			{0.5f, -0.4f, 0.5f, 0, 1, 0, 1},
			{-0.5f, -0.4f, 0.5f, 0, 0, 1, 1},
		};
	const uint16_t indices[] = {0, 1, 2};

	CVertexBufferDesc vbDesc{};
	vbDesc.sizeBytes = sizeof(verts);
	vbDesc.stride = sizeof(STriangleVertex);
	vbDesc.usage = BufferUsage_Immutable;
	vbDesc.debugName = "test.triangle.vb";
	ref_vertexbuffer vb = m_backend.CreateVertexBuffer(vbDesc, verts);
	if(!vb)
		return false;

	CIndexBufferDesc ibDesc{};
	ibDesc.sizeBytes = sizeof(indices);
	ibDesc.format = EIndexFormat::UInt16;
	ibDesc.usage = BufferUsage_Immutable;
	ibDesc.debugName = "test.triangle.ib";
	ref_indexbuffer ib = m_backend.CreateIndexBuffer(ibDesc, indices);
	if(!ib)
		return false;

	CVertexLayoutDesc layout;
	layout.elements.push_back({0, offsetof(STriangleVertex, x),
							   EVertexElementType::Float3, EVertexElementSemantic::Position,
							   0, EVertexInputRate::PerVertex, 0});
	layout.elements.push_back({0, offsetof(STriangleVertex, r),
							   EVertexElementType::Float4, EVertexElementSemantic::Color,
							   0, EVertexInputRate::PerVertex, 0});
	ref_vertexdecl vdecl = m_backend.CreateVertexDeclaration(layout);
	if(!vdecl)
		return false;

	m_triangle = m_backend.CreateGeometry();
	if(!m_triangle)
		return false;
	m_triangle->SetVertexDeclaration(vdecl);
	m_triangle->SetVertexBuffer(0, vb, 0, 0);
	m_triangle->SetIndexBuffer(ib);
	m_triangle->SetTopology(EPrimitiveTopology::TriangleList);
	return true;
}

bool CBackendTest::CreateQuadGeometry()
{
	// Full-screen quad в NDC. V-координата перевёрнута, чтобы (0,0) в текстуре
	// попадало в верхний-левый угол экрана. В D3D9 render target имеет origin
	// в верхнем-левом углу, но NDC — снизу-вверх.
	const SQuadVertex verts[] =
		{
			{-1.0f, -1.0f, 0.5f, 0.0f, 1.0f}, // bottom-left
			{-1.0f, 1.0f, 0.5f, 0.0f, 0.0f},  // top-left
			{1.0f, 1.0f, 0.5f, 1.0f, 0.0f},	  // top-right
			{1.0f, -1.0f, 0.5f, 1.0f, 1.0f},  // bottom-right
		};
	const uint16_t indices[] = {0, 1, 2, 0, 2, 3};

	CVertexBufferDesc vbDesc{};
	vbDesc.sizeBytes = sizeof(verts);
	vbDesc.stride = sizeof(SQuadVertex);
	vbDesc.usage = BufferUsage_Immutable;
	vbDesc.debugName = "test.quad.vb";
	ref_vertexbuffer vb = m_backend.CreateVertexBuffer(vbDesc, verts);
	if(!vb)
		return false;

	CIndexBufferDesc ibDesc{};
	ibDesc.sizeBytes = sizeof(indices);
	ibDesc.format = EIndexFormat::UInt16;
	ibDesc.usage = BufferUsage_Immutable;
	ibDesc.debugName = "test.quad.ib";
	ref_indexbuffer ib = m_backend.CreateIndexBuffer(ibDesc, indices);
	if(!ib)
		return false;

	CVertexLayoutDesc layout;
	layout.elements.push_back({0, offsetof(SQuadVertex, x),
							   EVertexElementType::Float3, EVertexElementSemantic::Position,
							   0, EVertexInputRate::PerVertex, 0});
	layout.elements.push_back({0, offsetof(SQuadVertex, u),
							   EVertexElementType::Float2, EVertexElementSemantic::TexCoord,
							   0, EVertexInputRate::PerVertex, 0});
	ref_vertexdecl vdecl = m_backend.CreateVertexDeclaration(layout);
	if(!vdecl)
		return false;

	m_quad = m_backend.CreateGeometry();
	if(!m_quad)
		return false;
	m_quad->SetVertexDeclaration(vdecl);
	m_quad->SetVertexBuffer(0, vb, 0, 0);
	m_quad->SetIndexBuffer(ib);
	m_quad->SetTopology(EPrimitiveTopology::TriangleList);
	return true;
}

//------------------------------------------------------------------------------

void CBackendTest::DrawPass1_Offscreen(IRenderBackend& rhi)
{
	RHI_RenderTargetView rtvs[1] = {m_offscreenRTV};
	rhi.SetRenderTargets(rtvs, 1, m_offscreenDSV);

	RHI_Viewport vp{0, 0, kOffscreenSize, kOffscreenSize, 0.0f, 1.0f};
	rhi.SetViewport(vp);

	rhi.ClearRenderTarget(m_offscreenRTV, fvec4{0.10f, 0.15f, 0.30f, 1.0f});
	rhi.ClearDepthStencil(m_offscreenDSV, 1.0f, 0);

	if(m_passOffscreenReady && m_triangle._get())
	{
		m_passOffscreen.Apply(m_backend);
		m_passOffscreen.ApplySamplers(m_backend);
		m_backend.DrawGeometry(m_triangle);
	}
}

void CBackendTest::DrawPass2_Display(IRenderBackend& rhi)
{
	const RHI_RenderTargetView bb = rhi.GetBackBufferRTV();
	const RHI_DepthStencilView ds = rhi.GetBackBufferDSV();

	RHI_RenderTargetView rtvs[1] = {bb};
	rhi.SetRenderTargets(rtvs, 1, ds);

	RHI_Viewport vp{0, 0, rhi.GetBackBufferWidth(), rhi.GetBackBufferHeight(), 0.0f, 1.0f};
	rhi.SetViewport(vp);

	rhi.ClearRenderTarget(bb, fvec4{0.05f, 0.05f, 0.05f, 1.0f});
	rhi.ClearDepthStencil(ds, 1.0f, 0);

	if(m_passDisplayReady && m_quad._get())
	{
		m_passDisplay.Apply(m_backend);
		m_passDisplay.ApplySamplers(m_backend); // bind offscreen texture → s_tex
		m_backend.DrawGeometry(m_quad);
	}
}

//------------------------------------------------------------------------------

void CBackendTest::Frame()
{
	if(!m_backend.IsReady())
		return;

	IRenderBackend* rhi = m_backend.GetRHI();
	if(!rhi)
		return;

	m_backend.BeginFrame();

	DrawPass1_Offscreen(*rhi);
	DrawPass2_Display(*rhi);

	m_backend.EndFrame();
	m_backend.Present();
}

//------------------------------------------------------------------------------

void CBackendTest::Shutdown()
{
	IRenderBackend* rhi = m_backend.GetRHI();

	m_passOffscreen.Release();
	m_passDisplay.Release();

	if(rhi)
	{
		if(m_offscreenRTV.IsValid())
			rhi->DestroyRTV(m_offscreenRTV);
		if(m_offscreenDSV.IsValid())
			rhi->DestroyDSV(m_offscreenDSV);
	}
	m_offscreenRTV = {};
	m_offscreenDSV = {};

	m_quad.Clear();
	m_triangle.Clear();

	m_offscreenColor.Clear();
	m_offscreenDepth.Clear();

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
