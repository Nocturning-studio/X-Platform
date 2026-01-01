// HW.cpp: implementation of the CHW class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(default : 4995)
#include "HW.h"
#include "xr_IOconsole.h"

#ifndef _EDITOR
void fill_vid_mode_list(CHW* _hw);
void free_vid_mode_list();
#else
void fill_vid_mode_list(CHW* _hw){};
void free_vid_mode_list(){};
#endif

void free_vid_mode_list();

ENGINE_API CHW HW;

#ifdef DEBUG
IDirect3DStateBlock9* dwDebugSB = 0;
#endif

//-----------------------------------------------------------------------------
void CHW::Reset(HWND hwnd)
{
	OPTICK_EVENT("CHW::Reset");

#ifdef DEBUG
	_RELEASE(dwDebugSB);
#endif
	_RELEASE(pBaseZB);
	_RELEASE(pBaseRT);

#ifndef _EDITOR
#ifndef DEDICATED_SERVER
	BOOL bWindowed = strstr(Core.Params, "-windowed") ? TRUE : !psDeviceFlags.is(rsFullscreen);
#else
	BOOL bWindowed = TRUE;
#endif

	selectResolution(DevPP.BackBufferWidth, DevPP.BackBufferHeight, bWindowed);

	DevPP.Windowed = bWindowed;
	// Для выделенного сервера важно IMMEDIATE, чтобы не ждать развертки скрытого окна
	DevPP.PresentationInterval = selectPresentInterval();
	DevPP.BackBufferCount = 1; // Хватит одного буфера для сервера

	// Discard самый быстрый
	DevPP.SwapEffect = D3DSWAPEFFECT_DISCARD;

	if (!bWindowed)
		DevPP.FullScreen_RefreshRateInHz = selectRefresh(DevPP.BackBufferWidth, DevPP.BackBufferHeight, Caps.fTarget);
	else
		DevPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
#endif

	D3DDISPLAYMODEEX Mode;
	ZeroMemory(&Mode, sizeof(Mode));
	Mode.Size = sizeof(D3DDISPLAYMODEEX);

	if (!DevPP.Windowed)
	{
		Mode.Width = DevPP.BackBufferWidth;
		Mode.Height = DevPP.BackBufferHeight;
		Mode.Format = DevPP.BackBufferFormat;
		Mode.RefreshRate = DevPP.FullScreen_RefreshRateInHz;
		Mode.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
	}

	while (TRUE)
	{
		HRESULT _hr = pDevice->ResetEx(&DevPP, DevPP.Windowed ? NULL : &Mode);

		if (SUCCEEDED(_hr))
			break;
		Msg("! ERROR: [%dx%d]: %s", DevPP.BackBufferWidth, DevPP.BackBufferHeight, Debug.error2string(_hr));
		Sleep(100);
	}
	R_CHK(pDevice->GetRenderTarget(0, &pBaseRT));
	R_CHK(pDevice->GetDepthStencilSurface(&pBaseZB));
#ifdef DEBUG
	R_CHK(pDevice->CreateStateBlock(D3DSBT_ALL, &dwDebugSB));
#endif
#ifndef _EDITOR
	updateWindowProps(hwnd);
#endif
}

xr_token* vid_mode_token = NULL;

void CHW::CreateD3D()
{
	OPTICK_EVENT("CHW::CreateD3D");
	Msg("Creating Direct3D9Ex");

	HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &this->pD3D);
	if (FAILED(hr) || !this->pD3D)
	{
		Msg("Failed to create Direct3D9Ex!");
		// Fallback к обычному d3d9 если Ex не найден, но в 2025 году это редкость
	}
}

void CHW::DestroyD3D()
{
	OPTICK_EVENT("CHW::DestroyD3D");
	_RELEASE(this->pD3D);
}

D3DFORMAT CHW::selectDepthStencil(D3DFORMAT fTarget)
{
	// D24S8 наиболее совместимый
	return D3DFMT_D24S8;
}

void CHW::DestroyDevice()
{
	OPTICK_EVENT("CHW::DestroyDevice");

	_RELEASE(pBaseZB);
	_RELEASE(pBaseRT);
#ifdef DEBUG
	_RELEASE(dwDebugSB);
#endif
	_RELEASE(HW.pDevice);

	DestroyD3D();

#ifndef _EDITOR
	free_vid_mode_list();
#endif
}

//-----------------------------------------------------------------------------
void CHW::selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed)
{
	OPTICK_EVENT("CHW::selectResolution");

	fill_vid_mode_list(this);

#ifdef DEDICATED_SERVER
	// --- ОПТИМИЗАЦИЯ ДЛЯ СЕРВЕРА ---
	// Используем минимально возможное разрешение, чтобы GPU почти не работал.
	// 32x32 вполне достаточно для создания контекста.
	dwWidth = 32;
	dwHeight = 32;
#else
	if (bWindowed)
	{
		dwWidth = psCurrentVidMode[0];
		dwHeight = psCurrentVidMode[1];
	}
	else
	{
#ifndef _EDITOR
		string64 buff;
		sprintf_s(buff, sizeof(buff), "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);
		if (_ParseItem(buff, vid_mode_token) == u32(-1))
		{
			sprintf_s(buff, sizeof(buff), "vid_mode %s", vid_mode_token[0].name);
			Console->Execute(buff);
		}
		dwWidth = psCurrentVidMode[0];
		dwHeight = psCurrentVidMode[1];
#endif
	}
#endif
}

void CHW::CreateDevice(HWND m_hWnd)
{
	OPTICK_EVENT("CHW::CreateDevice");

	CreateD3D();

#ifdef DEDICATED_SERVER
	BOOL bWindowed = TRUE;
#else
	BOOL bWindowed = !psDeviceFlags.is(rsFullscreen);
#endif

	DevAdapter = D3DADAPTER_DEFAULT;
	DevT = Caps.bForceGPU_REF ? D3DDEVTYPE_REF : D3DDEVTYPE_HAL;

	// Display the name of video board
	D3DADAPTER_IDENTIFIER9 adapterID;
	R_CHK(pD3D->GetAdapterIdentifier(DevAdapter, 0, &adapterID));
	Msg("* GPU [vendor:%X]-[device:%X]: %s", adapterID.VendorId, adapterID.DeviceId, adapterID.Description);

	Caps.id_vendor = adapterID.VendorId;
	Caps.id_device = adapterID.DeviceId;

	D3DDISPLAYMODE mWindowed;
	R_CHK(pD3D->GetAdapterDisplayMode(DevAdapter, &mWindowed));

	D3DFORMAT& fTarget = Caps.fTarget;
	D3DFORMAT& fDepth = Caps.fDepth;

	fTarget = mWindowed.Format; // Используем формат десктопа
	fDepth = D3DFMT_D24S8;

	// Set up the presentation parameters
	D3DPRESENT_PARAMETERS& P = DevPP;
	ZeroMemory(&P, sizeof(P));

#ifndef _EDITOR
	selectResolution(P.BackBufferWidth, P.BackBufferHeight, bWindowed);
#endif

	P.BackBufferFormat = fTarget;

	// FIX 1: Увеличиваем буфер для надежности (было 1)
	P.BackBufferCount = 2;

	P.MultiSampleType = D3DMULTISAMPLE_NONE;
	P.MultiSampleQuality = 0;
	P.SwapEffect = D3DSWAPEFFECT_DISCARD;
	P.hDeviceWindow = m_hWnd;
	P.Windowed = bWindowed;
	P.EnableAutoDepthStencil = TRUE;
	P.AutoDepthStencilFormat = fDepth;
	P.Flags = 0; // Можно добавить D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL для оптимизации

	P.PresentationInterval = selectPresentInterval();

	// FIX 2: Подготовка структуры DisplayModeEx для CreateDeviceEx
	D3DDISPLAYMODEEX ModeEx;
	ZeroMemory(&ModeEx, sizeof(D3DDISPLAYMODEEX));
	ModeEx.Size = sizeof(D3DDISPLAYMODEEX);

	D3DDISPLAYMODEEX* pModeEx = NULL;

	if (!bWindowed)
	{
		// FIX 3: Не используем 0 (DEFAULT), берем частоту десктопа
		u32 refreshRate = mWindowed.RefreshRate;
		if (refreshRate == 0)
			refreshRate = 60;

		P.FullScreen_RefreshRateInHz = refreshRate;

		ModeEx.Width = P.BackBufferWidth;
		ModeEx.Height = P.BackBufferHeight;
		ModeEx.Format = P.BackBufferFormat;
		ModeEx.RefreshRate = P.FullScreen_RefreshRateInHz;
		ModeEx.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;

		// Указатель на структуру для передачи в функцию
		pModeEx = &ModeEx;
	}
	else
	{
		P.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		pModeEx = NULL;
	}

	u32 GPU = selectGPU();

	// Create Device
	// FIX 4: Передаем pModeEx вместо NULL
	HRESULT R = pD3D->CreateDeviceEx(DevAdapter, DevT, m_hWnd, GPU | D3DCREATE_MULTITHREADED, &P, pModeEx, &pDevice);

	if (FAILED(R))
	{
		// Попытка без FPU preserve
		R = pD3D->CreateDeviceEx(DevAdapter, DevT, m_hWnd, GPU | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &P,
								 pModeEx, &pDevice);
	}

	// FIX 5: Последний шанс - если не вышло с конкретным режимом, пробуем NULL (как было раньше), но с четкой частотой
	if (FAILED(R) && !bWindowed)
	{
		Msg("! Failed to create device with explicit ModeEx, trying fallback...");
		P.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		R = pD3D->CreateDeviceEx(DevAdapter, DevT, m_hWnd, GPU | D3DCREATE_MULTITHREADED, &P, NULL, &pDevice);
	}

	if (FAILED(R))
	{
		Msg("! Device creation failed: %s", Debug.error2string(R));
		FlushLog();
		MessageBox(NULL, "Failed to initialize graphics hardware.", "Error!", MB_OK | MB_ICONERROR);
		TerminateProcess(GetCurrentProcess(), 0);
	}

	_SHOW_REF("* CREATE: DeviceREF:", HW.pDevice);

	R_CHK(pDevice->GetRenderTarget(0, &pBaseRT));
	R_CHK(pDevice->GetDepthStencilSurface(&pBaseZB));

#ifndef _EDITOR
	updateWindowProps(m_hWnd);
	fill_vid_mode_list(this);
#endif
}

//-----------------------------------------------------------------------------
u32 CHW::selectPresentInterval()
{
	// Для сервера нам вообще не нужна синхронизация.
	// Скрытое окно не обновляется монитором, поэтому DEFAULT может заблокировать Present
	// в ожидании несуществующей развертки. Ставим IMMEDIATE.
#ifdef DEDICATED_SERVER
	return D3DPRESENT_INTERVAL_IMMEDIATE;
#else
	if (!psDeviceFlags.test(rsVSync))
		return D3DPRESENT_INTERVAL_IMMEDIATE;
	return D3DPRESENT_INTERVAL_DEFAULT;
#endif
}

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

u32 CHW::selectRefresh(u32 dwWidth, u32 dwHeight, D3DFORMAT fmt)
{
	return D3DPRESENT_RATE_DEFAULT;
}

BOOL CHW::support(D3DFORMAT fmt, DWORD type, DWORD usage)
{
	HRESULT hr = pD3D->CheckDeviceFormat(DevAdapter, DevT, Caps.fTarget, usage, (D3DRESOURCETYPE)type, fmt);
	if (FAILED(hr))
		return FALSE;
	else
		return TRUE;
}

//-----------------------------------------------------------------------------
// Магия скрытия окна здесь
//-----------------------------------------------------------------------------
void CHW::updateWindowProps(HWND m_hWnd)
{
	OPTICK_EVENT("CHW::updateWindowProps");

#ifdef DEDICATED_SERVER
	// 1. Делаем окно "только для сообщений". Это удаляет его из таскбара, списка приложений и отрисовки рабочего стола.
	SetParent(m_hWnd, HWND_MESSAGE);

	// 2. Дополнительно скрываем его стилем
	SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP); // Убираем все рамки

	// 3. Прячем через ShowWindow (D3D может продолжить работать, если не привязан к Present/VSync)
	// Важно использовать SW_HIDE. Поскольку мы используем HWND_MESSAGE и D3DPRESENT_INTERVAL_IMMEDIATE,
	// проблем с "застывшим" рендером быть не должно.
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

// ... Остальной код (fill_vid_mode_list и прочее) без изменений
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

#ifndef _EDITOR
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
	u32 cnt = _hw->pD3D->GetAdapterModeCount(_hw->DevAdapter, _hw->Caps.fTarget);

	for (u32 i = 0; i < cnt; ++i)
	{
		D3DDISPLAYMODE Mode;
		string32 str;
		_hw->pD3D->EnumAdapterModes(_hw->DevAdapter, _hw->Caps.fTarget, i, &Mode);
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
#endif
