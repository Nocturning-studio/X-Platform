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
	virtual RHI_DeviceStatus CheckDeviceStatus() const override;
	virtual void Present() override;

	virtual void OnFrameBegin() override;
	virtual void OnFrameEnd() override;

	DX_DEPRECATED virtual void* GetDeviceHandle() override { return m_pDevice; }
	DX_DEPRECATED virtual void* GetD3DHandle() override { return m_pD3D; }

	virtual const RHIDeviceCaps& GetDeviceCaps() const override;

	virtual RHI_RenderTargetView CreateRTV(RHI_TextureHandle tex, u32 mip = 0, u32 face = 0) override;
	virtual RHI_DepthStencilView CreateDSV(RHI_TextureHandle tex, u32 mip = 0, u32 face = 0) override;
	virtual RHI_RenderTargetView GetBackBufferRTV() const override;
	virtual RHI_DepthStencilView GetBackBufferDSV() const override;
	virtual void DestroyRTV(RHI_RenderTargetView rtv) override;
	virtual void DestroyDSV(RHI_DepthStencilView dsv) override;

	virtual void SetRenderTargets(const RHI_RenderTargetView* rtvs, uint32_t count, RHI_DepthStencilView dsv) override;
	virtual void ClearRenderTarget(RHI_RenderTargetView rtv, const fvec4& color) override;
	virtual void ClearDepthStencil(RHI_DepthStencilView dsv, float depth, uint8_t stencil) override;
	virtual void Clear(uint32_t clearFlags, const fvec4 color, float depth, uint8_t stencil) override;

	virtual void GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const override;

	virtual u32 GetBackBufferWidth() const override { return m_backBufferWidth; }
	virtual u32 GetBackBufferHeight() const override { return m_backBufferHeight; }
	virtual RHI_Format GetBackBufferFormat() const override;

	RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc, const void* initialData = nullptr) override;
	void DestroyTexture(RHI_TextureHandle handle) override;
	virtual bool CheckFormatSupport(RHI_Format fmt, bool isRenderTarget, bool isDepthStencil, bool isCube = false) override;
	virtual void* GetTextureNativeHandle(RHI_TextureHandle handle) override;
	virtual bool GetCubeMapFaceNative(RHI_TextureHandle handle, uint32_t face, uint32_t level, void** outSurface) override;

	virtual void SetBlendState(const RHI_BlendState& state) override;
	virtual const RHI_BlendState& GetBlendState() const override { return m_blendCache; }
	virtual void SetDepthStencilState(const RHI_DepthStencilState& state) override;
	virtual const RHI_DepthStencilState& GetDepthStencilState() const override { return m_depthCache; }
	virtual void SetRasterizerState(const RHI_RasterizerState& state) override;
	virtual const RHI_RasterizerState& GetRasterizerState() const override { return m_rasterCache; }

	virtual void SetViewport(const RHI_Viewport& vp) override;
	virtual RHI_Viewport GetViewport() const override;
	virtual void SetScissorRect(const RHI_Rect* rect) override;
	virtual bool GetScissorRect(RHI_Rect& out) const override;

	virtual void InvalidateStateCache() override;

	virtual void SetShaderResource(uint32_t slot, RHI_TextureHandle tex) override;

  private:
	IDirect3D9Ex* m_pD3D;
	IDirect3DDevice9Ex* m_pDevice;
	D3DPRESENT_PARAMETERS m_PP;
	RHIDeviceCaps m_DeviceCaps;
	D3DADAPTER_IDENTIFIER9 m_AdapterID;
	D3DDISPLAYMODE m_DesktopMode;
	u32 m_backBufferWidth = 0;
	u32 m_backBufferHeight = 0;
	D3DFORMAT m_BackBufferFmt;
	HWND m_hWnd;
	UINT m_DesktopRefreshRate = 60;

	std::vector<DX9Texture*> m_Textures;
	std::stack<uint32_t> m_FreeTextureIndices;

	RHI_BlendState m_blendCache{};
	RHI_DepthStencilState m_depthCache{};
	RHI_RasterizerState m_rasterCache{};

	bool m_blendCacheValid = false;
	bool m_depthCacheValid = false;
	bool m_rasterCacheValid = false;

	RHI_Viewport m_viewportCache{};
	bool m_viewportCacheValid = false;

	RHI_Rect m_scissorCache{};
	bool m_scissorCacheValid = false;
	bool m_scissorEnabled = false;

	void FillPresentParams(const RHI_PresentationParams& params, D3DFORMAT backBufferFmt, D3DFORMAT depthStencilFmt, UINT fullscreenRefreshHz);
	void CacheDeviceCapsFromD3D();
	void CacheBackBufferDimensions();
	bool DetermineDepthAndBackBufferFormatsFromPresentParams(const RHI_PresentationParams& params, D3DFORMAT& outBackBufferFmt, D3DFORMAT& outDepthStencilFmt);
	D3DFORMAT SelectDepthStencilFormat(D3DFORMAT backBufferFmt) const;

	RHI_TextureHandle AllocRHI_TextureHandle(DX9Texture* tex);
	DX9Texture* GetTexture(RHI_TextureHandle handle);
	void FreeRHI_TextureHandle(RHI_TextureHandle handle);

	void ReleaseAllResources();

	struct SDX9SurfaceSlot
	{
		IDirect3DSurface9* surface = nullptr;
	};

	std::vector<SDX9SurfaceSlot> m_rtvSlots;
	std::vector<SDX9SurfaceSlot> m_dsvSlots;
	std::stack<uint32_t> m_freeRTVSlots;
	std::stack<uint32_t> m_freeDSVSlots;

	RHI_RenderTargetView m_backBufferRTV{};
	RHI_DepthStencilView m_backBufferDSV{};

	// Кэш последних привязанных RT/DS. Нужен для дедупликации вызовов
	// SetRenderTarget/SetDepthStencilSurface.
	IDirect3DSurface9* m_currentRTASurfaces[4] = {};
	IDirect3DSurface9* m_currentDSSurface = nullptr;

	// --- RTV/DSV helpers ---
	uint32_t AllocRTVSlot(IDirect3DSurface9* surf);
	uint32_t AllocDSVSlot(IDirect3DSurface9* surf);
	void FreeRTVSlot(uint32_t id);
	void FreeDSVSlot(uint32_t id);

	IDirect3DSurface9* ResolveRTASurface(RHI_RenderTargetView rtv) const;
	IDirect3DSurface9* ResolveDSSurface(RHI_DepthStencilView dsv) const;

	IDirect3DSurface9* GetTextureSurfaceForRT(DX9Texture* tex, uint32_t mip, uint32_t face) const;

	void RefreshBackBufferRTVs();
	void ReleaseAllRTVDSV();
	void InvalidateRenderTargetCache();
	void InvalidateUserRTVDSVOnReset();
};
////////////////////////////////////////////////////////////////////////////////
