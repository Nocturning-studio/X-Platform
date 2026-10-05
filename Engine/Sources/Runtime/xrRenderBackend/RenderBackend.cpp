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

RHI_Format CRenderBackend::GetBackBufferFormat() const
{
	return m_pRHI ? m_pRHI->GetBackBufferFormat() : RHI_Format::Unknown;
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

void CRenderBackend::Clear(uint32_t flags, fvec4 colorRGBA, float z, uint32_t stencil)
{
	if (!m_pRHI)
		return;

	m_pRHI->Clear(flags, colorRGBA, z, static_cast<u8>(stencil));
}

RHI_DeviceStatus CRenderBackend::CheckDeviceStatus() const
{
	return m_pRHI ? m_pRHI->CheckDeviceStatus() : RHI_DeviceStatus::Lost;
}

void CRenderBackend::GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const
{
	outResolutions.clear();
	if (m_pRHI)
		m_pRHI->GetAvailableResolutions(format, outResolutions);
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

RHI_RenderTargetView CRenderBackend::CreateRTV(RHI_TextureHandle tex, uint32_t mip, uint32_t face)
{
	if (!m_pRHI)
		return {};
	return m_pRHI->CreateRTV(tex, mip, face);
}

RHI_DepthStencilView CRenderBackend::CreateDSV(RHI_TextureHandle tex, uint32_t mip, uint32_t face)
{
	if (!m_pRHI)
		return {};
	return m_pRHI->CreateDSV(tex, mip, face);
}

void CRenderBackend::DestroyRTV(RHI_RenderTargetView rtv)
{
	if (m_pRHI)
		m_pRHI->DestroyRTV(rtv);
}

void CRenderBackend::DestroyDSV(RHI_DepthStencilView dsv)
{
	if (m_pRHI)
		m_pRHI->DestroyDSV(dsv);
}

RHI_RenderTargetView CRenderBackend::GetBackBufferRTV() const
{
	if (!m_pRHI)
		return {};
	return m_pRHI->GetBackBufferRTV();
}

RHI_DepthStencilView CRenderBackend::GetBackBufferDSV() const
{
	if (!m_pRHI)
		return {};
	return m_pRHI->GetBackBufferDSV();
}

void CRenderBackend::SetRenderTargets(const RHI_RenderTargetView* rtvs, uint32_t count, RHI_DepthStencilView dsv)
{
	if (m_pRHI)
		m_pRHI->SetRenderTargets(rtvs, count, dsv);
}

void CRenderBackend::ClearRenderTarget(RHI_RenderTargetView rtv, const fvec4& color)
{
	if (m_pRHI)
		m_pRHI->ClearRenderTarget(rtv, color);
}

void CRenderBackend::ClearDepthStencil(RHI_DepthStencilView dsv, float depth, uint8_t stencil)
{
	if (m_pRHI)
		m_pRHI->ClearDepthStencil(dsv, depth, stencil);
}

void CRenderBackend::SetBlendState(const RHI_BlendState& state)
{
	if (m_pRHI)
		m_pRHI->SetBlendState(state);
}

void CRenderBackend::SetDepthStencilState(const RHI_DepthStencilState& state)
{
	if (m_pRHI)
		m_pRHI->SetDepthStencilState(state);
}

void CRenderBackend::SetRasterizerState(const RHI_RasterizerState& state)
{
	if (m_pRHI)
		m_pRHI->SetRasterizerState(state);
}

const RHI_BlendState& CRenderBackend::GetBlendState() const
{
	static const RHI_BlendState s_empty{};
	return m_pRHI ? m_pRHI->GetBlendState() : s_empty;
}

const RHI_DepthStencilState& CRenderBackend::GetDepthStencilState() const
{
	static const RHI_DepthStencilState s_empty{};
	return m_pRHI ? m_pRHI->GetDepthStencilState() : s_empty;
}

const RHI_RasterizerState& CRenderBackend::GetRasterizerState() const
{
	static const RHI_RasterizerState s_empty{};
	return m_pRHI ? m_pRHI->GetRasterizerState() : s_empty;
}

void CRenderBackend::SetViewport(const RHI_Viewport& vp)
{
	if (m_pRHI)
		m_pRHI->SetViewport(vp);
}

void CRenderBackend::SetScissorRect(const RHI_Rect* rect)
{
	if (m_pRHI)
		m_pRHI->SetScissorRect(rect);
}

RHI_Viewport CRenderBackend::GetViewport() const
{
	if (!m_pRHI)
		return {};
	return m_pRHI->GetViewport();
}

bool CRenderBackend::GetScissorRect(RHI_Rect& out) const
{
	if (!m_pRHI)
		return false;
	return m_pRHI->GetScissorRect(out);
}

void CRenderBackend::InvalidateStateCache()
{
	if (m_pRHI)
		m_pRHI->InvalidateStateCache();
}

void CRenderBackend::SetVertexBuffer(uint32_t slot, RHI_BufferHandle vb,
	uint32_t offset, uint32_t stride)
{
	if (m_pRHI)
		m_pRHI->SetVertexBuffer(slot, vb, offset, stride);
}

void CRenderBackend::SetIndexBuffer(RHI_BufferHandle ib, RHI_IndexFormat fmt)
{
	if (m_pRHI)
		m_pRHI->SetIndexBuffer(ib, fmt);
}

void CRenderBackend::SetInputLayout(RHI_InputLayoutHandle layout)
{
	if (m_pRHI)
		m_pRHI->SetInputLayout(layout);
}

void CRenderBackend::SetPrimitiveTopology(RHI_Topology topology)
{
	if (m_pRHI)
		m_pRHI->SetPrimitiveTopology(topology);
}

void CRenderBackend::Draw(uint32_t vertexCount, uint32_t startVertex)
{
	if (m_pRHI)
		m_pRHI->Draw(vertexCount, startVertex);
}

void CRenderBackend::DrawIndexed(uint32_t indexCount, uint32_t startIndex, uint32_t baseVertex)
{
	if (m_pRHI)
		m_pRHI->DrawIndexed(indexCount, startIndex, baseVertex);
}

RHI_ShaderCompileResult CRenderBackend::CompileShader(const RHI_ShaderCompileDesc& desc)
{
	if (!m_pRHI)
	{
		RHI_ShaderCompileResult r;
		r.errorMessage = "backend is not ready";
		return r;
	}
	return m_pRHI->CompileShader(desc);
}

RHI_ShaderHandle CRenderBackend::CreateVertexShader(const void* bytecode, size_t size, const char* debugName)
{
	if (!m_pRHI)
		return {};
	return m_pRHI->CreateVertexShader(bytecode, size, debugName);
}

RHI_ShaderHandle CRenderBackend::CreatePixelShader(const void* bytecode, size_t size, const char* debugName)
{
	if (!m_pRHI)
		return {};
	return m_pRHI->CreatePixelShader(bytecode, size, debugName);
}

void CRenderBackend::DestroyShader(RHI_ShaderHandle handle)
{
	if (m_pRHI)
		m_pRHI->DestroyShader(handle);
}

bool CRenderBackend::GetShaderBytecode(RHI_ShaderHandle handle, const void** outData, size_t* outSize)
{
	if (!m_pRHI)
		return false;
	return m_pRHI->GetShaderBytecode(handle, outData, outSize);
}

void CRenderBackend::SetVertexShader(RHI_ShaderHandle handle)
{
	if (m_pRHI)
		m_pRHI->SetVertexShader(handle);
}

void CRenderBackend::SetPixelShader(RHI_ShaderHandle handle)
{
	if (m_pRHI)
		m_pRHI->SetPixelShader(handle);
}

void CRenderBackend::SetSampler(uint32_t slot, const RHI_SamplerDesc& desc)
{
	if (m_pRHI)
		m_pRHI->SetSampler(slot, desc);
}

void CRenderBackend::SetShaderResource(uint32_t slot, RHI_TextureHandle tex)
{
	if (m_pRHI)
		m_pRHI->SetShaderResource(slot, tex);
}

void CRenderBackend::SetShaderConstants(RHI_ShaderType stage, uint32_t startRegister, const float* data, uint32_t vec4Count)
{
	if (m_pRHI)
		m_pRHI->SetShaderConstants(stage, startRegister, data, vec4Count);
}
////////////////////////////////////////////////////////////////////////////////
