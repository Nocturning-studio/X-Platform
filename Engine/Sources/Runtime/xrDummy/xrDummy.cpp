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
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_DESTROY)
		{
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProcA(hWnd, msg, wParam, lParam);
	}

	bool CreateOffscreenTargets();
	void DestroyOffscreenTargets();

	void DrawScene(IRenderBackend& rhi);

	HWND m_hWnd = nullptr;
	CRenderBackend m_backend;

	ref_geometry m_triangle;
	CShaderPass m_shader;
	bool m_shaderReady = false;

	// --- Offscreen render targets (RHI-native) ---
	// Хэндлы текстуры + view'ов. Хранятся раздельно: DestroyRTV/DSV должны
	// вызываться ДО DestroyTexture — иначе surface из D3DPOOL_DEFAULT
	// останется висеть и не даст освободить текстуру.
	RHI_TextureHandle    m_offscreenColorTex{};
	RHI_TextureHandle    m_offscreenDepthTex{};
	RHI_RenderTargetView m_offscreenRTV{};
	RHI_DepthStencilView m_offscreenDSV{};

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

	RECT rc{ 0, 0, width, height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	m_hWnd = CreateWindowA(
		"xrDummy", "X-Ray Test",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		nullptr, nullptr, hInst, nullptr);
	if (!m_hWnd)
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

	// --- шейдеры ---
	m_shader.SetVertexShader("test\\test.hlsl", "vs_main");
	m_shader.SetPixelShader("test\\test.hlsl", "ps_main");
	R_ASSERT2(m_shader.Compile(m_backend), "! [Test] shader compile failed");
	m_shaderReady = true;

	// --- offscreen render targets ---
	R_ASSERT2(CreateOffscreenTargets(), "! [Test] CreateOffscreenTargets failed");

	// --- геометрия ---
	const STestVertex verts[] =
	{
		{0.0f,  0.6f, 0.5f, 1, 0, 0, 1},
		{0.5f, -0.4f, 0.5f, 0, 1, 0, 1},
		{-0.5f, -0.4f, 0.5f, 0, 0, 1, 1},
	};
	const uint16_t indices[] = { 0, 1, 2 };

	CVertexBufferDesc vbDesc{};
	vbDesc.sizeBytes = sizeof(verts);
	vbDesc.stride = sizeof(STestVertex);
	vbDesc.usage = BufferUsage_Immutable;
	vbDesc.debugName = "test.triangle.vb";
	ref_vertexbuffer vb = m_backend.CreateVertexBuffer(vbDesc, verts);
	R_ASSERT2(vb, "! [Test] CreateVertexBuffer failed");

	CIndexBufferDesc ibDesc{};
	ibDesc.sizeBytes = sizeof(indices);
	ibDesc.format = EIndexFormat::UInt16;
	ibDesc.usage = BufferUsage_Immutable;
	ibDesc.debugName = "test.triangle.ib";
	ref_indexbuffer ib = m_backend.CreateIndexBuffer(ibDesc, indices);
	R_ASSERT2(ib, "! [Test] CreateIndexBuffer failed");

	// vertex layout: POSITION.xyz + COLOR.rgba
	CVertexLayoutDesc layout;
	layout.elements.push_back({ 0, offsetof(STestVertex, x),
							   EVertexElementType::Float3, EVertexElementSemantic::Position,
							   0, EVertexInputRate::PerVertex, 0 });
	layout.elements.push_back({ 0, offsetof(STestVertex, r),
							   EVertexElementType::Float4, EVertexElementSemantic::Color,
							   0, EVertexInputRate::PerVertex, 0 });
	ref_vertexdecl vdecl = m_backend.CreateVertexDeclaration(layout);
	R_ASSERT2(vdecl, "! [Test] CreateVertexDeclaration failed");

	m_triangle = m_backend.CreateGeometry();
	R_ASSERT2(m_triangle, "! [Test] CreateGeometry failed");
	m_triangle->SetVertexDeclaration(vdecl);
	m_triangle->SetVertexBuffer(0, vb, 0, 0);
	m_triangle->SetIndexBuffer(ib);
	m_triangle->SetTopology(EPrimitiveTopology::TriangleList);

	Msg("* [Test] Init OK - %ux%u (vb=%u B, ib=%u idx)",
		width, height, vbDesc.sizeBytes, (uint32_t)(sizeof(indices) / sizeof(indices[0])));
	return true;
}

//------------------------------------------------------------------------------

bool CBackendTest::CreateOffscreenTargets()
{
	IRenderBackend* rhi = m_backend.GetRHI();
	if (!rhi)
	{
		Msg("! [Test] CreateOffscreenTargets: RHI is null");
		return false;
	}

	// --- Color RT ---
	RHI_TextureDesc colorDesc{};
	colorDesc.width = kOffscreenSize;
	colorDesc.height = kOffscreenSize;
	colorDesc.depth = 1;
	colorDesc.mipLevels = 1;
	colorDesc.format = RHI_Format::RGBA8_UNORM;
	colorDesc.isRenderTarget = true;
	colorDesc.isDepthStencil = false;
	colorDesc.isCubeMap = false;

	m_offscreenColorTex = rhi->CreateTexture(colorDesc);
	if (!m_offscreenColorTex.IsValid())
	{
		Msg("! [Test] CreateTexture (offscreen color) failed");
		return false;
	}

	m_offscreenRTV = rhi->CreateRTV(m_offscreenColorTex);
	if (!m_offscreenRTV.IsValid())
	{
		Msg("! [Test] CreateRTV failed");
		return false;
	}

	// --- Depth/stencil ---
	RHI_TextureDesc depthDesc{};
	depthDesc.width = kOffscreenSize;
	depthDesc.height = kOffscreenSize;
	depthDesc.depth = 1;
	depthDesc.mipLevels = 1;
	depthDesc.format = RHI_Format::D24_UNORM_S8_UINT;
	depthDesc.isRenderTarget = false;
	depthDesc.isDepthStencil = true;
	depthDesc.isCubeMap = false;

	m_offscreenDepthTex = rhi->CreateTexture(depthDesc);
	if (!m_offscreenDepthTex.IsValid())
	{
		Msg("! [Test] CreateTexture (offscreen depth) failed");
		return false;
	}

	m_offscreenDSV = rhi->CreateDSV(m_offscreenDepthTex);
	if (!m_offscreenDSV.IsValid())
	{
		Msg("! [Test] CreateDSV failed");
		return false;
	}

	Msg("* [Test] Offscreen targets ready: %ux%u color=%u depth=%u rtv=%u dsv=%u",
		kOffscreenSize, kOffscreenSize,
		m_offscreenColorTex.id, m_offscreenDepthTex.id,
		m_offscreenRTV.id, m_offscreenDSV.id);
	return true;
}

void CBackendTest::DestroyOffscreenTargets()
{
	IRenderBackend* rhi = m_backend.GetRHI();
	if (!rhi)
		return;

	// ВАЖНЫЙ ПОРЯДОК: сначала views, потом текстуры.
	//
	// В D3D9 поверхность, полученная через GetSurfaceLevel, держит ref на
	// родительскую текстуру. DestroyRTV/DSV делают Release на surface — и
	// только после этого текстура сможет реально освободиться в
	// DestroyTexture.
	if (m_offscreenRTV.IsValid())
		rhi->DestroyRTV(m_offscreenRTV);
	if (m_offscreenDSV.IsValid())
		rhi->DestroyDSV(m_offscreenDSV);

	if (m_offscreenColorTex.IsValid())
		rhi->DestroyTexture(m_offscreenColorTex);
	if (m_offscreenDepthTex.IsValid())
		rhi->DestroyTexture(m_offscreenDepthTex);

	m_offscreenRTV = {};
	m_offscreenDSV = {};
	m_offscreenColorTex = {};
	m_offscreenDepthTex = {};
}

//------------------------------------------------------------------------------

void CBackendTest::DrawScene(IRenderBackend& rhi)
{
	if (!m_shaderReady || !m_triangle._get())
		return;

	m_shader.Apply(m_backend);
	m_shader.ApplySamplers(m_backend);
	m_backend.DrawGeometry(m_triangle);
}

//------------------------------------------------------------------------------

void CBackendTest::Frame()
{
	// --- device lost ---
	if (!m_backend.IsReady())
		return;

	IRenderBackend* rhi = m_backend.GetRHI();
	if (!rhi)
		return;

	m_backend.BeginFrame();

	// ============================================================
	// Pass 1: offscreen
	// ============================================================
	// Рендерим в 512x512 offscreen RT. Результат сейчас никто не видит —
	// привязки текстур к шейдеру в RHI пока нет. Но API-путь полностью
	// прогоняется: SetRenderTargets, SetViewport, Clear*, Draw.
	{
		RHI_RenderTargetView rtvs[1] = { m_offscreenRTV };
		rhi->SetRenderTargets(rtvs, 1, m_offscreenDSV);

		RHI_Viewport vp{ 0, 0, kOffscreenSize, kOffscreenSize, 0.0f, 1.0f };
		rhi->SetViewport(vp);

		// Явная очистка конкретных targets — не зависит от того, что сейчас
		// привязано. Небольшой красный оттенок, чтобы в отладчике было
		// понятно, какой из pass'ов дал какую картинку.
		rhi->ClearRenderTarget(m_offscreenRTV, fvec4{ 0.35f, 0.10f, 0.10f, 1.0f });
		rhi->ClearDepthStencil(m_offscreenDSV, 1.0f, 0);

		DrawScene(*rhi);
	}

	// ============================================================
	// Pass 2: back buffer
	// ============================================================
	{
		const RHI_RenderTargetView bb = rhi->GetBackBufferRTV();
		const RHI_DepthStencilView ds = rhi->GetBackBufferDSV();

		RHI_RenderTargetView rtvs[1] = { bb };
		rhi->SetRenderTargets(rtvs, 1, ds);

		RHI_Viewport vp{ 0, 0, rhi->GetBackBufferWidth(), rhi->GetBackBufferHeight(), 0.0f, 1.0f };
		rhi->SetViewport(vp);

		// Тут используем CRenderBackend::Clear — он чистит «текущий привязанный»
		// target. Просто чтобы показать: оба пути дают одинаковый результат.
		m_backend.Clear(RHI_CLEAR_TARGET | RHI_CLEAR_ZBUFFER | RHI_CLEAR_STENCIL, 0xFF102030, 1.0f, 0);

		DrawScene(*rhi);
	}

	m_backend.EndFrame();
	m_backend.Present();
}

//------------------------------------------------------------------------------

void CBackendTest::Shutdown()
{
	m_shader.Release();
	m_triangle.Clear();
	m_backend.DestroyDevice();

	if (m_hWnd)
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
	if (!test.Init(hInst, 1280, 720))
	{
		test.Shutdown();
		return 1;
	}

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
		if (running)
			test.Frame();
	}

	test.Shutdown();
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
