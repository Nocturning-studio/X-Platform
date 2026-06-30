#pragma once

#include "framework.h"
#include "xrRHI_Internal.h"

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

	virtual ShaderHandle CreateShader(ShaderType type, const void* bytecode, size_t bytecodeSize) = 0;
	virtual void DestroyShader(ShaderHandle handle) = 0;
	virtual void SetShader(ShaderType type, ShaderHandle handle) = 0;

	virtual ConstantBufferHandle CreateConstantBuffer(u32 sizeInBytes) = 0;
	virtual ConstantBufferHandle CreateConstantBuffer(const ShaderConstantLayout& layout) { return CreateConstantBuffer(layout.totalSize); }
	virtual void UpdateConstantBuffer(ConstantBufferHandle handle, u32 offset, const void* data, u32 size) = 0;
	void UpdateConstantBuffer(ConstantBufferHandle handle, const void* data, u32 size){ UpdateConstantBuffer(handle, 0, data, size); }
	virtual void DestroyConstantBuffer(ConstantBufferHandle handle) = 0;
	virtual void SetShaderConstantBuffer(ShaderType type, u32 startRegister, ConstantBufferHandle handle) = 0;
	virtual ShaderConstantLayout ReflectConstantLayout(ShaderHandle handle) = 0;
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
