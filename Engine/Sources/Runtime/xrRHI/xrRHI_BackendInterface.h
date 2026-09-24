#pragma once

#include "framework.h"
#include "xrRHI_Internal.h"

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

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<u32, u32>>& outResolutions) const = 0;

	virtual RHI_Format GetBackBufferFormat() const = 0;

	virtual void Clear(u32 clearFlags, const fvec4 color, float depth, u8 stencil) = 0;

	virtual RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc, const void* initialData = nullptr) = 0;
	virtual void DestroyTexture(RHI_TextureHandle handle) = 0;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) = 0;
	virtual void* GetTextureNativeHandle(RHI_TextureHandle handle) = 0;
	virtual bool GetCubeMapFaceNative(RHI_TextureHandle handle, u32 face, u32 level, void** outSurface) = 0;
};

#ifdef __cplusplus
extern "C"
{
#endif

	XRRHI_API IRenderBackend* CreateRenderBackend(RHI_BackendType type);

#ifdef __cplusplus
}
#endif
