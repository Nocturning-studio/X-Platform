#pragma once

#include <d3d9.h>
#include <DXSDK/d3dx9.h>

#include <xrRHI/xrRHI.h>
#include "xrBackendDX9_Internal.h"

#pragma warning(push)
#pragma warning(disable : 4251)

class XRRHI_API CRenderBackendDX9 : public IRenderBackend
{
  public:
	CRenderBackendDX9();
	virtual ~CRenderBackendDX9();

	virtual bool CreateDevice(HWND hWnd, const RHI_PresentationParams& params) override;
	virtual void DestroyDevice() override;
	virtual bool Reset(const RHI_PresentationParams& params) override;
	virtual void Present() override;

	virtual void OnFrameBegin() override;
	virtual void OnFrameEnd() override;

	DEPRECATED virtual void* GetDeviceHandle() override { return m_pDevice; }
	DEPRECATED virtual void* GetD3DHandle() override { return m_pD3D; }

	virtual const RHIDeviceCaps& GetDeviceCaps() const override;
	virtual void Clear(u32 clearFlags, const fvec4 color, float depth, u8 stencil) override;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<u32, u32>>& outResolutions) const override;

	virtual RHI_Format GetBackBufferFormat() const override;

	RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc, const void* initialData = nullptr) override;
	void DestroyTexture(RHI_TextureHandle handle) override;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) override;
	virtual void* GetTextureNativeHandle(RHI_TextureHandle handle) override;
	virtual bool GetCubeMapFaceNative(RHI_TextureHandle handle, u32 face, u32 level, void** outSurface) override;

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

	void CacheDeviceCapsFromD3D();

	RHI_TextureHandle AllocRHI_TextureHandle(DX9Texture* tex);
	DX9Texture* GetTexture(RHI_TextureHandle handle);
	void FreeRHI_TextureHandle(RHI_TextureHandle handle);

	void ReleaseAllResources();

	D3DFORMAT SelectDepthStencilFormat(D3DFORMAT backBufferFmt) const;

	bool DetermineDepthAndBackBufferFormatsFromPresentParams(const RHI_PresentationParams& params, D3DFORMAT& outBackBufferFmt, D3DFORMAT& outDepthStencilFmt);

	void FillPresentParams(const RHI_PresentationParams& params, D3DFORMAT backBufferFmt, D3DFORMAT depthStencilFmt, UINT fullscreenRefreshHz);

	DWORD SelectVertexProcessing();
};

#pragma warning(pop)
