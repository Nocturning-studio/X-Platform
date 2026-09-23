#include "stdafx.h"
#pragma hdrstop

#include "R_Backend.h"
#include "ResourceManager.h"
#include "xr_IOconsole.h"
#include <SDL3/SDL.h>

ENGINE_API extern int psAnisotropic;

ENGINE_API CRenderBackend RenderBackend;

xr_token* vid_mode_token = NULL;

static void free_vid_mode_list()
{
	if(vid_mode_token)
	{
		for(int i = 0; vid_mode_token[i].name; i++)
			xr_free(vid_mode_token[i].name);
		xr_free(vid_mode_token);
		vid_mode_token = NULL;
	}
}

static void fill_vid_mode_list()
{
	if(vid_mode_token != NULL)
		return;

	xr_vector<std::pair<u32, u32>> resolutions;

	// Собираем все режимы со всех дисплеев
	int display_count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&display_count);
	if(displays && display_count > 0)
	{
		for(int d = 0; d < display_count; ++d)
		{
			int mode_count = 0;
			SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displays[d], &mode_count);
			if(!modes)
				continue;

			for(int m = 0; m < mode_count; ++m)
			{
				const SDL_DisplayMode* mode = modes[m];
				if(!mode)
					continue;
				resolutions.emplace_back((u32)mode->w, (u32)mode->h);
			}

			SDL_free(modes);
		}
		SDL_free(displays);
	}

	// Fallback: текущий desktop-режим основного дисплея
	if(resolutions.empty())
	{
		SDL_DisplayID primary = SDL_GetPrimaryDisplay();
		const SDL_DisplayMode* dm = SDL_GetDesktopDisplayMode(primary);
		if(dm)
			resolutions.emplace_back((u32)dm->w, (u32)dm->h);
	}

	// Сортируем по возрастанию (ширина, затем высота)
	std::sort(resolutions.begin(), resolutions.end(),
			  [](const std::pair<u32, u32>& a, const std::pair<u32, u32>& b)
			  {
				  return a.first != b.first ? a.first < b.first : a.second < b.second;
			  });

	// Убираем дубликаты
	resolutions.erase(std::unique(resolutions.begin(), resolutions.end()), resolutions.end());

	// Формируем токены
	xr_vector<LPCSTR> _tmp;
	_tmp.reserve(resolutions.size());
	for(const auto& res : resolutions)
	{
		string32 str;
		sprintf_s(str, sizeof(str), "%dx%d", res.first, res.second);
		_tmp.push_back(xr_strdup(str));
	}

	u32 _cnt = _tmp.size() + 1;
	vid_mode_token = xr_alloc<xr_token>(_cnt);
	vid_mode_token[_cnt - 1].id = -1;
	vid_mode_token[_cnt - 1].name = NULL;

	for(u32 i = 0; i < _tmp.size(); ++i)
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
	if(!m_hRHI_DLL)
	{
		Msg("! Failed to load xrRHI.dll");
		FlushLog();
		MessageBox(NULL, "Failed to load xrRHI.dll", "Fatal Error", MB_OK | MB_ICONERROR);
		TerminateProcess(GetCurrentProcess(), 0);
		return;
	}

	typedef IRenderBackend* (*CreateBackendFunc)(RHI_BackendType);
	CreateBackendFunc createBackend = (CreateBackendFunc)GetProcAddress(m_hRHI_DLL, "CreateRenderBackend");
	if(!createBackend)
	{
		Msg("! Failed to get CreateRenderBackend function from xrRHI.dll");
		FlushLog();
		MessageBox(NULL, "Invalid xrRHI.dll", "Fatal Error", MB_OK | MB_ICONERROR);
		TerminateProcess(GetCurrentProcess(), 0);
		return;
	}

	RHI_BackendType desiredType = RHI_BackendType::DirectX9;
	m_pRHI = createBackend(desiredType);
	if(!m_pRHI)
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
	SelectResolution(width, height, bWindowed);
	u32 presentInterval = SelectPresentInterval();
	u32 refreshHz = D3DPRESENT_RATE_DEFAULT;

	CWindowManager& wm = Engine.WindowManager;
	wm.SetWindowed(bWindowed);
	wm.SetResolution(width, height);
	wm.SetRefreshRate(refreshHz);
	wm.Apply();

	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	LONG clientW = rcClient.right - rcClient.left;
	LONG clientH = rcClient.bottom - rcClient.top;

	RHI_PresentationParams params;
	params.BackBufferWidth = clientW;
	params.BackBufferHeight = clientH;
	params.Windowed = bWindowed;
	params.BackBufferFormat = RHI_Format::RGBA8_UNORM;
	params.DepthStencilFormat = RHI_Format::D24_UNORM_S8_UINT;
	params.BackBufferCount = 2;
	params.SyncInterval = (presentInterval == 0) ? 0 : 1;
	params.FullscreenRefreshHz = refreshHz;
	params.SwapEffect = RHI_SwapEffect::Discard;
	params.EnableAutoDepthStencil = true;

	if(!m_pRHI->CreateDevice(m_hWnd, params))
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
	vp.X = 0;
	vp.Y = 0;
	vp.Width = desc.Width;
	vp.Height = desc.Height;
	vp.MinZ = 0.0f;
	vp.MaxZ = 1.0f;
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

	fill_vid_mode_list();

	Msg("* RHI backend initialized successfully.");
}

void CRenderBackend::Destroy()
{
	if(m_pRHI)
	{
		m_pRHI->DestroyDevice();
		delete m_pRHI;
		m_pRHI = nullptr;
	}
	if(m_hRHI_DLL)
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

	if(!m_pRHI)
		return;

#ifndef DEDICATED_SERVER
	BOOL bWindowed = strstr(Core.Params, "-windowed") ? TRUE : !psDeviceFlags.is(rsFullscreen);
#else
	BOOL bWindowed = TRUE;
#endif

	u32 width, height;
	SelectResolution(width, height, bWindowed);
	u32 presentInterval = SelectPresentInterval();
	u32 refreshHz = D3DPRESENT_RATE_DEFAULT;

	CWindowManager& wm = Engine.WindowManager;
	wm.SetWindowed(bWindowed);
	wm.SetResolution(width, height);
	wm.SetRefreshRate(refreshHz);
	wm.Apply();

	HWND hWnd = wm.GetHandle();

	RECT rcClient;
	GetClientRect(hWnd, &rcClient);
	LONG clientW = rcClient.right - rcClient.left;
	LONG clientH = rcClient.bottom - rcClient.top;

	RHI_PresentationParams params;
	params.BackBufferWidth = clientW;
	params.BackBufferHeight = clientH;
	params.Windowed = bWindowed;
	params.BackBufferFormat = RHI_Format::RGBA8_UNORM;
	params.DepthStencilFormat = RHI_Format::D24_UNORM_S8_UINT;
	params.BackBufferCount = 1;
	params.SyncInterval = (presentInterval == 0) ? 0 : 1;
	params.FullscreenRefreshHz = refreshHz;
	params.SwapEffect = RHI_SwapEffect::Discard;
	params.EnableAutoDepthStencil = true;

	if(!m_pRHI->Reset(params))
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
	vp.X = 0;
	vp.Y = 0;
	vp.Width = desc.Width;
	vp.Height = desc.Height;
	vp.MinZ = 0.0f;
	vp.MaxZ = 1.0f;
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

bool CRenderBackend::NeedReset()
{
	HRESULT _hr = RenderBackend.GetDevice()->TestCooperativeLevel();
	if(FAILED(_hr) && D3DERR_DEVICENOTRESET == _hr)
		return true;
	else
		return false;
}

void CRenderBackend::SelectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed)
{
#ifdef DEDICATED_SERVER
	dwWidth = 32;
	dwHeight = 32;
#else
	dwWidth = psCurrentVidMode[0];
	dwHeight = psCurrentVidMode[1];
#endif
}

u32 CRenderBackend::SelectPresentInterval()
{
#ifdef DEDICATED_SERVER
	return D3DPRESENT_INTERVAL_IMMEDIATE;
#else
	if(!psDeviceFlags.test(rsVSync))
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
		for(int i = 0; i < dwTriCount; i++)
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
	m_constantMgr.ResetDirty();
	m_viewport.create(FVF::F_TL, Vertex.Buffer(), QuadIB);
}

void CRenderBackend::OnDeviceDestroy()
{
	Index.Destroy();
	Vertex.Destroy();
	m_constantMgr.ResetDirty();
	_RELEASE(QuadIB);
}

void CRenderBackend::ResetBegin()
{
	m_constantMgr.ForceDirty();
	m_stateCache.Invalidate(*this);
	m_resBinder.Invalidate(*this);
}

void CRenderBackend::ResetEnd()
{
	m_constantMgr.ResetDirty();
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

void CRenderBackend::SetBlend(BOOL enable, D3DBLEND src, D3DBLEND dest)
{
	m_stateCache.SetBlend(GetDevice(), enable, src, dest);
}

void CRenderBackend::SetBlendAlpha()
{
	SetBlend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
}

void CRenderBackend::SetBlendAdd()
{
	SetBlend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
}

void CRenderBackend::SetBlendMultiply()
{
	SetBlend(TRUE, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO);
}

void CRenderBackend::SetBlendDefault()
{
	SetBlend(FALSE, D3DBLEND_ONE, D3DBLEND_ZERO);
}

void CRenderBackend::SetBlendSubstract()
{
	SetBlend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
	SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT);
}

void CRenderBackend::SetBlendScreen()
{
	SetBlend(TRUE, D3DBLEND_ONE, D3DBLEND_INVSRCCOLOR);
}

void CRenderBackend::SetBlendLightAdd()
{
	SetBlend(TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
}

void CRenderBackend::SetBlendColorAdd()
{
	SetBlend(TRUE, D3DBLEND_SRCCOLOR, D3DBLEND_ONE);
}

void CRenderBackend::SetBlendEx(BOOL enable, D3DBLEND src, D3DBLEND dest, D3DBLENDOP op)
{
	m_stateCache.SetBlendEx(GetDevice(), enable, src, dest, op);
}

BOOL CRenderBackend::GetBlendState() const
{
	return m_stateCache.GetBlendEnable();
}

D3DBLEND CRenderBackend::GetSrcBlend() const
{
	return m_stateCache.GetSrcBlend();
}

D3DBLEND CRenderBackend::GetDstBlend() const
{
	return m_stateCache.GetDstBlend();
}

void CRenderBackend::EnableAnisotropyFiltering()
{
	for(u32 i = 0; i < RHI()->GetDeviceCaps().MaxSimultaneousTextures; i++)
		CHK_DX(m_pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, psAnisotropic));
}

void CRenderBackend::DisableAnisotropyFiltering()
{
	for(u32 i = 0; i < RHI()->GetDeviceCaps().MaxSimultaneousTextures; i++)
		CHK_DX(m_pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 1));
}

void CRenderBackend::SetAnisotropyFiltering(int max_anisothropy)
{
	for(u32 i = 0; i < RHI()->GetDeviceCaps().MaxSimultaneousTextures; i++)
		CHK_DX(m_pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, max_anisothropy));
}

void CRenderBackend::Invalidate()
{
	m_stateCache.Invalidate(*this);
	m_resBinder.Invalidate(*this);
}

void CRenderBackend::OnFrameBegin()
{
#ifndef DEDICATED_SERVER
	std::memset(&stat, 0, sizeof(stat));
	Vertex.Flush();
	Index.Flush();
	SetStencil(FALSE);
	RHI()->OnFrameBegin();
#endif
}

void CRenderBackend::OnFrameEnd()
{
#ifndef DEDICATED_SERVER
	RHI()->OnFrameEnd();
	Invalidate();
#endif
}

void CRenderBackend::Present()
{
	PROFILE_FUNCTION();

	RHI()->Present();
}

#ifndef DEDICATED_SERVER

void CRenderBackend::SetTextures(STextureList* _T)
{
	m_resBinder.SetTextures(*this, _T);
}
#else

void CRenderBackend::SetTextures(STextureList* _T)
{
}

#endif

void CRenderBackend::SetRenderTarget(const ref_rt& rt_1, const ref_rt& rt_2, const ref_rt& rt_3, const ref_rt& rt_4)
{
	VERIFY2(rt_1, "Rendertarget must have minimum one target surface (ref_rt& rt_1)");

	RenderBackend.SetRenderTargetSurface(rt_1->pRT, 0);

	if(rt_2)
		RenderBackend.SetRenderTargetSurface(rt_2->pRT, 1);
	else
		RenderBackend.SetRenderTargetSurface(NULL, 1);

	if(rt_3)
		RenderBackend.SetRenderTargetSurface(rt_3->pRT, 2);
	else
		RenderBackend.SetRenderTargetSurface(NULL, 2);

	if(rt_4)
		RenderBackend.SetRenderTargetSurface(rt_4->pRT, 3);
	else
		RenderBackend.SetRenderTargetSurface(NULL, 3);
}

void CRenderBackend::SetRenderTarget(u32 W, u32 H, IDirect3DSurface9* rt_1, IDirect3DSurface9* rt_2, IDirect3DSurface9* rt_3, IDirect3DSurface9* rt_4)
{
	VERIFY2(rt_1, "Rendertarget must have minimum one target surface (IDirect3DSurface9* rt_1)");

	RenderBackend.SetRenderTargetSurface(rt_1, 0);

	if(rt_2)
		RenderBackend.SetRenderTargetSurface(rt_2, 1);
	else
		RenderBackend.SetRenderTargetSurface(NULL, 1);

	if(rt_3)
		RenderBackend.SetRenderTargetSurface(rt_3, 2);
	else
		RenderBackend.SetRenderTargetSurface(NULL, 2);

	if(rt_4)
		RenderBackend.SetRenderTargetSurface(rt_4, 3);
	else
		RenderBackend.SetRenderTargetSurface(NULL, 3);
}

void CRenderBackend::SetDepthBuffer(IDirect3DSurface9* zb)
{
	RenderBackend.SetDepthBufferSurface(zb);
}

void CRenderBackend::ClearDepthBuffer(IDirect3DSurface9* zb)
{
	RenderBackend.SetDepthBuffer(zb);
	CHK_DX(RenderBackend.GetDevice()->Clear(0L, nullptr, D3DCLEAR_ZBUFFER, 0x0, 1.0f, 0L));
}

// 2D texgen (texture adjustment matrix)
void CRenderBackend::ComputeTexgenScreen(fmat4x4& m_Texgen)
{
	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);
	float o_w = (.5f / _w);
	float o_h = (.5f / _h);
	fmat4x4 m_TexelAdjust = {0.5f, 0.0f, 0.0f, 0.0f,
							 0.0f, -0.5f, 0.0f, 0.0f,
							 0.0f, 0.0f, 1.0f, 0.0f,
							 0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f};
	m_Texgen.mul(m_TexelAdjust, RenderBackend.transforms.m_WorldViewProject);
}

void CRenderBackend::SetViewportGeom(u32 w, u32 h, ref_geom geometry, u32& vOffset)
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
	RenderBackend.SetGeometry(geometry);
}

void CRenderBackend::SetViewportGeom(u32 w, u32 h, u32& vOffset)
{
	SetViewportGeom(w, h, m_viewport, vOffset);
}

void CRenderBackend::SetViewportGeom(ref_geom geometry, u32& vOffset)
{
	u32 w = Device.dwWidth;
	u32 h = Device.dwHeight;
	SetViewportGeom(w, h, geometry, vOffset);
}

void CRenderBackend::SetViewportGeom(u32& vOffset)
{
	u32 w = Device.dwWidth;
	u32 h = Device.dwHeight;
	SetViewportGeom(w, h, m_viewport, vOffset);
}

void CRenderBackend::RenderViewportGeom(u32 w, u32 h)
{
	u32 vOffset;
	SetViewportGeom(w, h, m_viewport, vOffset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, vOffset, 0, 4, 0, 2);
}

void CRenderBackend::RenderViewportSurface()
{
	u32 Offset = 0;
	SetViewportGeom(Offset);
	RenderBackend.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRenderBackend::RenderViewportSurface(const ref_rt& rt_1, IDirect3DSurface9* zb)
{
	SetRenderTarget(rt_1);
	SetDepthBuffer(zb);
	RenderViewportGeom(rt_1->dwWidth, rt_1->dwHeight);
}

void CRenderBackend::RenderViewportSurface(u32 w, u32 h, IDirect3DSurface9* rt_1, IDirect3DSurface9* zb)
{
	SetRenderTarget(w, h, rt_1);
	SetDepthBuffer(zb);
	RenderViewportGeom(w, h);
}

void CRenderBackend::RenderViewportSurface(IDirect3DSurface9* rt_1)
{
	D3DSURFACE_DESC desc;
	HRESULT hr = rt_1->GetDesc(&desc);

	if(FAILED(hr))
		return;

	SetRenderTarget(desc.Width, desc.Height, rt_1);
	SetDepthBuffer(NULL);
	RenderViewportGeom(desc.Width, desc.Height);
}

void CRenderBackend::RenderViewportSurface(u32 w, u32 h, const ref_rt& rt_1, const ref_rt& rt_2, const ref_rt& rt_3, const ref_rt& rt_4)
{
	SetRenderTarget(rt_1, rt_2, rt_3, rt_4);
	SetDepthBuffer(NULL);
	RenderViewportGeom(w, h);
}

void CRenderBackend::RenderToMipLevel(ref_rt target, u32 mip_level)
{
	if(!target || !target->valid())
	{
		Msg("!CRenderBackend::RenderToMipLevel -  Texture is not present! (Name %s, level %d)", target->cName.c_str(), mip_level);
		return;
	}

	IDirect3DSurface9* mip_surface = target->get_surface_level(mip_level);
	if(!mip_surface)
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
	if(!target || !target->valid())
		return;

	IDirect3DSurface9* mip_surface = target->get_surface_level(mip_level);
	if(!mip_surface)
		return;

	u32 width, height;
	target->get_level_desc(mip_level, width, height);

	// Сохраняем состояние
	SaveRenderState();

	// Устанавливаем шейдер
	SetShaderElement(shader, pass);

	// Рендерим
	RenderViewportSurface(width, height, mip_surface);

	// Восстанавливаем состояние
	RestoreRenderState();

	mip_surface->Release();
}

// Генерация mip-цепочки
void CRenderBackend::GenerateMipChain(ref_rt source, ref_rt mip_chain, ShaderElement* downsample_shader, u32 pass)
{
	if(!source || !mip_chain || !source->valid() || !mip_chain->valid())
		return;

	// Копируем исходное изображение в уровень 0
	IDirect3DSurface9* src_surface = source->pRT;
	IDirect3DSurface9* dst_level0 = mip_chain->get_surface_level(0);

	if(src_surface && dst_level0)
	{
		RECT src_rect = {0, 0, (LONG)source->dwWidth, (LONG)source->dwHeight};
		RECT dst_rect = {0, 0, 64, 64};
		RenderBackend.GetDevice()->StretchRect(src_surface, &src_rect, dst_level0, &dst_rect, D3DTEXF_LINEAR);
		dst_level0->Release();
	}

	// Генерируем остальные mip-уровни
	for(u32 i = 1; i < mip_chain->get_levels_count(); i++)
	{
		RenderToMipLevel(mip_chain, i, downsample_shader, pass);
	}
}

// Копирование содержимого из одного ref_rt в другой
void CRenderBackend::CopyViewportSurface(ref_rt source, ref_rt destination)
{
	if(!source || !destination || !source->valid() || !destination->valid())
	{
		Msg("! ERROR: CopyViewportSurface - invalid source or destination");
		return;
	}

	// Получаем поверхности
	IDirect3DSurface9* src_surface = source->pRT;
	IDirect3DSurface9* dst_surface = destination->pRT;

	if(!src_surface || !dst_surface)
	{
		Msg("! ERROR: CopyViewportSurface - failed to get surfaces");
		return;
	}

	// Определяем области копирования
	RECT src_rect = {0, 0, (LONG)source->dwWidth, (LONG)source->dwHeight};
	RECT dst_rect = {0, 0, (LONG)destination->dwWidth, (LONG)destination->dwHeight};

	// Выполняем копирование
	HRESULT hr = RenderBackend.GetDevice()->StretchRect(src_surface, &src_rect, dst_surface, &dst_rect, D3DTEXF_LINEAR);

	if(FAILED(hr))
	{
		Msg("! ERROR: CopyViewportSurface - StretchRect failed (0x%08x)", hr);
	}
}

// Версия с указанием фильтра
void CRenderBackend::CopyViewportSurface(ref_rt source, ref_rt destination, D3DTEXTUREFILTERTYPE filter)
{
	if(!source || !destination || !source->valid() || !destination->valid())
		return;

	IDirect3DSurface9* src_surface = source->pRT;
	IDirect3DSurface9* dst_surface = destination->pRT;

	if(!src_surface || !dst_surface)
		return;

	RECT src_rect = {0, 0, (LONG)source->dwWidth, (LONG)source->dwHeight};
	RECT dst_rect = {0, 0, (LONG)destination->dwWidth, (LONG)destination->dwHeight};

	RenderBackend.GetDevice()->StretchRect(src_surface, &src_rect, dst_surface, &dst_rect, filter);
}

// Версия с указанием конкретных областей
void CRenderBackend::CopyViewportSurface(ref_rt source, RECT src_rect, ref_rt destination, RECT dst_rect, D3DTEXTUREFILTERTYPE filter)
{
	if(!source || !destination || !source->valid() || !destination->valid())
		return;

	IDirect3DSurface9* src_surface = source->pRT;
	IDirect3DSurface9* dst_surface = destination->pRT;

	if(!src_surface || !dst_surface)
		return;

	RenderBackend.GetDevice()->StretchRect(src_surface, &src_rect, dst_surface, &dst_rect, filter);
}

void CRenderBackend::CopySurface(IDirect3DSurface9* source, IDirect3DSurface9* destination)
{
	if(!source || !destination)
	{
		Msg("! ERROR: CopySurface - invalid source or destination surface");
		return;
	}

	// Получаем описания поверхностей для проверки
	D3DSURFACE_DESC src_desc, dst_desc;
	HRESULT hr1 = source->GetDesc(&src_desc);
	HRESULT hr2 = destination->GetDesc(&dst_desc);

	if(FAILED(hr1) || FAILED(hr2))
	{
		Msg("! ERROR: CopySurface - failed to get surface descriptions");
		return;
	}

	// Определяем области копирования
	RECT src_rect = {0, 0, (LONG)src_desc.Width, (LONG)src_desc.Height};
	RECT dst_rect = {0, 0, (LONG)dst_desc.Width, (LONG)dst_desc.Height};

	// Выполняем копирование
	HRESULT hr = RenderBackend.GetDevice()->StretchRect(source, &src_rect, destination, &dst_rect, D3DTEXF_LINEAR);

	if(FAILED(hr))
	{
		Msg("! ERROR: CopySurface - StretchRect failed (0x%08x)", hr);
	}
}

// Версия с фильтром
void CRenderBackend::CopySurface(IDirect3DSurface9* source, IDirect3DSurface9* destination, D3DTEXTUREFILTERTYPE filter)
{
	if(!source || !destination)
		return;

	D3DSURFACE_DESC src_desc, dst_desc;
	if(FAILED(source->GetDesc(&src_desc)) || FAILED(destination->GetDesc(&dst_desc)))
		return;

	RECT src_rect = {0, 0, (LONG)src_desc.Width, (LONG)src_desc.Height};
	RECT dst_rect = {0, 0, (LONG)dst_desc.Width, (LONG)dst_desc.Height};

	RenderBackend.GetDevice()->StretchRect(source, &src_rect, destination, &dst_rect, filter);
}

// Версия с указанием областей
void CRenderBackend::CopySurface(IDirect3DSurface9* source, RECT src_rect, IDirect3DSurface9* destination, RECT dst_rect, D3DTEXTUREFILTERTYPE filter)
{
	if(!source || !destination)
		return;

	RenderBackend.GetDevice()->StretchRect(source, &src_rect, destination, &dst_rect, filter);
}
