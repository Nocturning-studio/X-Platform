////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
#include "xrRenderBackendAPI.h"
#include "Resources/Textures/Texture.h"
#include "Resources/Shaders/ShaderPass.h"
#include "Resources/ResourceManager.h"
////////////////////////////////////////////////////////////////////////////////
enum class RHI_BackendType : uint32_t;
struct RHI_PresentationParams;
struct RHIDeviceCaps;
class IRenderBackend;
struct IDirect3DDevice9Ex;
////////////////////////////////////////////////////////////////////////////////
class XRRB_API CRenderBackend
{
  public:
	CRenderBackend();
	~CRenderBackend();

	CRenderBackend(const CRenderBackend&) = delete;
	CRenderBackend& operator=(const CRenderBackend&) = delete;

	// =====================================================================
	// Lifecycle
	// =====================================================================
	bool CreateDevice(HWND hWnd, RHI_BackendType backendType, const RHI_PresentationParams& params);
	void DestroyDevice();

	bool ResetDevice(const RHI_PresentationParams& params);
	bool NeedReset() const;
	RHI_DeviceStatus CheckDeviceStatus() const;

	// =====================================================================
	// Frame
	// =====================================================================
	void BeginFrame();
	void EndFrame();
	void Present();
	bool OnDeviceReset();
	bool IsInScene() const { return m_inScene; }

	// =====================================================================
	// Device info
	// =====================================================================
	IRenderBackend* GetRHI() const { return m_pRHI; }

	// DEPRECATED: raw D3D9 device. Используется только legacy-кодом.
	DEPRECATED IDirect3DDevice9Ex* GetDevice() const;

	const RHIDeviceCaps& GetDeviceCaps() const;
	RHI_Format GetBackBufferFormat() const;
	const RHI_PresentationParams& GetPresentParams() const { return m_presentParams; }
	bool IsReady() const { return m_pRHI != nullptr; }
	uint32_t GetBackBufferWidth() const;
	uint32_t GetBackBufferHeight() const;
	void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const;

	// =====================================================================
	// Subsystems
	// =====================================================================
	CResourceManager& Resources() { return m_resources; }
	const CResourceManager& Resources() const { return m_resources; }

	// =====================================================================
	// Render targets
	// =====================================================================
	RHI_RenderTargetView CreateRTV(RHI_TextureHandle tex, uint32_t mip = 0, uint32_t face = 0);
	RHI_DepthStencilView CreateDSV(RHI_TextureHandle tex, uint32_t mip = 0, uint32_t face = 0);
	void DestroyRTV(RHI_RenderTargetView rtv);
	void DestroyDSV(RHI_DepthStencilView dsv);

	RHI_RenderTargetView GetBackBufferRTV() const;
	RHI_DepthStencilView GetBackBufferDSV() const;

	void SetRenderTargets(const RHI_RenderTargetView* rtvs, uint32_t count, RHI_DepthStencilView dsv);
	void ClearRenderTarget(RHI_RenderTargetView rtv, const fvec4& color);
	void ClearDepthStencil(RHI_DepthStencilView dsv, float depth, uint8_t stencil);

	// =====================================================================
	// Pipeline state (PSO)
	// =====================================================================
	RHI_PipelineStateHandle CreatePipelineState(const RHI_PipelineStateDesc& desc);
	void DestroyPipelineState(RHI_PipelineStateHandle handle);
	void SetPipelineState(RHI_PipelineStateHandle handle);
	RHI_PipelineStateHandle GetCurrentPipelineState() const;

	RHI_PipelineLayoutHandle CreatePipelineLayout(const RHI_PipelineLayoutDesc& desc);
	void DestroyPipelineLayout(RHI_PipelineLayoutHandle handle);

	// Dynamic PSO-adjacent state
	void SetStencilRef(uint8_t ref);
	void SetBlendFactor(const fvec4& factor);
	void SetDepthBounds(float minZ, float maxZ);

	// Dynamic rasterizer
	void SetViewport(const RHI_Viewport& vp);
	void SetScissorRect(const RHI_Rect* rect);
	RHI_Viewport GetViewport() const;
	bool GetScissorRect(RHI_Rect& out) const;

	// Dynamic geometry binding
	void SetVertexBuffer(uint32_t slot, RHI_BufferHandle vb, uint32_t offset, uint32_t stride);
	void SetIndexBuffer(RHI_BufferHandle ib, RHI_IndexFormat fmt);
	void SetPrimitiveTopology(RHI_Topology topology);

	// =====================================================================
	// Draw
	// =====================================================================
	void Draw(uint32_t vertexCount, uint32_t startVertex = 0);
	void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, uint32_t baseVertex = 0);
	void DrawFullscreen()
	{
		if(m_pRHI)
			m_pRHI->DrawFullscreen();
	}

	// =====================================================================
	// Shaders
	// =====================================================================
	RHI_ShaderCompileResult CompileShader(const RHI_ShaderCompileDesc& desc);
	RHI_ShaderHandle CreateVertexShader(const void* bytecode, size_t size, const char* debugName = nullptr);
	RHI_ShaderHandle CreatePixelShader(const void* bytecode, size_t size, const char* debugName = nullptr);
	void DestroyShader(RHI_ShaderHandle handle);
	bool GetShaderBytecode(RHI_ShaderHandle handle, const void** outData, size_t* outSize);

	// =====================================================================
	// Samplers and shader resources
	// =====================================================================
	void SetSampler(uint32_t slot, const RHI_SamplerDesc& desc);
	void SetShaderResource(uint32_t slot, RHI_TextureHandle tex);
	void SetShaderConstants(RHI_ShaderType stage, uint32_t startRegister, const float* data, uint32_t vec4Count);

	// =====================================================================
	// High-level resource creation (через CResourceManager)
	// =====================================================================
	ref_texture CreateRenderTarget(uint32_t w, uint32_t h, RHI_Format fmt, uint32_t mips = 1);
	ref_texture CreateDepthStencil(uint32_t w, uint32_t h, RHI_Format fmt = RHI_Format::D24_UNORM_S8_UINT);

	ref_vertexdecl CreateVertexDeclaration(const RHI_InputLayoutDesc& layout);
	ref_vertexbuffer CreateVertexBuffer(const RHI_BufferDesc& desc, const void* data = nullptr);
	ref_indexbuffer CreateIndexBuffer(const RHI_BufferDesc& desc, const void* data = nullptr);
	ref_geometry CreateGeometry();

	void BindGeometry(const ref_geometry& g);
	void DrawGeometry(const ref_geometry& g);

	// =====================================================================
	// Clear (текущий привязанный target)
	// =====================================================================
	void Clear(uint32_t flags, uint32_t colorARGB = 0, float z = 1.0f, uint32_t stencil = 0);
	void Clear(uint32_t flags, fvec4 colorRGBA, float z = 1.0f, uint32_t stencil = 0);

	// =====================================================================
	// Shader pass
	// =====================================================================
	//
	// SetShaderPass теперь применяет только resource bindings (samplers,
	// SRV, constants). Шейдеры выставляются через SetPipelineState.
	//
	void SetShaderPass(CShaderPass* pass);
	void SetShaderPass(CShaderPass& pass) { SetShaderPass(&pass); }

	// =====================================================================
	// PSO helpers
	// =====================================================================
	//
	// Собрать desc из пары (shaderPass, geometry) + состояний.
	// Удобно, когда все поля уже известны вызывающему коду.
	//
	RHI_PipelineStateDesc BuildPipelineDesc(const CShaderPass& pass,
											const CGeometry& geom,
											const RHI_BlendState& blend = {},
											const RHI_DepthStencilState& depthStencil = {},
											const RHI_RasterizerState& rasterizer = {},
											RHI_Format rtvFormat = RHI_Format::RGBA8_UNORM,
											RHI_Format dsvFormat = RHI_Format::D24_UNORM_S8_UINT);

	// Тот же сбор, но сразу создаёт (через кэш) и применяет PSO.
	// Возвращает handle, чтобы вызывающий мог его сохранить, если хочет.
	RHI_PipelineStateHandle ApplyPipeline(const CShaderPass& pass,
										  const CGeometry& geom,
										  const RHI_BlendState& blend = {},
										  const RHI_DepthStencilState& depthStencil = {},
										  const RHI_RasterizerState& rasterizer = {},
										  RHI_Format rtvFormat = RHI_Format::RGBA8_UNORM,
										  RHI_Format dsvFormat = RHI_Format::D24_UNORM_S8_UINT);

	// =====================================================================
	// Misc
	// =====================================================================
	void InvalidateCache();

  private:
	bool LoadRHIModule(RHI_BackendType type);

  private:
	HMODULE m_hRHI = nullptr;
	IRenderBackend* m_pRHI = nullptr;

	RHI_PresentationParams m_presentParams{};

	CResourceManager m_resources;
	bool m_inScene = false;
};
////////////////////////////////////////////////////////////////////////////////
