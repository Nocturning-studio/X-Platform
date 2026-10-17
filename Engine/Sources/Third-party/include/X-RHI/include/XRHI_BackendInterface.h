////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
class XRHI_API IRenderBackend
{
  public:
	virtual ~IRenderBackend() = default;

	virtual bool CreateDevice(HWND hWnd, const RHI_PresentationParams& params) = 0;
	virtual void DestroyDevice() = 0;
	virtual bool Reset(const RHI_PresentationParams& params) = 0;
	virtual RHI_DeviceStatus CheckDeviceStatus() const = 0;
	virtual void Present() = 0;

	virtual void OnFrameBegin() = 0;
	virtual void OnFrameEnd() = 0;

	virtual void* GetDeviceHandle() = 0;
	virtual void* GetD3DHandle() { return nullptr; }
	virtual RHI_BackendType GetBackendType() const = 0;

	virtual const RHIDeviceCaps& GetDeviceCaps() const = 0;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const = 0;

	virtual uint32_t GetBackBufferWidth() const = 0;
	virtual uint32_t GetBackBufferHeight() const = 0;
	virtual RHI_Format GetBackBufferFormat() const = 0;

	virtual RHI_RenderTargetView CreateRTV(RHI_TextureHandle tex, uint32_t mip = 0, uint32_t face = 0) = 0;
	virtual RHI_DepthStencilView CreateDSV(RHI_TextureHandle tex, uint32_t mip = 0, uint32_t face = 0) = 0;
	virtual RHI_RenderTargetView GetBackBufferRTV() const = 0;
	virtual RHI_DepthStencilView GetBackBufferDSV() const = 0;
	virtual void DestroyRTV(RHI_RenderTargetView rtv) = 0;
	virtual void DestroyDSV(RHI_DepthStencilView dsv) = 0;

	virtual void SetRenderTargets(const RHI_RenderTargetView* rtvs, uint32_t count, RHI_DepthStencilView dsv) = 0;
	virtual void ClearRenderTarget(RHI_RenderTargetView rtv, const Float4& color) = 0;
	virtual void ClearDepthStencil(RHI_DepthStencilView dsv, float depth, uint8_t stencil) = 0;
	virtual void Clear(uint32_t clearFlags, const Float4 color, float depth, uint8_t stencil) = 0;

	virtual RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc, const void* initialData = nullptr) = 0;
	virtual void DestroyTexture(RHI_TextureHandle handle) = 0;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) = 0;
	virtual void* GetTextureNativeHandle(RHI_TextureHandle handle) = 0;
	virtual bool GetCubeMapFaceNative(RHI_TextureHandle handle, uint32_t face, uint32_t level, void** outSurface) = 0;

	virtual RHI_BufferHandle CreateVertexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr) = 0;
	virtual RHI_BufferHandle CreateIndexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr) = 0;
	virtual void DestroyBuffer(RHI_BufferHandle handle) = 0;

	virtual void* LockBuffer(RHI_BufferHandle handle, uint32_t offset, uint32_t size, uint32_t flags) = 0;
	virtual void UnlockBuffer(RHI_BufferHandle handle) = 0;

	virtual RHI_InputLayoutHandle CreateInputLayout(const RHI_InputLayoutDesc& desc) = 0;
	virtual void DestroyInputLayout(RHI_InputLayoutHandle handle) = 0;

	virtual RHI_PipelineLayoutHandle CreatePipelineLayout(const RHI_PipelineLayoutDesc& desc) = 0;
	virtual void DestroyPipelineLayout(RHI_PipelineLayoutHandle handle) = 0;

	virtual RHI_PipelineStateHandle CreatePipelineState(const RHI_PipelineStateDesc& desc) = 0;
	virtual void DestroyPipelineState(RHI_PipelineStateHandle handle) = 0;
	virtual void SetPipelineState(RHI_PipelineStateHandle handle) = 0;
	virtual RHI_PipelineStateHandle GetCurrentPipelineState() const = 0;

	virtual void InvalidateCache() = 0;

	virtual RHI_ShaderCompileResult CompileShader(const RHI_ShaderCompileDesc& desc) = 0;
	virtual bool GetShaderBytecode(RHI_ShaderHandle handle, const void** outData, size_t* outSize) = 0;
	virtual RHI_ShaderHandle CreateVertexShader(const void* bytecode, size_t size, const char* debugName = nullptr) = 0;
	virtual RHI_ShaderHandle CreatePixelShader(const void* bytecode, size_t size, const char* debugName = nullptr) = 0;
	virtual void DestroyShader(RHI_ShaderHandle handle) = 0;

	virtual void SetSampler(uint32_t slot, const RHI_SamplerDesc& desc) = 0;
	virtual void SetShaderResource(uint32_t slot, RHI_TextureHandle tex) = 0;
	virtual void SetShaderConstants(RHI_ShaderType stage, uint32_t startRegister, const float* data, uint32_t vec4Count) = 0;

	virtual void SetVertexBuffer(uint32_t slot, RHI_BufferHandle vb, uint32_t offset, uint32_t stride) = 0;
	virtual void SetIndexBuffer(RHI_BufferHandle ib, RHI_IndexFormat fmt) = 0;
	virtual void SetPrimitiveTopology(RHI_Topology topology) = 0;

	virtual void* GetShaderNativeHandle(RHI_ShaderHandle handle) { return nullptr; }
	virtual void* GetInputLayoutNativeHandle(RHI_InputLayoutHandle handle) { return nullptr; }

	virtual void SetViewport(const RHI_Viewport& vp) = 0;
	virtual void SetScissorRect(const RHI_Rect* rect) = 0;
	virtual RHI_Viewport GetViewport() const = 0;
	virtual bool GetScissorRect(RHI_Rect& out) const = 0;

	virtual void SetStencilRef(uint8_t ref) = 0;
	virtual void SetBlendFactor(const Float4& factor) = 0;
	virtual void SetDepthBounds(float minZ, float maxZ) = 0;

	virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, uint32_t baseVertex = 0) = 0;
	virtual void DrawFullscreen() = 0;
};
////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"
{
#endif

	XRHI_API IRenderBackend* CreateRenderBackend(RHI_BackendType type);

#ifdef __cplusplus
}
#endif
////////////////////////////////////////////////////////////////////////////////
