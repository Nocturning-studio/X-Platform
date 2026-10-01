////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrRHI_API.h"
////////////////////////////////////////////////////////////////////////////////
class XRRHI_API IRenderBackend
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

	virtual const RHIDeviceCaps& GetDeviceCaps() const = 0;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const = 0;

	virtual u32 GetBackBufferWidth() const = 0;
	virtual u32 GetBackBufferHeight() const = 0;
	virtual RHI_Format GetBackBufferFormat() const = 0;

	virtual RHI_RenderTargetView CreateRTV(RHI_TextureHandle tex, u32 mip = 0, u32 face = 0) = 0;
	virtual RHI_DepthStencilView CreateDSV(RHI_TextureHandle tex, u32 mip = 0, u32 face = 0) = 0;
	virtual RHI_RenderTargetView GetBackBufferRTV() const = 0;
	virtual RHI_DepthStencilView GetBackBufferDSV() const = 0;
	virtual void DestroyRTV(RHI_RenderTargetView rtv) = 0;
	virtual void DestroyDSV(RHI_DepthStencilView dsv) = 0;

	virtual void SetRenderTargets(const RHI_RenderTargetView* rtvs, uint32_t count, RHI_DepthStencilView dsv) = 0;
	virtual void ClearRenderTarget(RHI_RenderTargetView rtv, const fvec4& color) = 0;
	virtual void ClearDepthStencil(RHI_DepthStencilView dsv, float depth, u8 stencil) = 0;
	virtual void Clear(uint32_t clearFlags, const fvec4 color, float depth, uint8_t stencil) = 0;

	virtual RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc, const void* initialData = nullptr) = 0;
	virtual void DestroyTexture(RHI_TextureHandle handle) = 0;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) = 0;
	virtual void* GetTextureNativeHandle(RHI_TextureHandle handle) = 0;
	virtual bool GetCubeMapFaceNative(RHI_TextureHandle handle, uint32_t face, uint32_t level, void** outSurface) = 0;

	virtual RHI_BufferHandle CreateVertexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr) = 0;
	virtual RHI_BufferHandle CreateIndexBuffer(const RHI_BufferDesc& desc, const void* initialData = nullptr) = 0;
	virtual void DestroyBuffer(RHI_BufferHandle handle) = 0;

	virtual void* LockBuffer(RHI_BufferHandle handle, uint32_t offset, uint32_t size, uint32_t flags) = 0;
	virtual void  UnlockBuffer(RHI_BufferHandle handle) = 0;

	virtual RHI_InputLayoutHandle CreateInputLayout(const RHI_InputLayoutDesc& desc) = 0;
	virtual void DestroyInputLayout(RHI_InputLayoutHandle handle) = 0;

	virtual void SetVertexBuffer(uint32_t slot, RHI_BufferHandle vb, uint32_t offset, uint32_t stride) = 0;
	virtual void SetIndexBuffer(RHI_BufferHandle ib, RHI_IndexFormat fmt) = 0;
	virtual void SetInputLayout(RHI_InputLayoutHandle layout) = 0;
	virtual void SetPrimitiveTopology(RHI_Topology topology) = 0;

	virtual void SetBlendState(const RHI_BlendState& state) = 0;
	virtual const RHI_BlendState& GetBlendState() const = 0;
	virtual void SetDepthStencilState(const RHI_DepthStencilState& state) = 0;
	virtual const RHI_DepthStencilState& GetDepthStencilState() const = 0;
	virtual void SetRasterizerState(const RHI_RasterizerState& state) = 0;
	virtual const RHI_RasterizerState& GetRasterizerState() const = 0;

	virtual void SetViewport(const RHI_Viewport& vp) = 0;
	virtual void SetScissorRect(const RHI_Rect* rect) = 0;
	virtual RHI_Viewport GetViewport() const = 0;
	virtual bool GetScissorRect(RHI_Rect& out) const = 0;

	virtual void InvalidateStateCache() = 0;

	virtual void SetShaderResource(uint32_t slot, RHI_TextureHandle tex) = 0;

	virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, uint32_t baseVertex = 0) = 0;
};
////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C"
{
#endif

	XRRHI_API IRenderBackend* CreateRenderBackend(RHI_BackendType type);

#ifdef __cplusplus
}
#endif
////////////////////////////////////////////////////////////////////////////////
