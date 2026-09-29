////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "RenderBackend.h"
#include <xrRHI/xrRHI.h>
////////////////////////////////////////////////////////////////////////////////

CRenderBackend::CRenderBackend() = default;

CRenderBackend::~CRenderBackend()
{
	DestroyDevice();
}

bool CRenderBackend::LoadRHIModule(RHI_BackendType type)
{
	if (m_pRHI)
		return true;

	m_hRHI = LoadLibraryA("xrRHI.dll");
	if (!m_hRHI)
	{
		Msg("! [RenderBackend] Failed to load xrRHI.dll");
		return false;
	}

	using CreateBackendFunc = IRenderBackend * (*)(RHI_BackendType);
	auto createBackend = reinterpret_cast<CreateBackendFunc>(
		GetProcAddress(m_hRHI, "CreateRenderBackend"));

	if (!createBackend)
	{
		Msg("! [RenderBackend] CreateRenderBackend not found in xrRHI.dll");
		FreeLibrary(m_hRHI);
		m_hRHI = nullptr;
		return false;
	}

	m_pRHI = createBackend(type);
	if (!m_pRHI)
	{
		Msg("! [RenderBackend] Failed to create IRenderBackend");
		FreeLibrary(m_hRHI);
		m_hRHI = nullptr;
		return false;
	}

	return true;
}

bool CRenderBackend::CreateDevice(HWND hWnd,
	RHI_BackendType backendType,
	const RHI_PresentationParams & params)
{
	if (m_pDevice)
	{
		Msg("! [RenderBackend] CreateDevice: device already created");
		return false;
	}

	if (!LoadRHIModule(backendType))
		return false;

	if (!m_pRHI->CreateDevice(hWnd, params))
	{
		Msg("! [RenderBackend] IRenderBackend::CreateDevice failed");
		delete m_pRHI;
		m_pRHI = nullptr;
		FreeLibrary(m_hRHI);
		m_hRHI = nullptr;
		return false;
	}

	// Кешируем D3D-указатели.
	m_pD3D = static_cast<IDirect3D9Ex*>(m_pRHI->GetD3DHandle());
	m_pDevice = static_cast<IDirect3DDevice9Ex*>(m_pRHI->GetDeviceHandle());

	if (!m_pDevice)
	{
		Msg("! [RenderBackend] RHI returned null device handle");
		DestroyDevice();
		return false;
	}

	if (!AcquireBackBuffers())
	{
		DestroyDevice();
		return false;
	}

	ApplyPresentParamsFromRHI(params);
	SetupDefaultViewport();

	// Сообщаем ресурсам про device.
	m_resources.SetDevice(m_pDevice);
	m_resources.OnDeviceReset(m_pDevice);

	D3DSURFACE_DESC desc{};
	m_pBaseRT->GetDesc(&desc);
	Msg("* [RenderBackend] Device created: %ux%u", desc.Width, desc.Height);
	return true;
}

void CRenderBackend::DestroyDevice()
{
	if (!m_pRHI && !m_hRHI && !m_pDevice)
		return;

	// Порядок: сначала подсистемы, затем сам RHI.
	m_resources.OnDeviceLost();
	m_states.Invalidate();
	ReleaseBackBuffers();

	if (m_pRHI)
	{
		m_pRHI->DestroyDevice();
		delete m_pRHI;
		m_pRHI = nullptr;
	}
	if (m_hRHI)
	{
		FreeLibrary(m_hRHI);
		m_hRHI = nullptr;
	}

	m_pD3D = nullptr;
	m_pDevice = nullptr;
	m_inScene = false;

	ZeroMemory(&m_DevPP, sizeof(m_DevPP));
}

bool CRenderBackend::ResetDevice(const RHI_PresentationParams & params)
{
	if (!m_pRHI || !m_pRHI->GetDeviceHandle())
		return false;

	// Отпускаем backbuffer/zb ДО Reset — иначе D3D9 откажет.
	ReleaseBackBuffers();

	if (!m_pRHI->Reset(params))
	{
		Msg("! [RenderBackend] RHI Reset failed");
		return false;
	}

	// Device-хэндлы после reset могут отличаться — перечитываем.
	m_pD3D = static_cast<IDirect3D9Ex*>(m_pRHI->GetD3DHandle());
	m_pDevice = static_cast<IDirect3DDevice9Ex*>(m_pRHI->GetDeviceHandle());
	if (!m_pDevice)
		return false;

	if (!AcquireBackBuffers())
		return false;

	ApplyPresentParamsFromRHI(params);
	SetupDefaultViewport();

	m_resources.SetDevice(m_pDevice);
	m_resources.OnDeviceReset(m_pDevice);

	return true;
}

bool CRenderBackend::NeedReset() const
{
	if (!m_pDevice)
		return false;
	return m_pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET;
}

void CRenderBackend::BeginFrame()
{
	if (!m_pDevice || m_inScene)
		return;

	m_resources.OnFrameBegin();
	if (m_pRHI)
		m_pRHI->OnFrameBegin();

	m_inScene = true;
}

void CRenderBackend::EndFrame()
{
	if (!m_pDevice || !m_inScene)
		return;

	if (m_pRHI)
		m_pRHI->OnFrameEnd();

	m_resources.OnFrameEnd();
	m_inScene = false;
}

void CRenderBackend::Present()
{
	if (m_pRHI)
		m_pRHI->Present();
}

void CRenderBackend::OnDeviceLost()
{
	if (!m_pDevice)
		return;

	m_resources.OnDeviceLost();
	m_states.Invalidate();
	m_inScene = false;
}

bool CRenderBackend::OnDeviceReset()
{
	if (!m_pRHI)
		return false;

	m_pD3D = static_cast<IDirect3D9Ex*>(m_pRHI->GetD3DHandle());
	m_pDevice = static_cast<IDirect3DDevice9Ex*>(m_pRHI->GetDeviceHandle());
	if (!m_pDevice)
		return false;

	if (!AcquireBackBuffers())
		return false;

	SetupDefaultViewport();
	m_resources.SetDevice(m_pDevice);
	m_resources.OnDeviceReset(m_pDevice);
	return true;
}

const RHIDeviceCaps& CRenderBackend::GetDeviceCaps() const
{
	static RHIDeviceCaps s_empty{};
	return m_pRHI ? m_pRHI->GetDeviceCaps() : s_empty;
}

void CRenderBackend::SetShaderPass(CShaderPass* pass)
{
	if (pass)
	{
		pass->Apply(*this);
		return;
	}

	if (m_pDevice)
	{
		m_pDevice->SetVertexShader(nullptr);
		m_pDevice->SetPixelShader(nullptr);
		for (uint32_t i = 0; i < 16; ++i)
			m_pDevice->SetTexture(i, nullptr);
	}
}

ref_texture CRenderBackend::CreateRenderTarget(uint32_t w, uint32_t h, ETextureFormat fmt, uint32_t mips)
{
	return m_resources.CreateRenderTarget(w, h, fmt, mips);
}

ref_texture CRenderBackend::CreateDepthStencil(uint32_t w, uint32_t h, ETextureFormat fmt)
{
	return m_resources.CreateDepthStencil(w, h, fmt);
}

void CRenderBackend::Clear(uint32_t flags, D3DCOLOR color, float z, uint32_t stencil)
{
	if (!m_pDevice) return;
	m_pDevice->Clear(0, nullptr, flags, color, z, stencil);
}

ref_vertexdecl CRenderBackend::CreateVertexDeclaration(const CVertexLayoutDesc& layout)
{
	return m_resources.CreateVertexDeclaration(layout);
}

ref_vertexbuffer CRenderBackend::CreateVertexBuffer(const CVertexBufferDesc& desc, const void* data)
{
	return m_resources.CreateVertexBuffer(desc, data);
}

ref_indexbuffer CRenderBackend::CreateIndexBuffer(const CIndexBufferDesc& desc, const void* data)
{
	return m_resources.CreateIndexBuffer(desc, data);
}

ref_geometry CRenderBackend::CreateGeometry()
{
	return m_resources.CreateGeometry();
}

void CRenderBackend::BindGeometry(const ref_geometry& g)
{
	if (g._get() && m_pDevice)
		g->Bind(m_pDevice);
}

void CRenderBackend::DrawGeometry(const ref_geometry& g)
{
	if (!g._get() || !m_pDevice)
		return;

	// В D3D9 топология — параметр Draw*, поэтому Bind+Draw идут одним куском.
	// В DX11/12 здесь дополнительно нужно выставить primitive topology в IA-стейт.
	g->Bind(m_pDevice);
	g->Draw(m_pDevice);
}

// ---------------------------------------------------------------------------

bool CRenderBackend::AcquireBackBuffers()
{
	ReleaseBackBuffers();
	if (!m_pDevice) return false;

	if (FAILED(m_pDevice->GetRenderTarget(0, &m_pBaseRT)))
	{
		Msg("! [RenderBackend] GetRenderTarget(0) failed");
		return false;
	}
	if (FAILED(m_pDevice->GetDepthStencilSurface(&m_pBaseZB)))
	{
		Msg("! [RenderBackend] GetDepthStencilSurface failed");
		RELEASE(m_pBaseRT);
		return false;
	}
	return true;
}

void CRenderBackend::ReleaseBackBuffers()
{
	RELEASE(m_pBaseZB);
	RELEASE(m_pBaseRT);
}

void CRenderBackend::ApplyPresentParamsFromRHI(const RHI_PresentationParams & params)
{
	D3DSURFACE_DESC desc{};
	if (m_pBaseRT)
		m_pBaseRT->GetDesc(&desc);

	m_DevPP.BackBufferWidth = desc.Width;
	m_DevPP.BackBufferHeight = desc.Height;
	m_DevPP.BackBufferFormat = D3DFMT_X8R8G8B8;
	m_DevPP.Windowed = params.Windowed;
	m_DevPP.PresentationInterval = (params.SyncInterval == 0)
		? D3DPRESENT_INTERVAL_IMMEDIATE
		: D3DPRESENT_INTERVAL_DEFAULT;
	m_DevPP.BackBufferCount = params.BackBufferCount;
	m_DevPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_DevPP.FullScreen_RefreshRateInHz = params.FullscreenRefreshHz;
}

void CRenderBackend::SetupDefaultViewport()
{
	if (!m_pDevice || !m_pBaseRT) return;

	D3DSURFACE_DESC desc{};
	m_pBaseRT->GetDesc(&desc);

	D3DVIEWPORT9 vp{};
	vp.X = 0; vp.Y = 0;
	vp.Width = desc.Width;
	vp.Height = desc.Height;
	vp.MinZ = 0.0f; vp.MaxZ = 1.0f;
	m_pDevice->SetViewport(&vp);
}
////////////////////////////////////////////////////////////////////////////////
