#pragma once
#include "xrRHI.h"
#include <d3d9.h>
#include <d3dx9.h>

RHI_BEGIN

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
	virtual void Clear(u32 clearFlags, const float color[4], float depth, u8 stencil) override;

  private:
	IDirect3D9Ex* m_pD3D;
	IDirect3DDevice9Ex* m_pDevice;
	D3DPRESENT_PARAMETERS m_PP;
	D3DCAPS9 m_Caps;		   // кэш возможностей
	D3DFORMAT m_BackBufferFmt; // формат бэкбуфера
	HWND m_hWnd;			   // сохраняем для Reset

	// Конвертация RHI_Format в D3DFORMAT
	D3DFORMAT RHIToD3DFormat(RHI_Format fmt) const;

	// Выбор подходящего формата глубины/стенсила
	D3DFORMAT SelectDepthStencilFormat(D3DFORMAT backBufferFmt) const;
};

RHI_END