////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "RenderBackend.h"
#include <xrRHI/xrRHI.h>
////////////////////////////////////////////////////////////////////////////////
namespace
{
	// 0xAARRGGBB -> fvec4
	inline fvec4 ARGBToFvec4(uint32_t c)
	{
		const float inv = 1.0f / 255.0f;
		return fvec4{ ((c >> 16) & 0xFF) * inv,
					  ((c >> 8) & 0xFF) * inv,
					  (c & 0xFF) * inv,
					  ((c >> 24) & 0xFF) * inv };
	}
} // namespace

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

bool CRenderBackend::CreateDevice(HWND hWnd, RHI_BackendType backendType, const RHI_PresentationParams& params)
{
	if (m_pRHI)
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

	m_presentParams = params;
	m_presentParams.BackBufferWidth = m_pRHI->GetBackBufferWidth();
	m_presentParams.BackBufferHeight = m_pRHI->GetBackBufferHeight();

	m_resources.SetRHI(m_pRHI);

	Msg("* [RenderBackend] Device created: %ux%u", m_presentParams.BackBufferWidth, m_presentParams.BackBufferHeight);
	return true;
}

void CRenderBackend::DestroyDevice()
{
	if (!m_pRHI && !m_hRHI)
		return;

	m_resources.DestroyAll();
	m_resources.SetRHI(nullptr);

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

	m_inScene = false;
	ZeroMemory(&m_presentParams, sizeof(m_presentParams));
}

bool CRenderBackend::ResetDevice(const RHI_PresentationParams& params)
{
	if (!m_pRHI)
		return false;

	if (!m_pRHI->Reset(params))
	{
		Msg("! [RenderBackend] RHI Reset failed");
		return false;
	}

	m_presentParams = params;
	m_presentParams.BackBufferWidth = m_pRHI->GetBackBufferWidth();
	m_presentParams.BackBufferHeight = m_pRHI->GetBackBufferHeight();

	m_resources.SetRHI(m_pRHI);

	Msg("* [RenderBackend] Device reset: %ux%u", m_presentParams.BackBufferWidth, m_presentParams.BackBufferHeight);
	return true;
}

bool CRenderBackend::NeedReset() const
{
	if (!m_pRHI)
		return false;
	return m_pRHI->CheckDeviceStatus() == RHI_DeviceStatus::NeedReset;
}

void CRenderBackend::BeginFrame()
{
	if (!m_pRHI || m_inScene)
		return;

	m_resources.OnFrameBegin();
	m_pRHI->OnFrameBegin();

	m_inScene = true;
}

void CRenderBackend::EndFrame()
{
	if (!m_pRHI || !m_inScene)
		return;

	m_pRHI->OnFrameEnd();

	m_resources.OnFrameEnd();
	m_inScene = false;
}

void CRenderBackend::Present()
{
	if (m_pRHI)
		m_pRHI->Present();
}

bool CRenderBackend::OnDeviceReset()
{
	if (!m_pRHI)
		return false;

	m_resources.SetRHI(m_pRHI);
	return true;
}

IDirect3DDevice9Ex* CRenderBackend::GetDevice() const
{
	if (!m_pRHI)
		return nullptr;
	return static_cast<IDirect3DDevice9Ex*>(m_pRHI->GetDeviceHandle());
}

const RHIDeviceCaps& CRenderBackend::GetDeviceCaps() const
{
	static RHIDeviceCaps s_empty{};
	return m_pRHI ? m_pRHI->GetDeviceCaps() : s_empty;
}

uint32_t CRenderBackend::GetBackBufferWidth() const
{
	return m_pRHI ? m_pRHI->GetBackBufferWidth() : 0;
}

uint32_t CRenderBackend::GetBackBufferHeight() const
{
	return m_pRHI ? m_pRHI->GetBackBufferHeight() : 0;
}

void CRenderBackend::SetShaderPass(CShaderPass* pass)
{
	if (pass)
	{
		pass->Apply(*this);
		return;
	}

	if (!m_pRHI)
		return;

	m_pRHI->SetVertexShader(RHI_ShaderHandle{});
	m_pRHI->SetPixelShader(RHI_ShaderHandle{});

	for (u32 i = 0; i < 16; ++i)
		m_pRHI->SetShaderResource(i, RHI_TextureHandle{});
}

ref_texture CRenderBackend::CreateRenderTarget(uint32_t w, uint32_t h, RHI_Format fmt, uint32_t mips)
{
	return m_resources.CreateRenderTarget(w, h, fmt, mips);
}

ref_texture CRenderBackend::CreateDepthStencil(uint32_t w, uint32_t h, RHI_Format fmt)
{
	return m_resources.CreateDepthStencil(w, h, fmt);
}

void CRenderBackend::Clear(uint32_t flags, uint32_t colorARGB, float z, uint32_t stencil)
{
	if (!m_pRHI)
		return;

	m_pRHI->Clear(flags, ARGBToFvec4(colorARGB), z, static_cast<u8>(stencil));
}

ref_vertexdecl CRenderBackend::CreateVertexDeclaration(const RHI_InputLayoutDesc& layout)
{
	return m_resources.CreateVertexDeclaration(layout);
}

ref_vertexbuffer CRenderBackend::CreateVertexBuffer(const RHI_BufferDesc& desc, const void* data)
{
	return m_resources.CreateVertexBuffer(desc, data);
}

ref_indexbuffer CRenderBackend::CreateIndexBuffer(const RHI_BufferDesc& desc, const void* data)
{
	return m_resources.CreateIndexBuffer(desc, data);
}

ref_geometry CRenderBackend::CreateGeometry()
{
	return m_resources.CreateGeometry();
}

void CRenderBackend::BindGeometry(const ref_geometry& g)
{
	if (g._get() && m_pRHI)
		g->Bind(*m_pRHI);
}

void CRenderBackend::DrawGeometry(const ref_geometry& g)
{
	if (!g._get() || !m_pRHI)
		return;
	g->Bind(*m_pRHI);
	g->Draw(*m_pRHI);
}
////////////////////////////////////////////////////////////////////////////////
