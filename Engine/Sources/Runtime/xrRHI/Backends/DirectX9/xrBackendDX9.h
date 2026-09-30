////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <d3d9.h>
#include <DXSDK/d3dx9.h>
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
	virtual void Present() override;

	virtual void OnFrameBegin() override;
	virtual void OnFrameEnd() override;

	DEPRECATED virtual void* GetDeviceHandle() override { return m_pDevice; }
	DEPRECATED virtual void* GetD3DHandle() override { return m_pD3D; }

	virtual const RHIDeviceCaps& GetDeviceCaps() const override;
	virtual void Clear(uint32_t clearFlags, const fvec4 color, float depth, uint8_t stencil) override;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const override;

	virtual RHI_Format GetBackBufferFormat() const override;

	RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc, const void* initialData = nullptr) override;
	void DestroyTexture(RHI_TextureHandle handle) override;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) override;
	virtual void* GetTextureNativeHandle(RHI_TextureHandle handle) override;
	virtual bool GetCubeMapFaceNative(RHI_TextureHandle handle, uint32_t face, uint32_t level, void** outSurface) override;

	virtual void SetBlendState(const RHI_BlendState& state) override;
	virtual void SetDepthStencilState(const RHI_DepthStencilState& state) override;
	virtual void SetRasterizerState(const RHI_RasterizerState& state) override;
	virtual void InvalidateStateCache() override;

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
	std::stack<uint32_t> m_FreeTextureIndices;

	RHI_BlendState        m_blendCache{};
	RHI_DepthStencilState m_depthCache{};
	RHI_RasterizerState   m_rasterCache{};

	bool m_blendCacheValid = false;
	bool m_depthCacheValid = false;
	bool m_rasterCacheValid = false;

	void CacheDeviceCapsFromD3D();

	RHI_TextureHandle AllocRHI_TextureHandle(DX9Texture* tex);
	DX9Texture* GetTexture(RHI_TextureHandle handle);
	void FreeRHI_TextureHandle(RHI_TextureHandle handle);

	void ReleaseAllResources();

	D3DFORMAT SelectDepthStencilFormat(D3DFORMAT backBufferFmt) const;

	bool DetermineDepthAndBackBufferFormatsFromPresentParams(const RHI_PresentationParams& params, D3DFORMAT& outBackBufferFmt, D3DFORMAT& outDepthStencilFmt);

	void FillPresentParams(const RHI_PresentationParams& params, D3DFORMAT backBufferFmt, D3DFORMAT depthStencilFmt, UINT fullscreenRefreshHz);
};
////////////////////////////////////////////////////////////////////////////////
