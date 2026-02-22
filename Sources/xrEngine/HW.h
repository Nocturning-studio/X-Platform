#pragma once
#include "hwcaps.h"
#include "../xrRHI/xrRHI.h"

class ENGINE_API CHW
{
  private:
	IDirect3D9Ex* pD3D;
	IDirect3DDevice9Ex* pDevice;

	IDirect3DSurface9* pBaseRT;
	IDirect3DSurface9* pBaseZB;

	CHWCaps Caps;

	UINT DevAdapter;
	D3DDEVTYPE DevT;
	D3DPRESENT_PARAMETERS DevPP;

	xrRHI::IRenderBackend* pBackend;

	HINSTANCE m_hRHI_DLL;

  public:
	CHW();
	~CHW();

	void CreateDevice(HWND hw);
	void DestroyDevice();
	void Reset(HWND hw);

	void selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed);
	u32 selectPresentInterval();
	u32 selectGPU();
	u32 selectRefresh(u32 dwWidth, u32 dwHeight, D3DFORMAT fmt);
	void updateWindowProps(HWND hw);
	BOOL support(D3DFORMAT fmt, DWORD type, DWORD usage);

	DEPRECATED IDirect3D9Ex* GetD3D() const
	{
		return pD3D;
	}
	DEPRECATED IDirect3DDevice9Ex* GetDevice() const
	{
		return pDevice;
	}
	DEPRECATED IDirect3DSurface9* GetBaseRT() const
	{
		return pBaseRT;
	}
	DEPRECATED IDirect3DSurface9* GetBaseZB() const
	{
		return pBaseZB;
	}
	DEPRECATED CHWCaps GetCaps()
	{
		return Caps;
	}
	DEPRECATED UINT GetDevAdapter()
	{
		return DevAdapter;
	}	
	DEPRECATED D3DDEVTYPE GetDevT()
	{
		return DevT;
	}	
	DEPRECATED D3DPRESENT_PARAMETERS GetDevPP()
	{
		return DevPP;
	}
};

extern ENGINE_API CHW HW;
