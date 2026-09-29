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
	virtual void Present() = 0;

	virtual void OnFrameBegin() = 0;
	virtual void OnFrameEnd() = 0;

	virtual void* GetDeviceHandle() = 0;
	virtual void* GetD3DHandle() { return nullptr; }

	virtual const RHIDeviceCaps& GetDeviceCaps() const = 0;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const = 0;

	virtual RHI_Format GetBackBufferFormat() const = 0;

	virtual void Clear(uint32_t clearFlags, const fvec4 color, float depth, uint8_t stencil) = 0;

	virtual RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc, const void* initialData = nullptr) = 0;
	virtual void DestroyTexture(RHI_TextureHandle handle) = 0;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) = 0;
	virtual void* GetTextureNativeHandle(RHI_TextureHandle handle) = 0;
	virtual bool GetCubeMapFaceNative(RHI_TextureHandle handle, uint32_t face, uint32_t level, void** outSurface) = 0;
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
