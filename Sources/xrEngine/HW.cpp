#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(default : 4995)
#include "HW.h"
#include "xr_IOconsole.h"
#include "../xrRHI/xrRHI.h" // интерфейсы RHI

void fill_vid_mode_list(CHW* _hw);
void free_vid_mode_list();

ENGINE_API CHW HW;

#ifdef DEBUG
IDirect3DStateBlock9* dwDebugSB = 0;
#endif

xr_token* vid_mode_token = NULL;

//-----------------------------------------------------------------------------
CHW::CHW(): pD3D(NULL), pDevice(NULL), pBaseRT(NULL), pBaseZB(NULL), pBackend(nullptr), m_hRHI_DLL(nullptr)
{
	ZeroMemory(&DevPP, sizeof(DevPP));
}

CHW::~CHW()
{
	DestroyDevice();
}

//-----------------------------------------------------------------------------
void CHW::DestroyDevice()
{
	if (pBackend)
	{
		pBackend->DestroyDevice();
		delete pBackend;
		pBackend = nullptr;
		if (m_hRHI_DLL)
		{
			FreeLibrary(m_hRHI_DLL);
			m_hRHI_DLL = nullptr;
		}
		pDevice = nullptr;
		pD3D = nullptr;
	}

	_RELEASE(pBaseZB);
	_RELEASE(pBaseRT);
#ifdef DEBUG
	_RELEASE(dwDebugSB);
#endif
	_RELEASE(pDevice);

#ifndef _EDITOR
	free_vid_mode_list();
#endif
}

//-----------------------------------------------------------------------------
void CHW::Reset()
{
#ifdef DEBUG
    _RELEASE(dwDebugSB);
#endif
    _RELEASE(pBaseZB);
    _RELEASE(pBaseRT);

    if (!pBackend)
        return;

#ifndef DEDICATED_SERVER
    BOOL bWindowed = strstr(Core.Params, "-windowed") ? TRUE : !psDeviceFlags.is(rsFullscreen);
#else
    BOOL bWindowed = TRUE;
#endif

    // --- 1. Выбираем желаемые параметры экрана ---
    u32 width, height;
    selectResolution(width, height, bWindowed);

    u32 presentInterval = selectPresentInterval();
    u32 refreshHz = D3DPRESENT_RATE_DEFAULT;

    // --- 2. Настраиваем оконный менеджер ---
    CWindowManager& wm = Engine.WindowManager;
    wm.SetWindowed(bWindowed);
    wm.SetResolution(width, height);
    wm.SetRefreshRate(refreshHz);
    wm.Apply();

    HWND hWnd = wm.GetHandle();

    // Принудительно выставляем окну точный размер (клиентская область)
    SetWindowPos(hWnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    UpdateWindow(hWnd);

    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    LONG clientW = rcClient.right - rcClient.left;
    LONG clientH = rcClient.bottom - rcClient.top;

    if (clientW != width || clientH != height)
    {
        Msg("! Window client size mismatch: expected %dx%d, got %dx%d. Forcing resize.", width, height, clientW, clientH);
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_SHOWWINDOW);
        UpdateWindow(hWnd);
        GetClientRect(hWnd, &rcClient);
        clientW = rcClient.right - rcClient.left;
        clientH = rcClient.bottom - rcClient.top;
    }

    // --- 3. Заполняем RHIPresentationParams точными размерами ---
    xrRHI::RHIPresentationParams params;
    params.BackBufferWidth = clientW;
    params.BackBufferHeight = clientH;
    params.Windowed = bWindowed;
    params.BackBufferFormat = xrRHI::RHI_Format::RGBA8_UNORM;   // или D3DFMT_X8R8G8B8, преобразуется в бэкенде
    params.DepthStencilFormat = xrRHI::RHI_Format::D24_UNORM_S8_UINT;
    params.BackBufferCount = 1;                                 // как в старом Reset
    params.SyncInterval = (presentInterval == 0) ? 0 : 1;
    params.FullscreenRefreshHz = refreshHz;
    params.SwapEffect = xrRHI::RHI_SwapEffect::Discard;
    params.EnableAutoDepthStencil = true;

    // --- 4. Сбрасываем устройство ---
    if (!pBackend->Reset(params))
    {
        Msg("! RHI Reset failed");
        return;
    }

    // --- 5. Восстанавливаем базовые поверхности ---
    R_CHK(pDevice->GetRenderTarget(0, &pBaseRT));
    R_CHK(pDevice->GetDepthStencilSurface(&pBaseZB));

    D3DSURFACE_DESC desc;
    pBaseRT->GetDesc(&desc);
    Msg("* Backbuffer real size: %dx%d", desc.Width, desc.Height);

    D3DVIEWPORT9 vp;
    vp.X = 0; vp.Y = 0;
    vp.Width = desc.Width;
    vp.Height = desc.Height;
    vp.MinZ = 0.0f; vp.MaxZ = 1.0f;
    R_CHK(pDevice->SetViewport(&vp));

    // Обновляем DevPP для совместимости
    DevPP.BackBufferWidth = desc.Width;
    DevPP.BackBufferHeight = desc.Height;
    DevPP.BackBufferFormat = D3DFMT_X8R8G8B8;  // в реальности может отличаться, но у нас так
    DevPP.Windowed = bWindowed;
    DevPP.PresentationInterval = presentInterval;
    DevPP.BackBufferCount = 1;
    DevPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
    DevPP.FullScreen_RefreshRateInHz = refreshHz;

#ifdef DEBUG
    if (pDevice)
        R_CHK(pDevice->CreateStateBlock(D3DSBT_ALL, &dwDebugSB));
#endif
}

//-----------------------------------------------------------------------------
void CHW::CreateDevice(HWND m_hWnd)
{
    m_hRHI_DLL = LoadLibrary("xrRHI.dll");
    if (!m_hRHI_DLL)
    {
        Msg("! Failed to load xrRHI.dll");
        FlushLog();
        MessageBox(NULL, "Failed to load xrRHI.dll", "Fatal Error", MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 0);
        return;
    }

    typedef xrRHI::IRenderBackend* (*CreateBackendFunc)(xrRHI::BackendType);
    CreateBackendFunc createBackend = (CreateBackendFunc)GetProcAddress(m_hRHI_DLL, "CreateRenderBackend");
    if (!createBackend)
    {
        Msg("! Failed to get CreateRenderBackend function from xrRHI.dll");
        FlushLog();
        MessageBox(NULL, "Invalid xrRHI.dll", "Fatal Error", MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 0);
        return;
    }

    xrRHI::BackendType desiredType = xrRHI::BackendType::DirectX9;
    pBackend = createBackend(desiredType);
    if (!pBackend)
    {
        Msg("! Failed to create render backend of requested type");
        FlushLog();
        MessageBox(NULL, "Failed to create render backend", "Fatal Error", MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 0);
        return;
    }

#ifdef DEDICATED_SERVER
    BOOL bWindowed = TRUE;
#else
    BOOL bWindowed = !psDeviceFlags.is(rsFullscreen);
#endif

    u32 width, height;
    selectResolution(width, height, bWindowed);
    u32 presentInterval = selectPresentInterval();
    u32 refreshHz = D3DPRESENT_RATE_DEFAULT;

    CWindowManager& wm = Engine.WindowManager;
    wm.SetWindowed(bWindowed);
    wm.SetResolution(width, height);
    wm.SetRefreshRate(refreshHz);
    wm.Apply();

    SetWindowPos(m_hWnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    UpdateWindow(m_hWnd);

    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);
    LONG clientW = rcClient.right - rcClient.left;
    LONG clientH = rcClient.bottom - rcClient.top;

    if (clientW != width || clientH != height)
    {
        Msg("! Window client size mismatch: expected %dx%d, got %dx%d. Forcing resize.", width, height, clientW, clientH);
        SetWindowLongPtr(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(m_hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_SHOWWINDOW);
        UpdateWindow(m_hWnd);
        GetClientRect(m_hWnd, &rcClient);
        clientW = rcClient.right - rcClient.left;
        clientH = rcClient.bottom - rcClient.top;
    }

    xrRHI::RHIPresentationParams params;
    params.BackBufferWidth = clientW;
    params.BackBufferHeight = clientH;
    params.Windowed = bWindowed;
    params.BackBufferFormat = xrRHI::RHI_Format::RGBA8_UNORM;
    params.DepthStencilFormat = xrRHI::RHI_Format::D24_UNORM_S8_UINT;
    params.BackBufferCount = 2;
    params.SyncInterval = (presentInterval == 0) ? 0 : 1;
    params.FullscreenRefreshHz = refreshHz;
    params.SwapEffect = xrRHI::RHI_SwapEffect::Discard;
    params.EnableAutoDepthStencil = true;

    if (!pBackend->CreateDevice(m_hWnd, params))
    {
        Msg("! Failed to create device via RHI backend");
        delete pBackend;
        pBackend = nullptr;
        FreeLibrary(m_hRHI_DLL);
        m_hRHI_DLL = nullptr;
        FlushLog();
        MessageBox(NULL, "Failed to create graphics device", "Fatal Error", MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 0);
        return;
    }

    pDevice = (IDirect3DDevice9Ex*)pBackend->GetDeviceHandle();
    pD3D = (IDirect3D9Ex*)pBackend->GetD3DHandle();

    DevAdapter = D3DADAPTER_DEFAULT;
    DevT = D3DDEVTYPE_HAL;
    Msg("[RHI] DevAdapter=%d, DevT=%d", DevAdapter, DevT);

    R_CHK(pDevice->GetRenderTarget(0, &pBaseRT));
    R_CHK(pDevice->GetDepthStencilSurface(&pBaseZB));

    D3DSURFACE_DESC desc;
    pBaseRT->GetDesc(&desc);
    Msg("* Backbuffer real size: %dx%d", desc.Width, desc.Height);

    D3DVIEWPORT9 vp;
    vp.X = 0; vp.Y = 0;
    vp.Width = desc.Width;
    vp.Height = desc.Height;
    vp.MinZ = 0.0f; vp.MaxZ = 1.0f;
    R_CHK(pDevice->SetViewport(&vp));

    DevPP.BackBufferWidth = desc.Width;
    DevPP.BackBufferHeight = desc.Height;
    DevPP.BackBufferFormat = D3DFMT_X8R8G8B8;
    DevPP.Windowed = bWindowed;
    DevPP.PresentationInterval = presentInterval;
    DevPP.BackBufferCount = 2;
    DevPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
    DevPP.FullScreen_RefreshRateInHz = refreshHz;

#ifndef DEDICATED_SERVER
    ShowCursor(FALSE);
    SetForegroundWindow(m_hWnd);
#endif

    fill_vid_mode_list(this); // больше не требует Caps

    Msg("* RHI backend initialized successfully.");
}

//-----------------------------------------------------------------------------
void CHW::selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed)
{
#ifdef DEDICATED_SERVER
	dwWidth = 32;
	dwHeight = 32;
#else
	dwWidth = psCurrentVidMode[0];
	dwHeight = psCurrentVidMode[1];
#endif
}

//-----------------------------------------------------------------------------
u32 CHW::selectPresentInterval()
{
#ifdef DEDICATED_SERVER
	return D3DPRESENT_INTERVAL_IMMEDIATE;
#else
	if (!psDeviceFlags.test(rsVSync))
		return D3DPRESENT_INTERVAL_IMMEDIATE;
	return D3DPRESENT_INTERVAL_DEFAULT;
#endif
}

struct _uniq_mode
{
	_uniq_mode(LPCSTR v) : _val(v)
	{
	}
	LPCSTR _val;
	bool operator()(LPCSTR _other)
	{
		return !xr_stricmp(_val, _other);
	}
};

void free_vid_mode_list()
{
	if (vid_mode_token)
	{
		for (int i = 0; vid_mode_token[i].name; i++)
			xr_free(vid_mode_token[i].name);
		xr_free(vid_mode_token);
		vid_mode_token = NULL;
	}
}

void fill_vid_mode_list(CHW* _hw)
{
    if (vid_mode_token != NULL)
        return;

    xrRHI::IRenderBackend* RHI = _hw->GetRHI();
    if (!RHI) return;

    std::vector<std::pair<u32, u32>> resolutions;
    RHI->GetAvailableResolutions(RHI->GetBackBufferFormat(), resolutions);

    if (resolutions.empty())
    {
        RECT rect;
        GetClientRect(GetDesktopWindow(), &rect);
        resolutions.emplace_back(rect.right - rect.left, rect.bottom - rect.top);
    }

    xr_vector<LPCSTR> _tmp;
    for (const auto& res : resolutions)
    {
        string32 str;
        sprintf_s(str, sizeof(str), "%dx%d", res.first, res.second);
        if (std::find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)) != _tmp.end())
            continue;
        _tmp.push_back(xr_strdup(str));
    }

    u32 _cnt = _tmp.size() + 1;
    vid_mode_token = xr_alloc<xr_token>(_cnt);
    vid_mode_token[_cnt - 1].id = -1;
    vid_mode_token[_cnt - 1].name = NULL;
    for (u32 i = 0; i < _tmp.size(); ++i)
    {
        vid_mode_token[i].id = i;
        vid_mode_token[i].name = _tmp[i];
    }
}
