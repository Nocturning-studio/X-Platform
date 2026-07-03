#include "stdafx.h"
#pragma hdrstop

#include "R_Backend.h"
#include "ResourceManager.h"
#include "xr_IOconsole.h"
#include <ppl.h>

ENGINE_API extern int psAnisotropic;

ENGINE_API CRenderBackend RenderBackend;

xr_token* vid_mode_token = NULL;

static void free_vid_mode_list()
{
    if (vid_mode_token)
    {
        for (int i = 0; vid_mode_token[i].name; i++)
            xr_free(vid_mode_token[i].name);
        xr_free(vid_mode_token);
        vid_mode_token = NULL;
    }
}

struct _uniq_mode
{
    LPCSTR _val;
    _uniq_mode(LPCSTR v) : _val(v) {}
    bool operator()(LPCSTR _other) { return !xr_stricmp(_val, _other); }
};

static void fill_vid_mode_list(xrRHI::IRenderBackend* RHI)
{
    if (!RHI) return;
    if (vid_mode_token != NULL) return;

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

CRenderBackend::CRenderBackend()
    : m_pD3D(NULL), m_pDevice(NULL), m_pBaseRT(NULL), m_pBaseZB(NULL),
    m_pRHI(nullptr), m_hRHI_DLL(NULL), QuadIB(NULL), old_QuadIB(NULL)
{
    ZeroMemory(&m_DevPP, sizeof(m_DevPP));
    Invalidate();
}

CRenderBackend::~CRenderBackend()
{
    Destroy();
}

void CRenderBackend::Create(HWND m_hWnd)
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
    m_pRHI = createBackend(desiredType);
    if (!m_pRHI)
    {
        Msg("! Failed to create render backend of requested type");
        FlushLog();
        MessageBox(NULL, "Failed to create render backend", "Fatal Error", MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 0);
        return;
    }

#ifndef DEDICATED_SERVER
    BOOL bWindowed = !psDeviceFlags.is(rsFullscreen);
#else
    BOOL bWindowed = TRUE;
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

    if (!m_pRHI->CreateDevice(m_hWnd, params))
    {
        Msg("! Failed to create device via RHI backend");
        delete m_pRHI;
        m_pRHI = nullptr;
        FreeLibrary(m_hRHI_DLL);
        m_hRHI_DLL = nullptr;
        FlushLog();
        MessageBox(NULL, "Failed to create graphics device", "Fatal Error", MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 0);
        return;
    }

    m_pDevice = (IDirect3DDevice9Ex*)m_pRHI->GetDeviceHandle();
    m_pD3D = (IDirect3D9Ex*)m_pRHI->GetD3DHandle();

    R_CHK(m_pDevice->GetRenderTarget(0, &m_pBaseRT));
    R_CHK(m_pDevice->GetDepthStencilSurface(&m_pBaseZB));

    D3DSURFACE_DESC desc;
    m_pBaseRT->GetDesc(&desc);
    Msg("* Backbuffer real size: %dx%d", desc.Width, desc.Height);

    D3DVIEWPORT9 vp;
    vp.X = 0; vp.Y = 0;
    vp.Width = desc.Width;
    vp.Height = desc.Height;
    vp.MinZ = 0.0f; vp.MaxZ = 1.0f;
    R_CHK(m_pDevice->SetViewport(&vp));

    m_DevPP.BackBufferWidth = desc.Width;
    m_DevPP.BackBufferHeight = desc.Height;
    m_DevPP.BackBufferFormat = D3DFMT_X8R8G8B8;
    m_DevPP.Windowed = bWindowed;
    m_DevPP.PresentationInterval = presentInterval;
    m_DevPP.BackBufferCount = 2;
    m_DevPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_DevPP.FullScreen_RefreshRateInHz = refreshHz;

#ifndef DEDICATED_SERVER
    ShowCursor(FALSE);
    SetForegroundWindow(m_hWnd);
#endif

    fill_vid_mode_list(m_pRHI);

    Msg("* RHI backend initialized successfully.");
}

void CRenderBackend::Destroy()
{
    if (m_pRHI)
    {
        m_pRHI->DestroyDevice();
        delete m_pRHI;
        m_pRHI = nullptr;
    }
    if (m_hRHI_DLL)
    {
        FreeLibrary(m_hRHI_DLL);
        m_hRHI_DLL = nullptr;
    }

    _RELEASE(m_pBaseZB);
    _RELEASE(m_pBaseRT);
    _RELEASE(m_pDevice);
    m_pD3D = NULL;

#ifndef _EDITOR
    free_vid_mode_list();
#endif
}

void CRenderBackend::Reset()
{
    _RELEASE(m_pBaseZB);
    _RELEASE(m_pBaseRT);

    if (!m_pRHI)
        return;

#ifndef DEDICATED_SERVER
    BOOL bWindowed = strstr(Core.Params, "-windowed") ? TRUE : !psDeviceFlags.is(rsFullscreen);
#else
    BOOL bWindowed = TRUE;
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

    HWND hWnd = wm.GetHandle();
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

    xrRHI::RHIPresentationParams params;
    params.BackBufferWidth = clientW;
    params.BackBufferHeight = clientH;
    params.Windowed = bWindowed;
    params.BackBufferFormat = xrRHI::RHI_Format::RGBA8_UNORM;
    params.DepthStencilFormat = xrRHI::RHI_Format::D24_UNORM_S8_UINT;
    params.BackBufferCount = 1;
    params.SyncInterval = (presentInterval == 0) ? 0 : 1;
    params.FullscreenRefreshHz = refreshHz;
    params.SwapEffect = xrRHI::RHI_SwapEffect::Discard;
    params.EnableAutoDepthStencil = true;

    if (!m_pRHI->Reset(params))
    {
        Msg("! RHI Reset failed");
        return;
    }

    R_CHK(m_pDevice->GetRenderTarget(0, &m_pBaseRT));
    R_CHK(m_pDevice->GetDepthStencilSurface(&m_pBaseZB));

    D3DSURFACE_DESC desc;
    m_pBaseRT->GetDesc(&desc);
    Msg("* Backbuffer real size: %dx%d", desc.Width, desc.Height);

    D3DVIEWPORT9 vp;
    vp.X = 0; vp.Y = 0;
    vp.Width = desc.Width;
    vp.Height = desc.Height;
    vp.MinZ = 0.0f; vp.MaxZ = 1.0f;
    R_CHK(m_pDevice->SetViewport(&vp));

    m_DevPP.BackBufferWidth = desc.Width;
    m_DevPP.BackBufferHeight = desc.Height;
    m_DevPP.BackBufferFormat = D3DFMT_X8R8G8B8;
    m_DevPP.Windowed = bWindowed;
    m_DevPP.PresentationInterval = presentInterval;
    m_DevPP.BackBufferCount = 1;
    m_DevPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_DevPP.FullScreen_RefreshRateInHz = refreshHz;
}

void CRenderBackend::selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed)
{
#ifdef DEDICATED_SERVER
    dwWidth = 32;
    dwHeight = 32;
#else
    dwWidth = psCurrentVidMode[0];
    dwHeight = psCurrentVidMode[1];
#endif
}

u32 CRenderBackend::selectPresentInterval()
{
#ifdef DEDICATED_SERVER
    return D3DPRESENT_INTERVAL_IMMEDIATE;
#else
    if (!psDeviceFlags.test(rsVSync))
        return D3DPRESENT_INTERVAL_IMMEDIATE;
    return D3DPRESENT_INTERVAL_DEFAULT;
#endif
}

void CRenderBackend::CreateQuadIB()
{
    const u32 dwTriCount = 4 * 1024;
    const u32 dwIdxCount = dwTriCount * 2 * 3;
    u16* Indices = 0;
    u32 dwUsage = D3DUSAGE_WRITEONLY;
    R_CHK(m_pDevice->CreateIndexBuffer(dwIdxCount * 2, dwUsage, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &QuadIB, NULL));
    R_CHK(QuadIB->Lock(0, 0, (void**)&Indices, 0));
    {
        int Cnt = 0;
        int ICnt = 0;
        for (int i = 0; i < dwTriCount; i++)
        {
            Indices[ICnt++] = u16(Cnt + 0);
            Indices[ICnt++] = u16(Cnt + 1);
            Indices[ICnt++] = u16(Cnt + 2);

            Indices[ICnt++] = u16(Cnt + 3);
            Indices[ICnt++] = u16(Cnt + 2);
            Indices[ICnt++] = u16(Cnt + 1);

            Cnt += 4;
        }
    }
    R_CHK(QuadIB->Unlock());
}

void CRenderBackend::OnDeviceCreate()
{
    CreateQuadIB();
    Vertex.Create();
    Index.Create();
    Invalidate();
    constants.reset_dirty();
    m_viewport.create(FVF::F_TL, Vertex.Buffer(), QuadIB);
}

void CRenderBackend::OnDeviceDestroy()
{
    Index.Destroy();
    Vertex.Destroy();
    constants.reset_dirty();
    _RELEASE(QuadIB);
}

void CRenderBackend::reset_begin()
{
    constants.force_dirty();
}

void CRenderBackend::reset_end()
{
    constants.reset_dirty();
}

void CRenderBackend::DeleteResources()
{
    m_viewport.destroy();
}

// ---------------------------------------------------------------------------
// State cache delegations
// ---------------------------------------------------------------------------
void CRenderBackend::SaveRenderState()
{
    m_stateCache.SaveRenderState(GetDevice());
}

void CRenderBackend::RestoreRenderState()
{
    m_stateCache.RestoreRenderState(GetDevice(), *this);
}

void CRenderBackend::set_Blend(BOOL enable, D3DBLEND src, D3DBLEND dest)
{
    m_stateCache.SetBlend(GetDevice(), enable, src, dest);
}

void CRenderBackend::set_Blend_Alpha()
{
    set_Blend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
}

void CRenderBackend::set_Blend_Add()
{
    set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
}

void CRenderBackend::set_Blend_Multiply()
{
    set_Blend(TRUE, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO);
}

void CRenderBackend::set_Blend_Default()
{
    set_Blend(FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
}

void CRenderBackend::set_Blend_Subtract()
{
    set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
    SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT);
}

void CRenderBackend::set_Blend_Screen()
{
    set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_INVSRCCOLOR);
}

void CRenderBackend::set_Blend_LightAdd()
{
    set_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
}

void CRenderBackend::set_Blend_ColorAdd()
{
    set_Blend(TRUE, D3DBLEND_SRCCOLOR, D3DBLEND_ONE);
}

void CRenderBackend::set_BlendEx(BOOL enable, D3DBLEND src, D3DBLEND dest, D3DBLENDOP op)
{
    m_stateCache.SetBlendEx(GetDevice(), enable, src, dest, op);
}

BOOL CRenderBackend::get_BlendState() const
{
    return m_stateCache.GetBlendEnable();
}

D3DBLEND CRenderBackend::get_SrcBlend() const
{
    return m_stateCache.GetSrcBlend();
}

D3DBLEND CRenderBackend::get_DstBlend() const
{
    return m_stateCache.GetDstBlend();
}

void CRenderBackend::enable_anisotropy_filtering()
{
    for (u32 i = 0; i < RHI()->GetDeviceCaps().MaxSimultaneousTextures; i++)
        CHK_DX(m_pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, psAnisotropic));
}

void CRenderBackend::disable_anisotropy_filtering()
{
    for (u32 i = 0; i < RHI()->GetDeviceCaps().MaxSimultaneousTextures; i++)
        CHK_DX(m_pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 1));
}

void CRenderBackend::set_anisotropy_filtering(int max_anisothropy)
{
    for (u32 i = 0; i < RHI()->GetDeviceCaps().MaxSimultaneousTextures; i++)
        CHK_DX(m_pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, max_anisothropy));
}

void CRenderBackend::Invalidate()
{
    m_stateCache.Invalidate(*this);
    m_resBinder.Invalidate(*this);
}
