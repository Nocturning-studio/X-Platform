#pragma once
#include "xrRHI.h"
#include <d3d9.h>
#include <d3dx9.h>

RHI_BEGIN

struct RHITextureImpl
{
	IDirect3DTexture9* tex2D;
	IDirect3DCubeTexture9* texCube;
	IDirect3DSurface9* surface; // для рендер-таргета/depth-stencil
	RHI_Format format;
	u32 width;
	u32 height;
	bool isRenderTarget;
	bool isDepthStencil;
};

struct RHISamplerImpl
{
	SamplerDesc desc;
};

class XRRHI_API CRenderBackendDX9 : public IRenderBackend
{
  public:
	CRenderBackendDX9();
	virtual ~CRenderBackendDX9();

	// IRenderBackend
	virtual bool CreateDevice(HWND hWnd, bool windowed, int width, int height, int presentInterval) override;
	virtual void DestroyDevice() override;
	virtual bool Reset(HWND hWnd) override;
	virtual void Present() override;
	DEPRECATED  virtual void* GetDeviceHandle() override
	{
		return m_pDevice;
	}
	DEPRECATED virtual void* GetD3DHandle() override
	{
		return m_pD3D;
	}
	virtual void GetDeviceCaps(void* pCaps) override;
	virtual void Clear(u32 clearFlags, const float4 color, float depth, u8 stencil) override;

	RHITexture CreateTexture(const TextureDesc& desc, const void* initialData);
	void DestroyTexture(RHITexture texture);
	void SetTexture(u32 slot, RHITexture texture, RHISampler sampler);

	virtual RHISampler CreateSampler(const SamplerDesc& desc) override;
	virtual void DestroySampler(RHISampler sampler) override;

  private:
	IDirect3D9Ex* m_pD3D;
	IDirect3DDevice9Ex* m_pDevice;
	D3DPRESENT_PARAMETERS m_PP;
	D3DCAPS9 m_Caps;		   // кэш возможностей
	D3DFORMAT m_BackBufferFmt; // формат бэкбуфера
	HWND m_hWnd;			   // сохраняем для Reset

	D3DFORMAT RHIToD3DFormat(RHI_Format fmt) const;
	D3DFORMAT SelectDepthStencilFormat(D3DFORMAT backBufferFmt) const;

	void ApplySampler(u32 slot, RHISampler sampler);
};

RHI_END