#include "pch.h"
#include "xrBackendDX9.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

RHI_BEGIN

CRenderBackendDX9::CRenderBackendDX9() : m_pD3D(nullptr), m_pDevice(nullptr), m_hWnd(nullptr)
{
	ZeroMemory(&m_PP, sizeof(m_PP));
	ZeroMemory(&m_Caps, sizeof(m_Caps));
	m_BackBufferFmt = D3DFMT_UNKNOWN;
}

CRenderBackendDX9::~CRenderBackendDX9()
{
	DestroyDevice();
}

D3DFORMAT CRenderBackendDX9::RHIToD3DFormat(RHI_Format fmt) const
{
	switch (fmt)
	{
	case RHI_Format::RGBA8_UNORM:
		return D3DFMT_A8R8G8B8;
	case RHI_Format::A8_UNORM:
		return D3DFMT_A8;
	case RHI_Format::R8_UNORM:
		return D3DFMT_L8;
	case RHI_Format::RGBA16_FLOAT:
		return D3DFMT_A16B16G16R16F;
	case RHI_Format::RG16_FLOAT:
		return D3DFMT_G16R16F;
	case RHI_Format::R16_FLOAT:
		return D3DFMT_R16F;
	case RHI_Format::D16_UNORM:
		return D3DFMT_D16;
	case RHI_Format::D24_UNORM_S8_UINT:
		return D3DFMT_D24S8;
	case RHI_Format::D32_FLOAT:
		return D3DFMT_D32F_LOCKABLE;
	case RHI_Format::D15S1:
		return D3DFMT_D15S1;
	case RHI_Format::D24X8:
		return D3DFMT_D24X8;
	case RHI_Format::D32_LOCKABLE:
		return D3DFMT_D32;
	case RHI_Format::D24S8_Shadow:
		return (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z');
	case RHI_Format::D16_Shadow:
		return (D3DFORMAT)MAKEFOURCC('D', 'F', '1', '6');
	case RHI_Format::D24X4S4:
		return D3DFMT_D24X4S4;
	default:
		return D3DFMT_UNKNOWN;
	}
}

D3DFORMAT CRenderBackendDX9::SelectDepthStencilFormat(D3DFORMAT backBufferFmt) const
{
	if (SUCCEEDED(m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFmt, D3DUSAGE_DEPTHSTENCIL,
											D3DRTYPE_SURFACE, D3DFMT_D24S8)))
		return D3DFMT_D24S8;
	if (SUCCEEDED(m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFmt, D3DUSAGE_DEPTHSTENCIL,
											D3DRTYPE_SURFACE, D3DFMT_D24X8)))
		return D3DFMT_D24X8;
	if (SUCCEEDED(m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFmt, D3DUSAGE_DEPTHSTENCIL,
											D3DRTYPE_SURFACE, D3DFMT_D16)))
		return D3DFMT_D16;
	return D3DFMT_UNKNOWN;
}

bool CRenderBackendDX9::CreateDevice(HWND hWnd, bool windowed, int width, int height, int presentInterval)
{
	m_hWnd = hWnd;

	HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D);
	if (FAILED(hr) || !m_pD3D)
	{
		Msg("! [DX9] Direct3DCreate9Ex failed (0x%08x)", hr);
		return false;
	}

	D3DADAPTER_IDENTIFIER9 adapterID;
	m_pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterID);
	Msg("* [DX9] GPU [vendor:%X]-[device:%X]: %s", adapterID.VendorId, adapterID.DeviceId, adapterID.Description);

	D3DDISPLAYMODE d3ddm;
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	m_BackBufferFmt = d3ddm.Format;

	D3DFORMAT depthStencilFormat = SelectDepthStencilFormat(m_BackBufferFmt);
	if (depthStencilFormat == D3DFMT_UNKNOWN)
	{
		Msg("! [DX9] No suitable depth-stencil format found");
		return false;
	}

	ZeroMemory(&m_PP, sizeof(m_PP));
	m_PP.BackBufferWidth = width;
	m_PP.BackBufferHeight = height;
	m_PP.BackBufferFormat = m_BackBufferFmt;
	m_PP.BackBufferCount = 2; // как в оригинальном CHW
	m_PP.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_PP.MultiSampleQuality = 0;
	m_PP.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_PP.hDeviceWindow = hWnd;
	m_PP.Windowed = windowed;
	m_PP.EnableAutoDepthStencil = TRUE;
	m_PP.AutoDepthStencilFormat = depthStencilFormat;
	m_PP.Flags = 0;

	if (!windowed)
	{
		m_PP.FullScreen_RefreshRateInHz = d3ddm.RefreshRate;
		if (m_PP.FullScreen_RefreshRateInHz == 0)
			m_PP.FullScreen_RefreshRateInHz = 60;
	}
	else
	{
		m_PP.FullScreen_RefreshRateInHz = 0;
	}

	// »спользуем переданный интервал презентации
	m_PP.PresentationInterval = (presentInterval == 0) ? D3DPRESENT_INTERVAL_IMMEDIATE : (UINT)presentInterval;

	DWORD vertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	D3DCAPS9 caps;
	m_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE)
			vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
		else
			vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	}

	D3DDISPLAYMODEEX ModeEx;
	D3DDISPLAYMODEEX* pModeEx = nullptr;
	if (!windowed)
	{
		ZeroMemory(&ModeEx, sizeof(ModeEx));
		ModeEx.Size = sizeof(ModeEx);
		ModeEx.Width = width;
		ModeEx.Height = height;
		ModeEx.Format = m_BackBufferFmt;
		ModeEx.RefreshRate = m_PP.FullScreen_RefreshRateInHz;
		ModeEx.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
		pModeEx = &ModeEx;
	}

	hr = m_pD3D->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, vertexProcessing | D3DCREATE_MULTITHREADED,
								&m_PP, pModeEx, &m_pDevice);

	if (FAILED(hr))
	{
		Msg("! [DX9] CreateDeviceEx failed (0x%08x), trying without MULTITHREADED", hr);
		hr = m_pD3D->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, vertexProcessing, &m_PP, pModeEx,
									&m_pDevice);
		if (FAILED(hr))
		{
			Msg("! [DX9] Second attempt failed (0x%08x)", hr);
			return false;
		}
	}

	m_pDevice->GetDeviceCaps(&m_Caps);
	Msg("* [DX9] Device created successfully: %dx%d %s, interval=%d", width, height,
		windowed ? "windowed" : "fullscreen", presentInterval);
	return true;
}

void CRenderBackendDX9::DestroyDevice()
{
	if (m_pDevice)
		m_pDevice->Release();
	if (m_pD3D)
		m_pD3D->Release();
	m_pDevice = nullptr;
	m_pD3D = nullptr;
}

bool CRenderBackendDX9::Reset(HWND /* hWnd */)
{
	if (!m_pDevice)
		return false;

	// Ќа вс€кий случай обновл€ем параметры (можно пересоздать m_PP при необходимости)
	// «десь можно добавить логику из CHW::Reset, но пока простой вызов
	HRESULT hr = m_pDevice->Reset(&m_PP);
	return SUCCEEDED(hr);
}

void CRenderBackendDX9::Present()
{
	if (m_pDevice)
		m_pDevice->PresentEx(nullptr, nullptr, nullptr, nullptr, 0);
}

void CRenderBackendDX9::GetDeviceCaps(void* pCaps)
{
	if (pCaps)
		*(D3DCAPS9*)pCaps = m_Caps;
}

void CRenderBackendDX9::Clear(u32 clearFlags, const float color[4], float depth, u8 stencil)
{
	if (!m_pDevice)
		return;

	DWORD d3dFlags = 0;
	if (clearFlags & RHI_CLEAR_TARGET)
		d3dFlags |= D3DCLEAR_TARGET;
	if (clearFlags & RHI_CLEAR_ZBUFFER)
		d3dFlags |= D3DCLEAR_ZBUFFER;
	if (clearFlags & RHI_CLEAR_STENCIL)
		d3dFlags |= D3DCLEAR_STENCIL;

	D3DCOLOR d3dColor = D3DCOLOR_COLORVALUE(color[0], color[1], color[2], color[3]);
	m_pDevice->Clear(0, nullptr, d3dFlags, d3dColor, depth, stencil);
}

RHI_END
