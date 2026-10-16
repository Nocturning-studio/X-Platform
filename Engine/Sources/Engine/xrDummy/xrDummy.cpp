////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
////////////////////////////////////////////////////////////////////////////////
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "resource.h"
////////////////////////////////////////////////////////////////////////////////
#include <xrMath/xrMath.h>
#include <xrCore/xrCore.h>
#include <X-RHI/include/XRHI.h>
#include <xrRenderBackend/xrRenderBackend.h>
#include <Engine/xrDummy/resource.h>
////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// Vertex layout треугольника
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
static constexpr uint32_t kOffscreenWidth = 1920;
static constexpr uint32_t kOffscreenHeight = 1080;

static constexpr uint32_t kTriangleCount = 3;

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

	HWND hWnd = CreateWindowA("xrDummy",
							  "X-Ray Sampleable Depth Test",
							  WS_OVERLAPPEDWINDOW,
							  CW_USEDEFAULT,
							  CW_USEDEFAULT,
							  rc.right - rc.left,
							  rc.bottom - rc.top,
							  nullptr, nullptr,
							  hInst,
							  nullptr);
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
	// Offscreen color RT
	// ========================================================================
	ref_texture offscreen = backend.CreateRenderTarget(kOffscreenWidth, kOffscreenHeight,
		RHI_Format::RGBA8_UNORM);
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
	// Offscreen depth: DSV + SRV (INTZ / DF24 / DF16)
	//
	// Двойной usage — DepthStencil | ShaderResource — это и есть триггер
	// подмены обычного D24S8 на INTZ в DX9-бэкенде. В DX12 это будет
	// D24S8/D32F с SRV-флагом.
	//
	// ВАЖНО: используем Resources().CreateTexture, а не backend.CreateDepthStencil,
	// потому что фабрика DepthStencil() не выставляет ShaderResource.
	// ========================================================================
	RHI_TextureDesc depthDesc = RHI_TextureDesc::DepthStencil(kOffscreenWidth, kOffscreenHeight, RHI_Format::D24_UNORM_S8_UINT);
	depthDesc.usage |= RHI_TexUsage_ShaderResource;
	depthDesc.debugName = "test.offscreen.depth";

	ref_texture offscreenDepth = backend.Resources().CreateTexture(depthDesc);
	if (!offscreenDepth)
	{
		R_ERROR("! [Test] CreateTexture(depth) failed — INTZ/DF24/DF16 unsupported?");
		return 1;
	}

	const RHI_DepthStencilView offscreenDSV = backend.CreateDSV(offscreenDepth->GetRHIHandle());
	if (!offscreenDSV.IsValid())
	{
		R_ERROR("! [Test] CreateDSV failed");
		return 1;
	}

	// ========================================================================
	// Геометрия одного треугольника (Z = 0, положение задаётся трансформом)
	// ========================================================================
	const STriangleVertex verts[] =
	{
		//   x      y     z     r  g  b  a
		{  0.0f,  0.6f, 0.0f,  1, 0, 0, 1 },
		{  0.5f, -0.4f, 0.0f,  0, 1, 0, 1 },
		{ -0.5f, -0.4f, 0.0f,  0, 0, 1, 1 },
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
	// Инстансы: три треугольника на разной глубине
	//
	// xrMath — row-vector convention: [xyz1] * M. Соответственно:
	//   - translate() кладёт смещение в 4-ю строку,
	//   - vs_triangle использует mul(float4(i.pos,1), mWorld).
	//
	// D3DCOMPILE_PACK_MATRIX_ROW_MAJOR (по умолчанию в RHI) согласуется
	// с CShaderConstantBuffer::SetMatrix — строки матрицы уходят в
	// последовательные float4-регистры как есть.
	// ========================================================================
	fmat4x4 triangleWorld[kTriangleCount] { Fidentity };
	triangleWorld[0].translate(-0.7f, 0.0f, 0.1f); // ближний,  слева
	triangleWorld[1].translate(0.0f, 0.0f, 0.5f); // средний,  центр
	triangleWorld[2].translate(0.7f, 0.0f, 0.9f); // дальний,  справа

	// ========================================================================
	// Pass 1: цветные треугольники -> offscreen RT + depth
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
	// Pass 2: fullscreen copy + depth visualization
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
		Msg("! [Test] passFullscreen: sampler 's_src' not found");

	if (!passFullscreen.SetTexture("s_depth", offscreenDepth))
		R_ERROR("! [Test] passFullscreen: sampler 's_depth' not found — проверь HLSL");

	// INTZ/DF24/DF16 не поддерживают линейную фильтрацию — только Point.
	passFullscreen.SetSamplerDesc("s_src", RHI_SamplerDesc::Linear());
	passFullscreen.SetSamplerDesc("s_depth", RHI_SamplerDesc::Point());

	// ========================================================================
	// PSO — собираем один раз, используем каждый кадр
	// ========================================================================

	// --- Pass 1: triangle -> offscreen (RGBA8 + D24S8 depth) ---
	RHI_PipelineStateDesc trianglePSD = backend.BuildPipelineDesc(passTriangle, 
																  *triangle,
																  RHI_BlendState::Opaque(),
																  RHI_DepthStencilState::Default(),   // depth ON + write
																  RHI_RasterizerState::CullNone(),
																  RHI_Format::RGBA8_UNORM,
																  RHI_Format::D24_UNORM_S8_UINT);     // формат DSV
	trianglePSD.debugName = "test.triangle";

	const RHI_PipelineStateHandle trianglePSO = backend.CreatePipelineState(trianglePSD);
	if (!trianglePSO.IsValid())
	{
		R_ERROR("! [Test] CreatePipelineState (triangle) failed");
		return 1;
	}

	// Проверка дедупликации PSO-кэша
	{
		const RHI_PipelineStateHandle again = backend.CreatePipelineState(trianglePSD);
		if (again != trianglePSO)
			R_ERRORF("! [Test] PSO cache dedup failed: %u != %u", trianglePSO.id, again.id);
		else
			Msg("* [Test] PSO cache dedup OK: trianglePSO id=%u", trianglePSO.id);
	}

	// --- Pass 2: fullscreen copy -> back buffer ---
	//
	// DrawFullscreen привязывает собственную геометрию, поэтому inputLayout
	// оставляем пустым, а BuildPipelineDesc (требующий CGeometry&) не
	// используем — собираем desc вручную.
	RHI_PipelineStateDesc fullscreenPSD;
	fullscreenPSD.vertexShader = passFullscreen.GetVertexShaderHandle();
	fullscreenPSD.pixelShader = passFullscreen.GetPixelShaderHandle();
	fullscreenPSD.inputLayout = {};
	fullscreenPSD.topologyType = RHI_PrimitiveTopologyType::Triangle;
	fullscreenPSD.blend = RHI_BlendState::Opaque();
	fullscreenPSD.depthStencil = RHI_DepthStencilState::NoDepth();
	fullscreenPSD.rasterizer = RHI_RasterizerState::CullNone();
	fullscreenPSD.rtvFormats[0] = backend.GetBackBufferFormat();
	fullscreenPSD.rtvCount = 1;
	fullscreenPSD.dsvFormat = RHI_Format::Unknown;
	fullscreenPSD.debugName = "test.fullscreen";

	const RHI_PipelineStateHandle fullscreenPSO = backend.CreatePipelineState(fullscreenPSD);
	if (!fullscreenPSO.IsValid())
	{
		R_ERROR("! [Test] CreatePipelineState (fullscreen) failed");
		return 1;
	}

	Msg("* [Test] Init OK - window=%ux%u, offscreen=%ux%u (color+depth), "
		"trianglePSO=%u, fullscreenPSO=%u",
		kWindowWidth, kWindowHeight, kOffscreenWidth, kOffscreenHeight,
		trianglePSO.id, fullscreenPSO.id);

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
		// Pass 1: три треугольника -> offscreen RT + depth
		// ------------------------------------------------------------
		{
			RHI_RenderTargetView rtvs[1] = { offscreenRTV };
			backend.SetRenderTargets(rtvs, 1, offscreenDSV);

			RHI_Viewport vp{ 0, 0, kOffscreenWidth, kOffscreenHeight, 0.0f, 1.0f };
			backend.SetViewport(vp);

			backend.ClearRenderTarget(offscreenRTV, fvec4{ 0.05f, 0.05f, 0.4f, 1.0f });
			backend.ClearDepthStencil(offscreenDSV, 1.0f, 0);

			backend.SetPipelineState(trianglePSO);
			backend.SetPrimitiveTopology(RHI_Topology::TriangleList);

			for (uint32_t t = 0; t < kTriangleCount; ++t)
			{
				if (!passTriangle.SetConstant("mWorld", triangleWorld[t]))
				{
					R_ERRORF("! [Test] SetConstant('mWorld') failed for instance %u", t);
					break;
				}
				backend.SetShaderPass(passTriangle);
				backend.DrawGeometry(triangle);
			}
		}

		// ------------------------------------------------------------
		// Pass 2: fullscreen — слева цвет, справа глубина
		// ------------------------------------------------------------
		{
			const RHI_RenderTargetView bb = backend.GetBackBufferRTV();
			const RHI_DepthStencilView ds = backend.GetBackBufferDSV();

			RHI_RenderTargetView rtvs[1] = { bb };
			backend.SetRenderTargets(rtvs, 1, ds);

			RHI_Viewport vp{ 0, 0, backend.GetBackBufferWidth(), backend.GetBackBufferHeight(), 0.0f, 1.0f };
			backend.SetViewport(vp);

			backend.ClearRenderTarget(bb, fvec4{ 0.02f, 0.02f, 0.02f, 1.0f });
			backend.ClearDepthStencil(ds, 1.0f, 0);

			backend.SetPipelineState(fullscreenPSO);
			backend.SetPrimitiveTopology(RHI_Topology::TriangleList);
			backend.SetShaderPass(passFullscreen);

			backend.DrawFullscreen();
		}

		backend.EndFrame();
		backend.Present();
	}

	// ========================================================================
	// Shutdown
	// ========================================================================

	passTriangle.Release();
	passFullscreen.Release();

	backend.DestroyDSV(offscreenDSV);
	backend.DestroyRTV(offscreenRTV);

	triangle.Clear();

	offscreenDepth.Clear();
	offscreen.Clear();
	vdecl.Clear();
	ib.Clear();
	vb.Clear();

	backend.DestroyPipelineState(trianglePSO);
	backend.DestroyPipelineState(fullscreenPSO);

	backend.DestroyDevice();

	if (hWnd)
		DestroyWindow(hWnd);

	return 0;
}
////////////////////////////////////////////////////////////////////////////////
