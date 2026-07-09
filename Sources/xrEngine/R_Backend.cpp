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
    m_stateCache.Invalidate(*this);
    m_resBinder.Invalidate(*this);
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

void CRenderBackend::OnFrameEnd()
{
#ifndef DEDICATED_SERVER
    for (u32 stage = 0; stage < RHI()->GetDeviceCaps().MaxSimultaneousTextures; stage++)
        CHK_DX(GetDevice()->SetTexture(0, 0));
    CHK_DX(GetDevice()->SetStreamSource(0, 0, 0, 0));
    CHK_DX(GetDevice()->SetIndices(0));
    CHK_DX(GetDevice()->SetVertexShader(0));
    CHK_DX(GetDevice()->SetPixelShader(0));
    Invalidate();
#endif
}

void CRenderBackend::OnFrameBegin()
{
#ifndef DEDICATED_SERVER
    std::memset(&stat, 0, sizeof(stat));
    Vertex.Flush();
    Index.Flush();
    set_Stencil(FALSE);
#endif
}

#ifndef DEDICATED_SERVER

void CRenderBackend::set_Textures(STextureList* _T)
{
    m_resBinder.SetTextures(*this, _T);
}
#else

void CRenderBackend::set_Textures(STextureList* _T)
{
}

#endif

void CRenderBackend::set_Render_Target_Surface(const ref_rt& rt_1, const ref_rt& rt_2, const ref_rt& rt_3, const ref_rt& rt_4)
{
    VERIFY2(rt_1, "Rendertarget must have minimum one target surface (ref_rt& rt_1)");

    RenderBackend.setRenderTarget(rt_1->pRT, 0);

    if (rt_2)
        RenderBackend.setRenderTarget(rt_2->pRT, 1);
    else
        RenderBackend.setRenderTarget(NULL, 1);

    if (rt_3)
        RenderBackend.setRenderTarget(rt_3->pRT, 2);
    else
        RenderBackend.setRenderTarget(NULL, 2);

    if (rt_4)
        RenderBackend.setRenderTarget(rt_4->pRT, 3);
    else
        RenderBackend.setRenderTarget(NULL, 3);
}

void CRenderBackend::set_Render_Target_Surface(u32 W, u32 H, IDirect3DSurface9* rt_1, IDirect3DSurface9* rt_2, IDirect3DSurface9* rt_3, IDirect3DSurface9* rt_4)
{
    VERIFY2(rt_1, "Rendertarget must have minimum one target surface (IDirect3DSurface9* rt_1)");

    RenderBackend.setRenderTarget(rt_1, 0);

    if (rt_2)
        RenderBackend.setRenderTarget(rt_2, 1);
    else
        RenderBackend.setRenderTarget(NULL, 1);

    if (rt_3)
        RenderBackend.setRenderTarget(rt_3, 2);
    else
        RenderBackend.setRenderTarget(NULL, 2);

    if (rt_4)
        RenderBackend.setRenderTarget(rt_4, 3);
    else
        RenderBackend.setRenderTarget(NULL, 3);
}

void CRenderBackend::set_Depth_Buffer(IDirect3DSurface9* zb)
{
    RenderBackend.setDepthBuffer(zb);
}

void CRenderBackend::clear_Depth_Buffer(IDirect3DSurface9* zb)
{
    RenderBackend.setDepthBuffer(zb);
    CHK_DX(RenderBackend.GetDevice()->Clear(0L, nullptr, D3DCLEAR_ZBUFFER, 0x0, 1.0f, 0L));
}

// 2D texgen (texture adjustment matrix)
void CRenderBackend::u_compute_texgen_screen(fmat4x4& m_Texgen)
{
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    float o_w = (.5f / _w);
    float o_h = (.5f / _h);
    fmat4x4 m_TexelAdjust = { 0.5f, 0.0f, 0.0f, 0.0f,
                             0.0f, -0.5f, 0.0f, 0.0f,
                             0.0f, 0.0f, 1.0f, 0.0f,
                             0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f };
    m_Texgen.mul(m_TexelAdjust, RenderBackend.transforms.m_WorldViewProject);
}

void CRenderBackend::set_viewport_geometry(u32 w, u32 h, ref_geom geometry, u32& vOffset)
{
    // Constants
    u32 Color = color_rgba(0, 0, 0, 255);

    float d_Z = EPS_S;
    float d_W = 1.f;

    fvec2 p0, p1;
    p0.set(0.5f / w, 0.5f / h);
    p1.set((w + 0.5f) / w, (h + 0.5f) / h);

    // Fill vertex buffer
    FVF::TL* pv = (FVF::TL*)RenderBackend.Vertex.Lock(4, geometry->vb_stride, vOffset);
    pv->set_position(0, (float)h, d_Z, d_W);
    pv->set_color(Color);
    pv->set_uv(p0.x, p1.y);
    pv++;

    pv->set_position(0, 0, d_Z, d_W);
    pv->set_color(Color);
    pv->set_uv(p0.x, p0.y);
    pv++;

    pv->set_position((float)w, (float)h, d_Z, d_W);
    pv->set_color(Color);
    pv->set_uv(p1.x, p1.y);
    pv++;

    pv->set_position((float)w, 0, d_Z, d_W);
    pv->set_color(Color);
    pv->set_uv(p1.x, p0.y);
    pv++;
    RenderBackend.Vertex.Unlock(4, geometry->vb_stride);

    // Set geometry
    RenderBackend.set_Geometry(geometry);
}

void CRenderBackend::set_viewport_geometry(u32 w, u32 h, u32& vOffset)
{
    set_viewport_geometry(w, h, m_viewport, vOffset);
}

void CRenderBackend::set_viewport_geometry(ref_geom geometry, u32& vOffset)
{
    u32 w = Device.dwWidth;
    u32 h = Device.dwHeight;
    set_viewport_geometry(w, h, geometry, vOffset);
}

void CRenderBackend::set_viewport_geometry(u32& vOffset)
{
    u32 w = Device.dwWidth;
    u32 h = Device.dwHeight;
    set_viewport_geometry(w, h, m_viewport, vOffset);
}

void CRenderBackend::render_viewport_geometry(u32 w, u32 h)
{
    u32 vOffset;
    set_viewport_geometry(w, h, m_viewport, vOffset);
    RenderBackend.Render(D3DPT_TRIANGLELIST, vOffset, 0, 4, 0, 2);
}

void CRenderBackend::RenderViewportSurface()
{
    u32 Offset = 0;
    set_viewport_geometry(Offset);
    RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderBackend::RenderViewportSurface(const ref_rt& rt_1, IDirect3DSurface9* zb)
{
    set_Render_Target_Surface(rt_1);
    set_Depth_Buffer(zb);
    render_viewport_geometry(rt_1->dwWidth, rt_1->dwHeight);
}

void CRenderBackend::RenderViewportSurface(u32 w, u32 h, IDirect3DSurface9* rt_1, IDirect3DSurface9* zb)
{
    set_Render_Target_Surface(w, h, rt_1);
    set_Depth_Buffer(zb);
    render_viewport_geometry(w, h);
}

void CRenderBackend::RenderViewportSurface(IDirect3DSurface9* rt_1)
{
    D3DSURFACE_DESC desc;
    HRESULT hr = rt_1->GetDesc(&desc);

    if (FAILED(hr))
        return;

    set_Render_Target_Surface(desc.Width, desc.Height, rt_1);
    set_Depth_Buffer(NULL);
    render_viewport_geometry(desc.Width, desc.Height);
}

void CRenderBackend::RenderViewportSurface(u32 w, u32 h, const ref_rt& rt_1, const ref_rt& rt_2, const ref_rt& rt_3, const ref_rt& rt_4)
{
    set_Render_Target_Surface(rt_1, rt_2, rt_3, rt_4);
    set_Depth_Buffer(NULL);
    render_viewport_geometry(w, h);
}

void CRenderBackend::RenderToMipLevel(ref_rt target, u32 mip_level)
{
    if (!target || !target->valid())
    {
        Msg("!CRenderBackend::RenderToMipLevel -  Texture is not present! (Name %s, level %d)", target->cName.c_str(), mip_level);
        return;
    }

    IDirect3DSurface9* mip_surface = target->get_surface_level(mip_level);
    if (!mip_surface)
    {
        Msg("!CRenderBackend::RenderToMipLevel -  mip level is not present! (Name %s, level %d)", target->cName.c_str(), mip_level);
        return;
    }

    u32 width, height;
    target->get_level_desc(mip_level, width, height);

    // Сохраняем состояние
    SaveRenderState();

    // Рендерим
    RenderViewportSurface(width, height, mip_surface);

    // Восстанавливаем состояние
    RestoreRenderState();

    mip_surface->Release();
}

void CRenderBackend::RenderToMipLevel(ref_rt target, u32 mip_level, ShaderElement* shader, u32 pass)
{
    if (!target || !target->valid())
        return;

    IDirect3DSurface9* mip_surface = target->get_surface_level(mip_level);
    if (!mip_surface)
        return;

    u32 width, height;
    target->get_level_desc(mip_level, width, height);

    // Сохраняем состояние
    SaveRenderState();

    // Устанавливаем шейдер
    set_Element(shader, pass);

    // Рендерим
    RenderViewportSurface(width, height, mip_surface);

    // Восстанавливаем состояние
    RestoreRenderState();

    mip_surface->Release();
}

// Генерация mip-цепочки
void CRenderBackend::GenerateMipChain(ref_rt source, ref_rt mip_chain, ShaderElement* downsample_shader, u32 pass)
{
    if (!source || !mip_chain || !source->valid() || !mip_chain->valid())
        return;

    // Копируем исходное изображение в уровень 0
    IDirect3DSurface9* src_surface = source->pRT;
    IDirect3DSurface9* dst_level0 = mip_chain->get_surface_level(0);

    if (src_surface && dst_level0)
    {
        RECT src_rect = { 0, 0, (LONG)source->dwWidth, (LONG)source->dwHeight };
        RECT dst_rect = { 0, 0, 64, 64 };
        RenderBackend.GetDevice()->StretchRect(src_surface, &src_rect, dst_level0, &dst_rect, D3DTEXF_LINEAR);
        dst_level0->Release();
    }

    // Генерируем остальные mip-уровни
    for (u32 i = 1; i < mip_chain->get_levels_count(); i++)
    {
        RenderToMipLevel(mip_chain, i, downsample_shader, pass);
    }
}

// Копирование содержимого из одного ref_rt в другой
void CRenderBackend::CopyViewportSurface(ref_rt source, ref_rt destination)
{
    if (!source || !destination || !source->valid() || !destination->valid())
    {
        Msg("! ERROR: CopyViewportSurface - invalid source or destination");
        return;
    }

    // Получаем поверхности
    IDirect3DSurface9* src_surface = source->pRT;
    IDirect3DSurface9* dst_surface = destination->pRT;

    if (!src_surface || !dst_surface)
    {
        Msg("! ERROR: CopyViewportSurface - failed to get surfaces");
        return;
    }

    // Определяем области копирования
    RECT src_rect = { 0, 0, (LONG)source->dwWidth, (LONG)source->dwHeight };
    RECT dst_rect = { 0, 0, (LONG)destination->dwWidth, (LONG)destination->dwHeight };

    // Выполняем копирование
    HRESULT hr = RenderBackend.GetDevice()->StretchRect(src_surface, &src_rect, dst_surface, &dst_rect, D3DTEXF_LINEAR);

    if (FAILED(hr))
    {
        Msg("! ERROR: CopyViewportSurface - StretchRect failed (0x%08x)", hr);
    }
}

// Версия с указанием фильтра
void CRenderBackend::CopyViewportSurface(ref_rt source, ref_rt destination, D3DTEXTUREFILTERTYPE filter)
{
    if (!source || !destination || !source->valid() || !destination->valid())
        return;

    IDirect3DSurface9* src_surface = source->pRT;
    IDirect3DSurface9* dst_surface = destination->pRT;

    if (!src_surface || !dst_surface)
        return;

    RECT src_rect = { 0, 0, (LONG)source->dwWidth, (LONG)source->dwHeight };
    RECT dst_rect = { 0, 0, (LONG)destination->dwWidth, (LONG)destination->dwHeight };

    RenderBackend.GetDevice()->StretchRect(src_surface, &src_rect, dst_surface, &dst_rect, filter);
}

// Версия с указанием конкретных областей
void CRenderBackend::CopyViewportSurface(ref_rt source, RECT src_rect, ref_rt destination, RECT dst_rect, D3DTEXTUREFILTERTYPE filter)
{
    if (!source || !destination || !source->valid() || !destination->valid())
        return;

    IDirect3DSurface9* src_surface = source->pRT;
    IDirect3DSurface9* dst_surface = destination->pRT;

    if (!src_surface || !dst_surface)
        return;

    RenderBackend.GetDevice()->StretchRect(src_surface, &src_rect, dst_surface, &dst_rect, filter);
}

void CRenderBackend::CopySurface(IDirect3DSurface9* source, IDirect3DSurface9* destination)
{
    if (!source || !destination)
    {
        Msg("! ERROR: CopySurface - invalid source or destination surface");
        return;
    }

    // Получаем описания поверхностей для проверки
    D3DSURFACE_DESC src_desc, dst_desc;
    HRESULT hr1 = source->GetDesc(&src_desc);
    HRESULT hr2 = destination->GetDesc(&dst_desc);

    if (FAILED(hr1) || FAILED(hr2))
    {
        Msg("! ERROR: CopySurface - failed to get surface descriptions");
        return;
    }

    // Определяем области копирования
    RECT src_rect = { 0, 0, (LONG)src_desc.Width, (LONG)src_desc.Height };
    RECT dst_rect = { 0, 0, (LONG)dst_desc.Width, (LONG)dst_desc.Height };

    // Выполняем копирование
    HRESULT hr = RenderBackend.GetDevice()->StretchRect(source, &src_rect, destination, &dst_rect, D3DTEXF_LINEAR);

    if (FAILED(hr))
    {
        Msg("! ERROR: CopySurface - StretchRect failed (0x%08x)", hr);
    }
}

// Версия с фильтром
void CRenderBackend::CopySurface(IDirect3DSurface9* source, IDirect3DSurface9* destination, D3DTEXTUREFILTERTYPE filter)
{
    if (!source || !destination)
        return;

    D3DSURFACE_DESC src_desc, dst_desc;
    if (FAILED(source->GetDesc(&src_desc)) || FAILED(destination->GetDesc(&dst_desc)))
        return;

    RECT src_rect = { 0, 0, (LONG)src_desc.Width, (LONG)src_desc.Height };
    RECT dst_rect = { 0, 0, (LONG)dst_desc.Width, (LONG)dst_desc.Height };

    RenderBackend.GetDevice()->StretchRect(source, &src_rect, destination, &dst_rect, filter);
}

// Версия с указанием областей
void CRenderBackend::CopySurface(IDirect3DSurface9* source, RECT src_rect, IDirect3DSurface9* destination, RECT dst_rect, D3DTEXTUREFILTERTYPE filter)
{
    if (!source || !destination)
        return;

    RenderBackend.GetDevice()->StretchRect(source, &src_rect, destination, &dst_rect, filter);
}
