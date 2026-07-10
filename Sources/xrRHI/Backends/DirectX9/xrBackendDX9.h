#pragma once

#include <d3d9.h>
#include <d3dx9.h>

#include "../../xrRHI.h"
#include "xrBackendDX9_Internal.h"

#pragma warning(push)
#pragma warning(disable : 4251)

RHI_BEGIN

class XRRHI_API CRenderBackendDX9 : public IRenderBackend
{
  public:
	CRenderBackendDX9();
	virtual ~CRenderBackendDX9();

	// IRenderBackend
	virtual bool CreateDevice(HWND hWnd, const RHIPresentationParams& params) override;
	virtual void DestroyDevice() override;
	virtual bool Reset(const RHIPresentationParams& params) override;
	virtual void Present() override;

	virtual void OnFrameBegin() override;
	virtual void OnFrameEnd() override;

	DEPRECATED  virtual void* GetDeviceHandle() override
	{
		return m_pDevice;
	}
	DEPRECATED virtual void* GetD3DHandle() override
	{
		return m_pD3D;
	}
	virtual const RHIDeviceCaps& GetDeviceCaps() const override;
	virtual void Clear(u32 clearFlags, const fvec4 color, float depth, u8 stencil) override;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<u32, u32>>& outResolutions) const override;

	virtual RHI_Format GetBackBufferFormat() const override;

	TextureHandle CreateTexture(const TextureDesc& desc, const void* initialData = nullptr) override;
	void DestroyTexture(TextureHandle handle) override;
	void SetTexture(u32 slot, TextureHandle texture, SamplerHandle sampler = SamplerHandle{}) override;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) override;
	virtual void* GetTextureNativeHandle(TextureHandle handle) override;
	virtual bool GetCubeMapFaceNative(TextureHandle handle, u32 face, u32 level, void** outSurface) override;

	SamplerHandle CreateSampler(const SamplerDesc& desc) override;
	void DestroySampler(SamplerHandle handle) override;

	virtual ShaderHandle CreateShader(ShaderType type, const void* bytecode, size_t bytecodeSize) override;
	virtual void DestroyShader(ShaderHandle handle) override;
	virtual void SetShader(ShaderType type, ShaderHandle handle) override;

	virtual ConstantBufferHandle CreateConstantBuffer(u32 size) override;
	virtual void DestroyConstantBuffer(ConstantBufferHandle handle) override;
	virtual void UpdateConstantBuffer(ConstantBufferHandle handle, u32 offset, const void* data, u32 size) override;
	virtual void SetShaderConstantBuffer(ShaderType type, u32 startRegister, ConstantBufferHandle handle) override;
	virtual ShaderConstantLayout ReflectConstantLayout(ShaderHandle handle) override;

  private:
	IDirect3D9Ex* m_pD3D;
	IDirect3DDevice9Ex* m_pDevice;
	D3DPRESENT_PARAMETERS m_PP;
	RHIDeviceCaps m_DeviceCaps;
	D3DADAPTER_IDENTIFIER9 m_AdapterID;
	D3DDISPLAYMODE m_DesktopMode;
	D3DFORMAT m_BackBufferFmt;
	HWND m_hWnd;
	UINT m_DesktopRefreshRate = 60;

	std::vector<DX9Texture*> m_Textures;
	std::stack<u32> m_FreeTextureIndices;

	std::vector<DX9Sampler*> m_Samplers;
	std::stack<u32> m_FreeSamplerIndices;

	std::vector<DX9Shader*> m_Shaders;
	std::stack<u32> m_FreeShaderIndices;
	std::vector<DX9ConstantBuffer*> m_ConstantBuffers;
	std::stack<u32> m_FreeConstantBufferIndices;

	void CacheDeviceCapsFromD3D();

	TextureHandle AllocTextureHandle(DX9Texture* tex);
	DX9Texture* GetTexture(TextureHandle handle);
	void FreeTextureHandle(TextureHandle handle);

	SamplerHandle AllocSamplerHandle(DX9Sampler* sampler);
	DX9Sampler* GetSampler(SamplerHandle handle);
	void FreeSamplerHandle(SamplerHandle handle);

	void ApplySampler(u32 slot, const SamplerDesc& desc);
	void ApplyDefaultSampler(u32 slot);

	ShaderHandle AllocShaderHandle(DX9Shader* shader);
	DX9Shader* GetShader(ShaderHandle handle);
	void FreeShaderHandle(ShaderHandle handle);

	ConstantBufferHandle AllocConstantBufferHandle(DX9ConstantBuffer* cb);
	DX9ConstantBuffer* GetConstantBuffer(ConstantBufferHandle handle);
	void FreeConstantBufferHandle(ConstantBufferHandle handle);

	void ReleaseAllResources();

	D3DFORMAT SelectDepthStencilFormat(D3DFORMAT backBufferFmt) const;

	bool DetermineDepthAndBackBufferFormatsFromPresentParams(const RHIPresentationParams& params,
		D3DFORMAT& outBackBufferFmt,
		D3DFORMAT& outDepthStencilFmt);

	void FillPresentParams(const RHIPresentationParams& params,
		D3DFORMAT backBufferFmt,
		D3DFORMAT depthStencilFmt,
		UINT fullscreenRefreshHz);

	DWORD SelectVertexProcessing();
};

RHI_END

#pragma warning(pop)