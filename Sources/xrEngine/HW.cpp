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
	Caps.bForceGPU_REF = FALSE;
	Caps.bForceGPU_SW = FALSE;
	Caps.bForceGPU_NonPure = FALSE;
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
    u32 refreshHz = bWindowed ? 0 : selectRefresh(width, height, Caps.fTarget);

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
    // Загружаем библиотеку xrRHI.dll
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

    // --- 1. Определяем режим окна ---
#ifdef DEDICATED_SERVER
    BOOL bWindowed = TRUE;
#else
    BOOL bWindowed = !psDeviceFlags.is(rsFullscreen);
#endif

    u32 width, height;
    selectResolution(width, height, bWindowed);
    u32 presentInterval = selectPresentInterval();
    u32 refreshHz = bWindowed ? 0 : selectRefresh(width, height, D3DFMT_X8R8G8B8); // пока формат фиксирован

    // --- 2. Настройка окна ДО создания устройства ---
    CWindowManager& wm = Engine.WindowManager;
    wm.SetWindowed(bWindowed);
    wm.SetResolution(width, height);
    wm.SetRefreshRate(refreshHz);
    wm.Apply();

    // Принудительно выставляем размер клиентской области
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

    // --- 3. Заполняем RHIPresentationParams точными размерами ---
    xrRHI::RHIPresentationParams params;
    params.BackBufferWidth = clientW;
    params.BackBufferHeight = clientH;
    params.Windowed = bWindowed;
    params.BackBufferFormat = xrRHI::RHI_Format::RGBA8_UNORM;   // соответствует D3DFMT_X8R8G8B8
    params.DepthStencilFormat = xrRHI::RHI_Format::D24_UNORM_S8_UINT;
    params.BackBufferCount = 2;                                 // оригинальное значение
    params.SyncInterval = (presentInterval == 0) ? 0 : 1;
    params.FullscreenRefreshHz = refreshHz;
    params.SwapEffect = xrRHI::RHI_SwapEffect::Discard;
    params.EnableAutoDepthStencil = true;

    // --- 4. Создаём устройство ---
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

    // --- 5. Получаем нативные указатели для совместимости ---
    pDevice = (IDirect3DDevice9Ex*)pBackend->GetDeviceHandle();
    pD3D = (IDirect3D9Ex*)pBackend->GetD3DHandle();

    DevAdapter = D3DADAPTER_DEFAULT;
    DevT = D3DDEVTYPE_HAL;
    Msg("[RHI] DevAdapter=%d, DevT=%d", DevAdapter, DevT);

    Caps.fTarget = D3DFMT_X8R8G8B8;
    Caps.fDepth = D3DFMT_D24S8;
    Caps.Update();

    // Получаем реальный размер бэкбуфера (может незначительно отличаться)
    R_CHK(pDevice->GetRenderTarget(0, &pBaseRT));
    R_CHK(pDevice->GetDepthStencilSurface(&pBaseZB));

    D3DSURFACE_DESC desc;
    pBaseRT->GetDesc(&desc);
    Msg("* Backbuffer real size: %dx%d", desc.Width, desc.Height);

    // Устанавливаем вьюпорт
    D3DVIEWPORT9 vp;
    vp.X = 0; vp.Y = 0;
    vp.Width = desc.Width;
    vp.Height = desc.Height;
    vp.MinZ = 0.0f; vp.MaxZ = 1.0f;
    R_CHK(pDevice->SetViewport(&vp));

    // Заполняем DevPP для совместимости со старым кодом
    DevPP.BackBufferWidth = desc.Width;
    DevPP.BackBufferHeight = desc.Height;
    DevPP.BackBufferFormat = D3DFMT_X8R8G8B8;
    DevPP.Windowed = bWindowed;
    DevPP.PresentationInterval = presentInterval;
    DevPP.BackBufferCount = 2;
    DevPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
    DevPP.FullScreen_RefreshRateInHz = refreshHz;

    // --- 6. Дополнительные инициализации ---
#ifndef DEDICATED_SERVER
    ShowCursor(FALSE);
    SetForegroundWindow(m_hWnd);
#endif

    fill_vid_mode_list(this);

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

//-----------------------------------------------------------------------------
u32 CHW::selectGPU()
{
	if (Caps.bForceGPU_SW)
		return D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	D3DCAPS9 caps;
	pD3D->GetDeviceCaps(DevAdapter, DevT, &caps);

	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		if (Caps.bForceGPU_NonPure)
			return D3DCREATE_HARDWARE_VERTEXPROCESSING;
		else
		{
			if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE)
				return D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
			else
				return D3DCREATE_HARDWARE_VERTEXPROCESSING;
		}
	}
	else
		return D3DCREATE_SOFTWARE_VERTEXPROCESSING;
}

//-----------------------------------------------------------------------------
u32 CHW::selectRefresh(u32 dwWidth, u32 dwHeight, D3DFORMAT fmt)
{
	return D3DPRESENT_RATE_DEFAULT;
}

//-----------------------------------------------------------------------------
BOOL CHW::support(D3DFORMAT fmt, DWORD type, DWORD usage)
{
	HRESULT hr = pD3D->CheckDeviceFormat(DevAdapter, DevT, Caps.fTarget, usage, (D3DRESOURCETYPE)type, fmt);
	Msg("support: fmt=%d, type=%d, usage=%d, fTarget=%d, hr=0x%08x", fmt, type, usage, Caps.fTarget, hr);
	LogWinErr("HR to string", hr);
	return SUCCEEDED(hr);
}

//-----------------------------------------------------------------------------
void CHW::updateWindowProps(HWND m_hWnd)
{
#ifdef DEDICATED_SERVER
	SetParent(m_hWnd, HWND_MESSAGE);
	SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP);
	ShowWindow(m_hWnd, SW_HIDE);
	return;
#endif

	BOOL bWindowed = !psDeviceFlags.is(rsFullscreen);

	u32 dwWindowStyle = 0;
	if (bWindowed || strstr(Core.Params, "-windowed"))
	{
		SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle = (WS_POPUP));

		RECT m_rcWindowBounds;
		RECT DesktopRect;
		GetClientRect(GetDesktopWindow(), &DesktopRect);

		SetRect(&m_rcWindowBounds, (DesktopRect.right - DevPP.BackBufferWidth) / 2,
				(DesktopRect.bottom - DevPP.BackBufferHeight) / 2, (DesktopRect.right + DevPP.BackBufferWidth) / 2,
				(DesktopRect.bottom + DevPP.BackBufferHeight) / 2);

		AdjustWindowRect(&m_rcWindowBounds, dwWindowStyle, FALSE);

		SetWindowPos(m_hWnd, HWND_NOTOPMOST, m_rcWindowBounds.left, m_rcWindowBounds.top,
					 (m_rcWindowBounds.right - m_rcWindowBounds.left), (m_rcWindowBounds.bottom - m_rcWindowBounds.top),
					 SWP_SHOWWINDOW | SWP_NOCOPYBITS);
	}
	else
	{
		SetWindowLong(m_hWnd, GWL_STYLE, dwWindowStyle = (WS_POPUP | WS_VISIBLE));
	}

#ifndef DEDICATED_SERVER
	ShowCursor(FALSE);
	SetForegroundWindow(m_hWnd);
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
	xr_vector<LPCSTR> _tmp;
	u32 cnt = _hw->GetD3D()->GetAdapterModeCount(_hw->GetDevAdapter(), _hw->GetCaps().fTarget);

	for (u32 i = 0; i < cnt; ++i)
	{
		D3DDISPLAYMODE Mode;
		string32 str;
		_hw->GetD3D()->EnumAdapterModes(_hw->GetDevAdapter(), _hw->GetCaps().fTarget, i, &Mode);
		sprintf_s(str, sizeof(str), "%dx%d", Mode.Width, Mode.Height);
		if (_tmp.end() != std::find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)))
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
