////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
#include "xrBackendDX9_Internal.h"
////////////////////////////////////////////////////////////////////////////////
class XRRHI_API CRenderBackendDX9 : public IRenderBackend
{
public:
	CRenderBackendDX9();
	virtual ~CRenderBackendDX9();

	virtual bool CreateDevice(HWND hWnd, const RHI_PresentationParams& params) override;
	virtual void DestroyDevice() override;
	virtual bool Reset(const RHI_PresentationParams& params) override;
	virtual RHI_DeviceStatus CheckDeviceStatus() const override;
	virtual void Present() override;

	virtual void OnFrameBegin() override;
	virtual void OnFrameEnd() override;

	DX_DEPRECATED virtual void* GetDeviceHandle() override { return m_pDevice; }
	DX_DEPRECATED virtual void* GetD3DHandle() override { return m_pD3D; }

	virtual const RHIDeviceCaps& GetDeviceCaps() const override;

	virtual RHI_RenderTargetView CreateRTV(RHI_TextureHandle tex, u32 mip = 0, u32 face = 0) override;
	virtual RHI_DepthStencilView CreateDSV(RHI_TextureHandle tex, u32 mip = 0, u32 face = 0) override;
	virtual RHI_RenderTargetView GetBackBufferRTV() const override;
	virtual RHI_DepthStencilView GetBackBufferDSV() const override;
	virtual void DestroyRTV(RHI_RenderTargetView rtv) override;
	virtual void DestroyDSV(RHI_DepthStencilView dsv) override;

	virtual void SetRenderTargets(const RHI_RenderTargetView* rtvs, uint32_t count, RHI_DepthStencilView dsv) override;
	virtual void ClearRenderTarget(RHI_RenderTargetView rtv, const fvec4& color) override;
	virtual void ClearDepthStencil(RHI_DepthStencilView dsv, float depth, uint8_t stencil) override;
	virtual void Clear(uint32_t clearFlags, const fvec4 color, float depth, uint8_t stencil) override;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const override;

	virtual u32 GetBackBufferWidth() const override { return m_backBufferWidth; }
	virtual u32 GetBackBufferHeight() const override { return m_backBufferHeight; }
	virtual RHI_Format GetBackBufferFormat() const override;

	RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc, const void* initialData = nullptr) override;
	void DestroyTexture(RHI_TextureHandle handle) override;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) override;
	virtual void* GetTextureNativeHandle(RHI_TextureHandle handle) override;
	virtual bool GetCubeMapFaceNative(RHI_TextureHandle handle, uint32_t face, uint32_t level, void** outSurface) override;

	virtual RHI_BufferHandle CreateVertexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr) override;
	virtual RHI_BufferHandle CreateIndexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr) override;
	virtual void DestroyBuffer(RHI_BufferHandle handle) override;

	virtual void* LockBuffer(RHI_BufferHandle handle, uint32_t offset, uint32_t size, uint32_t flags) override;
	virtual void UnlockBuffer(RHI_BufferHandle handle) override;

	virtual RHI_InputLayoutHandle CreateInputLayout(const RHI_InputLayoutDesc& desc) override;
	virtual void DestroyInputLayout(RHI_InputLayoutHandle handle) override;

	virtual void SetVertexBuffer(uint32_t slot, RHI_BufferHandle vb, uint32_t offset, uint32_t stride) override;
	virtual void SetIndexBuffer(RHI_BufferHandle ib, RHI_IndexFormat fmt) override;
	virtual void SetInputLayout(RHI_InputLayoutHandle layout) override;
	virtual void SetPrimitiveTopology(RHI_Topology topology) override;

	virtual void SetBlendState(const RHI_BlendState& state) override;
	virtual const RHI_BlendState& GetBlendState() const override { return m_blendCache; }
	virtual void SetDepthStencilState(const RHI_DepthStencilState& state) override;
	virtual const RHI_DepthStencilState& GetDepthStencilState() const override { return m_depthCache; }
	virtual void SetRasterizerState(const RHI_RasterizerState& state) override;
	virtual const RHI_RasterizerState& GetRasterizerState() const override { return m_rasterCache; }

	virtual void SetViewport(const RHI_Viewport& vp) override;
	virtual RHI_Viewport GetViewport() const override;
	virtual void SetScissorRect(const RHI_Rect* rect) override;
	virtual bool GetScissorRect(RHI_Rect& out) const override;

	virtual void InvalidateStateCache() override;

	virtual RHI_ShaderCompileResult CompileShader(const RHI_ShaderCompileDesc& desc) override;
	virtual bool GetShaderBytecode(RHI_ShaderHandle handle, const void** outData, size_t* outSize) override;
	virtual RHI_ShaderHandle CreateVertexShader(const void* bytecode, size_t size, const char* debugName = nullptr) override;
	virtual RHI_ShaderHandle CreatePixelShader(const void* bytecode, size_t size, const char* debugName = nullptr) override;
	virtual void SetVertexShader(RHI_ShaderHandle handle) override;
	virtual void SetPixelShader(RHI_ShaderHandle handle) override;
	virtual void SetSampler(uint32_t slot, const RHI_SamplerDesc& desc) override;
	virtual void SetShaderResource(uint32_t slot, RHI_TextureHandle tex) override;
	virtual void SetShaderConstants(RHI_ShaderType stage, uint32_t startRegister, const float* data, uint32_t vec4Count) override;
	virtual void DestroyShader(RHI_ShaderHandle handle) override;

	virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) override;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, uint32_t baseVertex = 0) override;
	virtual void DrawFullscreen() override;

private:
	// ============================================================================
	// Core D3D device / adapter / presentation
	// ============================================================================
	IDirect3D9Ex* m_pD3D;
	IDirect3DDevice9Ex* m_pDevice;
	HWND m_hWnd;
	D3DPRESENT_PARAMETERS m_PP;
	D3DADAPTER_IDENTIFIER9 m_AdapterID;
	D3DDISPLAYMODE m_DesktopMode;
	UINT m_DesktopRefreshRate = 60;
	RHIDeviceCaps m_DeviceCaps;

	// ============================================================================
	// Back buffer
	// ============================================================================
	u32 m_backBufferWidth = 0;
	u32 m_backBufferHeight = 0;
	D3DFORMAT m_BackBufferFmt;

	// ============================================================================
	// Resource pools
	// ============================================================================
	std::vector<DX9Texture*> m_Textures;
	std::stack<uint32_t> m_FreeTextureIndices;

	std::vector<DX9Buffer*> m_buffers;
	std::stack<uint32_t> m_freeBufferIndices;

	std::vector<DX9InputLayout*> m_inputLayouts;
	std::stack<uint32_t> m_freeInputLayoutIndices;

	std::vector<DX9Shader*> m_shaders;
	std::stack<uint32_t> m_freeShaderIndices;

	// ============================================================================
	// Geometry binding cache
	// ============================================================================
	static constexpr uint32_t kMaxVertexStreams = 16; // D3D9: 16 streams

	IDirect3DVertexDeclaration9* m_currentDecl = nullptr;
	RHI_Topology m_currentTopology = RHI_Topology::TriangleList;
	D3DPRIMITIVETYPE m_currentD3DTopology = D3DPT_TRIANGLELIST;

	IDirect3DVertexBuffer9* m_currentVB[kMaxVertexStreams] = {};
	uint32_t m_currentVBStride[kMaxVertexStreams] = {};
	uint32_t m_currentVBOffset[kMaxVertexStreams] = {};

	IDirect3DIndexBuffer9* m_currentIB = nullptr;
	uint32_t m_stream0VertexCount = 0;

	IDirect3DVertexBuffer9* m_fullscreenVB = nullptr;
	IDirect3DVertexDeclaration9* m_fullscreenDecl = nullptr;

	// ============================================================================
	// State caches
	// ============================================================================
	RHI_BlendState m_blendCache{};
	RHI_DepthStencilState m_depthCache{};
	RHI_RasterizerState m_rasterCache{};

	bool m_blendCacheValid = false;
	bool m_depthCacheValid = false;
	bool m_rasterCacheValid = false;

	RHI_Viewport m_viewportCache{};
	bool m_viewportCacheValid = false;

	RHI_Rect m_scissorCache{};
	bool m_scissorCacheValid = false;
	bool m_scissorEnabled = false;

	IDirect3DVertexShader9* m_currentVS = nullptr;
	IDirect3DPixelShader9* m_currentPS = nullptr;

	// ============================================================================
	// RTV / DSV storage and binding cache
	// ============================================================================
	struct SDX9SurfaceSlot
	{
		IDirect3DSurface9* surface = nullptr;
	};

	std::vector<SDX9SurfaceSlot> m_rtvSlots;
	std::vector<SDX9SurfaceSlot> m_dsvSlots;
	std::stack<uint32_t> m_freeRTVSlots;
	std::stack<uint32_t> m_freeDSVSlots;

	RHI_RenderTargetView m_backBufferRTV{};
	RHI_DepthStencilView m_backBufferDSV{};

	// Кэш последних привязанных RT/DS. Нужен для дедупликации вызовов
	// SetRenderTarget/SetDepthStencilSurface.
	IDirect3DSurface9* m_currentRTASurfaces[4] = {};
	IDirect3DSurface9* m_currentDSSurface = nullptr;

	// ============================================================================
	// Device / presentation helpers
	// ============================================================================
	void FillPresentParams(const RHI_PresentationParams& params,
						   D3DFORMAT backBufferFmt,
						   D3DFORMAT depthStencilFmt,
						   UINT fullscreenRefreshHz);

	void CacheDeviceCapsFromD3D();
	void CacheBackBufferDimensions();

	bool DetermineDepthAndBackBufferFormatsFromPresentParams(const RHI_PresentationParams& params,
															 D3DFORMAT& outBackBufferFmt,
															 D3DFORMAT& outDepthStencilFmt);

	D3DFORMAT SelectDepthStencilFormat(D3DFORMAT backBufferFmt) const;

	// ============================================================================
	// Resource handle helpers
	// ============================================================================
	RHI_TextureHandle AllocRHI_TextureHandle(DX9Texture* tex);
	DX9Texture* GetTexture(RHI_TextureHandle handle);
	void FreeRHI_TextureHandle(RHI_TextureHandle handle);

	RHI_BufferHandle AllocBufferHandle(DX9Buffer* buf);
	DX9Buffer* GetBuffer(RHI_BufferHandle h) const;
	void FreeBufferHandle(RHI_BufferHandle h);

	RHI_InputLayoutHandle AllocInputLayoutHandle(DX9InputLayout* lay);
	DX9InputLayout* GetInputLayout(RHI_InputLayoutHandle h) const;
	void FreeInputLayoutHandle(RHI_InputLayoutHandle h);

	RHI_ShaderHandle AllocShaderHandle(DX9Shader* sh);
	DX9Shader* GetShader(RHI_ShaderHandle h) const;
	void FreeShaderHandle(RHI_ShaderHandle h);

	// ============================================================================
	// Lifetime / invalidation
	// ============================================================================
	void ReleaseAllResources();
	void InvalidateGeometryCache();

	void ReleaseNativeDeviceResources();
	void RecreateNativeDeviceResources();

	// ============================================================================
	// RTV / DSV helpers
	// ============================================================================
	uint32_t AllocRTVSlot(IDirect3DSurface9* surf);
	uint32_t AllocDSVSlot(IDirect3DSurface9* surf);
	void FreeRTVSlot(uint32_t id);
	void FreeDSVSlot(uint32_t id);

	IDirect3DSurface9* ResolveRTASurface(RHI_RenderTargetView rtv) const;
	IDirect3DSurface9* ResolveDSSurface(RHI_DepthStencilView dsv) const;

	IDirect3DSurface9* GetTextureSurfaceForRT(DX9Texture* tex, uint32_t mip, uint32_t face) const;

	void RefreshBackBufferRTVs();
	void ReleaseAllRTVDSV();
	void InvalidateRenderTargetCache();
	void InvalidateUserRTVDSVOnReset();

	// ============================================================================
	// Screen quad
	// ============================================================================
	void CreateFullscreenGeometry();
	void DestroyFullscreenGeometry();
};
////////////////////////////////////////////////////////////////////////////////
