#pragma once
#include "framework.h"
#include "xrRHI_Internal.h"
#include "xrRHI_Debug.h"
#include "xrRHI_Types.h"

RHI_BEGIN

class XRRHI_API IRenderBackend
{
  public:
	virtual ~IRenderBackend()
	{
	}

	// Создание и уничтожение устройства
	virtual bool CreateDevice(HWND hWnd, const RHIPresentationParams& params) = 0;
	virtual void DestroyDevice() = 0;
	virtual bool Reset(const RHIPresentationParams& params) = 0;
	virtual void Present() = 0;

	virtual void* GetDeviceHandle() = 0;
	virtual void* GetD3DHandle()
	{
		return nullptr;
	}

	virtual void GetDeviceCaps(void* pCaps) = 0;

	virtual void Clear(u32 clearFlags, const fvec4 color, float depth, u8 stencil) = 0;

	virtual RHITexture CreateTexture(const TextureDesc& desc, const void* initialData = nullptr) = 0;
	virtual void DestroyTexture(RHITexture texture) = 0;
	virtual void SetTexture(u32 slot, RHITexture texture, RHISampler sampler = nullptr) = 0;

	virtual RHISampler CreateSampler(const SamplerDesc& desc) = 0;
	virtual void DestroySampler(RHISampler sampler) = 0;
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
