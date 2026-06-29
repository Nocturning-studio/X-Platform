#pragma once
#include "framework.h"
#include "xrRHI_Internal.h"
#include "xrRHI_Debug.h"
#include "xrRHI_Types.h"
#include "xrRHI_Caps.h"
#include "xrRHI_Handles.h"

RHI_BEGIN

class XRRHI_API IRenderBackend
{
  public:
	virtual ~IRenderBackend()
	{
	}

	virtual bool CreateDevice(HWND hWnd, const RHIPresentationParams& params) = 0;
	virtual void DestroyDevice() = 0;
	virtual bool Reset(const RHIPresentationParams& params) = 0;
	virtual void Present() = 0;

	virtual void* GetDeviceHandle() = 0;
	virtual void* GetD3DHandle()
	{
		return nullptr;
	}

	virtual const RHIDeviceCaps& GetDeviceCaps() const = 0;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<u32, u32>>& outResolutions) const = 0;

	virtual RHI_Format GetBackBufferFormat() const = 0;

	virtual void Clear(u32 clearFlags, const fvec4 color, float depth, u8 stencil) = 0;

	virtual TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr) = 0;
	virtual void DestroyTexture(TextureHandle handle) = 0;
	virtual void SetTexture(u32 slot, TextureHandle handle, SamplerHandle sampler = SamplerHandle{}) = 0;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) = 0;
	virtual void* GetTextureNativeHandle(TextureHandle handle) = 0;
	virtual bool GetCubeMapFaceNative(TextureHandle handle, u32 face, u32 level, void** outSurface) = 0;

	virtual SamplerHandle CreateSampler(const SamplerDesc& desc) = 0;
	virtual void DestroySampler(SamplerHandle handle) = 0;
};

RHI_END

#ifdef __cplusplus
extern "C"
{
#endif

	XRRHI_API xrRHI::IRenderBackend* CreateRenderBackend(xrRHI::BackendType type);

#ifdef __cplusplus
}
#endif
