////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#include <source/pch.h>
#include "BackendDX9.h"
////////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d9.lib")
////////////////////////////////////////////////////////////////////////////////
CRenderBackendDX9::CRenderBackendDX9() : m_pD3D(nullptr), m_pDevice(nullptr), m_hWnd(nullptr)
{
	ZeroMemory(&m_PP, sizeof(m_PP));
	m_backBufferFmt = D3DFMT_UNKNOWN;
}

CRenderBackendDX9::~CRenderBackendDX9()
{
	DestroyDevice();
}

D3DFORMAT CRenderBackendDX9::SelectDepthStencilFormat(D3DFORMAT backBufferFmt) const
{
	if(SUCCEEDED(m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFmt, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8)))
		return D3DFMT_D24S8;
	if(SUCCEEDED(m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFmt, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X8)))
		return D3DFMT_D24X8;
	if(SUCCEEDED(m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFmt, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D16)))
		return D3DFMT_D16;
	return D3DFMT_UNKNOWN;
}

bool CRenderBackendDX9::DetermineDepthAndBackBufferFormatsFromPresentParams(const RHI_PresentationParams& params,
																			D3DFORMAT& outBackBufferFmt,
																			D3DFORMAT& outDepthStencilFmt)
{
	if(params.BackBufferFormat != RHI_Format::Unknown)
	{
		outBackBufferFmt = RHIToD3DFormat(params.BackBufferFormat);
		if(outBackBufferFmt == D3DFMT_UNKNOWN)
		{
			Print("! [DX9] Unsupported backbuffer format %d, falling back", (int)params.BackBufferFormat);
			outBackBufferFmt = m_backBufferFmt;
		}
	}
	else
	{
		outBackBufferFmt = m_backBufferFmt;
	}

	outDepthStencilFmt = D3DFMT_UNKNOWN;
	if(params.EnableAutoDepthStencil)
	{
		if(params.DepthStencilFormat != RHI_Format::Unknown)
		{
			outDepthStencilFmt = RHIToD3DFormat(params.DepthStencilFormat);
			if(outDepthStencilFmt == D3DFMT_UNKNOWN)
				Print("! [DX9] Unsupported depth/stencil format %d, trying auto select",
					(int)params.DepthStencilFormat);
		}
		if(outDepthStencilFmt == D3DFMT_UNKNOWN)
			outDepthStencilFmt = SelectDepthStencilFormat(outBackBufferFmt);

		if(outDepthStencilFmt == D3DFMT_UNKNOWN)
		{
			Print("! [DX9] No suitable depth-stencil format found");
			return false;
		}
	}

	return true;
}

void CRenderBackendDX9::FillPresentParams(const RHI_PresentationParams& params,
										  D3DFORMAT backBufferFmt,
										  D3DFORMAT depthStencilFmt,
										  UINT fullscreenRefreshHz)
{
	ZeroMemory(&m_PP, sizeof(m_PP));
	m_PP.BackBufferWidth = params.BackBufferWidth;
	m_PP.BackBufferHeight = params.BackBufferHeight;
	m_PP.BackBufferFormat = backBufferFmt;
	m_PP.BackBufferCount = std::max(1u, std::min(3u, params.BackBufferCount));
	m_PP.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_PP.MultiSampleQuality = 0;

	switch(params.SwapEffect)
	{
	case RHI_SwapEffect::Flip:
		m_PP.SwapEffect = D3DSWAPEFFECT_FLIPEX;
		break;
	case RHI_SwapEffect::Discard:
	default:
		if(params.SwapEffect != RHI_SwapEffect::Discard)
			Print("! [DX9] Swap effect not supported, falling back to Discard");
		m_PP.SwapEffect = D3DSWAPEFFECT_DISCARD;
		break;
	}

	m_PP.hDeviceWindow = m_hWnd;
	m_PP.Windowed = params.Windowed;

	m_PP.EnableAutoDepthStencil = params.EnableAutoDepthStencil;
	if(params.EnableAutoDepthStencil)
		m_PP.AutoDepthStencilFormat = depthStencilFmt;

	m_PP.Flags = 0;

	if(!params.Windowed)
	{
		m_PP.FullScreen_RefreshRateInHz = (fullscreenRefreshHz > 0) ? fullscreenRefreshHz : 60;
	}
	else
	{
		m_PP.FullScreen_RefreshRateInHz = 0;
	}

	m_PP.PresentationInterval = (params.SyncInterval == 0) ? D3DPRESENT_INTERVAL_IMMEDIATE : D3DPRESENT_INTERVAL_ONE;
}

bool CRenderBackendDX9::CreateDevice(HWND hWnd, const RHI_PresentationParams& params)
{
	m_hWnd = hWnd;

	HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D);
	if(FAILED(hr) || !m_pD3D)
	{
		Print("! [DX9] Direct3DCreate9Ex failed (0x%08x)", hr);
		return false;
	}

	D3DADAPTER_IDENTIFIER9 adapterID;
	m_pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterID);
	Print("* [DX9] GPU [vendor:%X]-[device:%X]: %s", adapterID.VendorId, adapterID.DeviceId, adapterID.Description);

	D3DDISPLAYMODE d3ddm;
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	m_backBufferFmt = d3ddm.Format;
	m_DesktopRefreshRate = d3ddm.RefreshRate;

	D3DFORMAT backBufferFmt = D3DFMT_UNKNOWN;
	D3DFORMAT depthStencilFmt = D3DFMT_UNKNOWN;
	if(!DetermineDepthAndBackBufferFormatsFromPresentParams(params, backBufferFmt, depthStencilFmt))
		return false;

	m_backBufferFmt = backBufferFmt;

	UINT refreshHz = params.FullscreenRefreshHz ? params.FullscreenRefreshHz : m_DesktopRefreshRate;
	FillPresentParams(params, backBufferFmt, depthStencilFmt, refreshHz);

	DWORD vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;

	D3DDISPLAYMODEEX ModeEx;
	D3DDISPLAYMODEEX* pModeEx = nullptr;
	if(!params.Windowed)
	{
		ZeroMemory(&ModeEx, sizeof(ModeEx));
		ModeEx.Size = sizeof(ModeEx);
		ModeEx.Width = params.BackBufferWidth;
		ModeEx.Height = params.BackBufferHeight;
		ModeEx.Format = backBufferFmt;
		ModeEx.RefreshRate = m_PP.FullScreen_RefreshRateInHz;
		ModeEx.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
		pModeEx = &ModeEx;
	}

	hr = m_pD3D->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, vertexProcessing | D3DCREATE_MULTITHREADED, &m_PP, pModeEx, &m_pDevice);
	if(FAILED(hr))
	{
		Print("! [DX9] CreateDeviceEx failed (0x%08x), trying without MULTITHREADED", hr);
		hr = m_pD3D->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, vertexProcessing, &m_PP, pModeEx, &m_pDevice);
		if(FAILED(hr))
		{
			Print("! [DX9] Second attempt failed (0x%08x)", hr);
			return false;
		}
	}

	CacheBackBufferDimensions();
	CreateFullscreenGeometry();

	m_pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &m_AdapterID);
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_DesktopMode);
	CacheDeviceCapsFromD3D();

	m_deviceRemoved = false;

	Print("* [DX9] Device created successfully: %dx%d %s, interval=%d", m_backBufferWidth, m_backBufferHeight, params.Windowed ? "windowed" : "fullscreen", params.SyncInterval);
	return true;
}

void CRenderBackendDX9::DestroyDevice()
{
	DestroyFullscreenGeometry();
	ReleaseAllResources();
	ReleaseAllRTVDSV();
	InvalidateCache();
	m_backBufferWidth = 0;
	m_backBufferHeight = 0;
	if(m_pDevice)
		m_pDevice->Release();
	if(m_pD3D)
		m_pD3D->Release();
	m_pDevice = nullptr;
	m_pD3D = nullptr;
}

void CRenderBackendDX9::ReleaseAllResources()
{
	// --- Textures ---
	for (DX9Texture*& t : m_textures.Data())
	{
		if (!t)
			continue;
		if (t->tex2D)
			t->tex2D->Release();
		if (t->tex3D)
			t->tex3D->Release();
		if (t->texCube)
			t->texCube->Release();
		delete t;
		t = nullptr;
	}
	m_textures.Clear();

	// --- Buffers ---
	for (DX9Buffer*& buf : m_buffers.Data())
	{
		if (!buf)
			continue;
		if (buf->vb)
			buf->vb->Release();
		if (buf->ib)
			buf->ib->Release();
		delete buf;
		buf = nullptr;
	}
	m_buffers.Clear();

	// --- Input layouts ---
	for (DX9InputLayout*& lay : m_inputLayouts.Data())
	{
		if (!lay)
			continue;
		if (lay->decl)
			lay->decl->Release();
		delete lay;
		lay = nullptr;
	}
	m_inputLayouts.Clear();

	// --- Shaders ---
	for (DX9Shader*& sh : m_shaders.Data())
	{
		if (!sh)
			continue;
		if (sh->vs)
			sh->vs->Release();
		if (sh->ps)
			sh->ps->Release();
		if (sh->blob)
			sh->blob->Release();
		delete sh;
		sh = nullptr;
	}
	m_shaders.Clear();

	// --- Pipeline layouts (CPU-only) ---
	for (DX9PipelineLayout*& lay : m_pipelineLayouts.Data())
	{
		if (!lay)
			continue;
		delete lay;
		lay = nullptr;
	}
	m_pipelineLayouts.Clear();

	// --- Pipeline states (CPU-only) ---
	for (DX9PipelineState*& pso : m_pipelineStates.Data())
	{
		if (!pso)
			continue;
		delete pso;
		pso = nullptr;
	}
	m_pipelineStates.Clear();

	m_stateCache.Invalidate();

	// --- Cached state ---
	InvalidateGeometryCache();
}

void CRenderBackendDX9::ReleaseNativeDeviceResources()
{
	// --- Textures ---
	for (DX9Texture* t : m_textures.Data())
	{
		if (!t)
			continue;
		if (t->tex2D)
		{
			t->tex2D->Release();
			t->tex2D = nullptr;
		}
		if (t->texCube)
		{
			t->texCube->Release();
			t->texCube = nullptr;
		}
		if (t->tex3D)
		{
			t->tex3D->Release();
			t->tex3D = nullptr;
		}
	}

	// --- Buffers ---
	for (DX9Buffer* b : m_buffers.Data())
	{
		if (!b)
			continue;
		if (b->vb)
		{
			b->vb->Release();
			b->vb = nullptr;
		}
		if (b->ib)
		{
			b->ib->Release();
			b->ib = nullptr;
		}
	}

	// --- Input layouts ---
	for (DX9InputLayout* l : m_inputLayouts.Data())
	{
		if (!l)
			continue;
		if (l->decl)
		{
			l->decl->Release();
			l->decl = nullptr;
		}
	}

	// --- Shaders ---
	for (DX9Shader* s : m_shaders.Data())
	{
		if (!s)
			continue;
		if (s->vs)
		{
			s->vs->Release();
			s->vs = nullptr;
		}
		if (s->ps)
		{
			s->ps->Release();
			s->ps = nullptr;
		}
		// s->blob держим — это CPU-side bytecode, нужен для пересоздания.
	}

	// --- RTV/DSV surfaces ---
	for (auto& slot : m_rtvSlots)
		if (slot.surface)
		{
			slot.surface->Release();
			slot.surface = nullptr;
		}
	for (auto& slot : m_dsvSlots)
		if (slot.surface)
		{
			slot.surface->Release();
			slot.surface = nullptr;
		}

	InvalidateCache();
}

void CRenderBackendDX9::RecreateNativeDeviceResources()
{
	if (!m_pDevice)
		return;

	// --- Textures ---
	for (DX9Texture* t : m_textures.Data())
	{
		if (!t)
			continue;

		D3DFORMAT fmt = t->d3dFormat;
		if (fmt == D3DFMT_UNKNOWN)
			fmt = RHIToD3DFormat(t->format);
		if (fmt == D3DFMT_UNKNOWN)
			continue;

		DWORD usage = 0;
		if (t->isRenderTarget) usage |= D3DUSAGE_RENDERTARGET;
		if (t->isDepthStencil) usage |= D3DUSAGE_DEPTHSTENCIL;
		if (t->usage & RHI_TexUsage_GenerateMips) usage |= D3DUSAGE_AUTOGENMIPMAP;

		HRESULT hr = E_FAIL;
		switch (t->dim)
		{
		case RHI_TextureDim::Tex2D:
		case RHI_TextureDim::Tex1D:
		{
			IDirect3DTexture9* tex = nullptr;
			hr = m_pDevice->CreateTexture(t->width, t->height, t->mipLevels, usage, fmt, D3DPOOL_DEFAULT, &tex, nullptr);
			if (SUCCEEDED(hr))
				t->tex2D = tex;
			break;
		}
		case RHI_TextureDim::Cube:
		{
			IDirect3DCubeTexture9* tex = nullptr;
			hr = m_pDevice->CreateCubeTexture(t->width, t->mipLevels, usage, fmt, D3DPOOL_DEFAULT, &tex, nullptr);
			if (SUCCEEDED(hr))
				t->texCube = tex;
			break;
		}
		case RHI_TextureDim::Tex3D:
		{
			IDirect3DVolumeTexture9* tex = nullptr;
			hr = m_pDevice->CreateVolumeTexture(t->width, t->height, t->depth, t->mipLevels, usage, fmt, D3DPOOL_DEFAULT, &tex, nullptr);
			if (SUCCEEDED(hr))
				t->tex3D = tex;
			break;
		}
		}
		if (FAILED(hr))
			Print("! [DX9] Reset: failed to recreate texture (fmt=%u, %ux%u)", (uint32_t)t->format, t->width, t->height);
	}

	// --- Buffers ---
	for (DX9Buffer* b : m_buffers.Data())
	{
		if (!b)
			continue;

		DWORD usage = 0;
		if (b->desc.usage & RHI_BufferUsage_Dynamic)
			usage |= D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;

		if (b->isIndex)
		{
			const D3DFORMAT fmt = (b->desc.indexFormat == RHI_IndexFormat::UInt32) ? D3DFMT_INDEX32 : D3DFMT_INDEX16;
			IDirect3DIndexBuffer9* ib = nullptr;
			if (SUCCEEDED(m_pDevice->CreateIndexBuffer(b->desc.sizeBytes, usage, fmt, D3DPOOL_DEFAULT, &ib, nullptr)))
				b->ib = ib;
		}
		else
		{
			IDirect3DVertexBuffer9* vb = nullptr;
			if (SUCCEEDED(m_pDevice->CreateVertexBuffer(b->desc.sizeBytes, usage, 0, D3DPOOL_DEFAULT, &vb, nullptr)))
				b->vb = vb;
		}
	}

	// --- Input layouts ---
	for (DX9InputLayout* l : m_inputLayouts.Data())
	{
		if (!l || l->desc.elements.empty())
			continue;

		std::vector<D3DVERTEXELEMENT9> elems;
		elems.reserve(l->desc.elements.size() + 1);
		for (const auto& e : l->desc.elements)
		{
			D3DVERTEXELEMENT9 de{};
			de.Stream = (WORD)e.stream;
			de.Offset = (WORD)e.offset;
			de.Type = ToD3DDeclType(e.type);
			de.Method = D3DDECLMETHOD_DEFAULT;
			de.Usage = ToD3DDeclUsage(e.semantic);
			de.UsageIndex = (BYTE)e.semanticIndex;
			elems.push_back(de);
		}
		elems.push_back(D3DDECL_END());

		IDirect3DVertexDeclaration9* decl = nullptr;
		if (SUCCEEDED(m_pDevice->CreateVertexDeclaration(elems.data(), &decl)))
			l->decl = decl;
	}

	// --- Shaders ---
	for (DX9Shader* s : m_shaders.Data())
	{
		if (!s || !s->blob)
			continue;
		const DWORD* code = static_cast<const DWORD*>(s->blob->GetBufferPointer());

		if (s->isVertex)
		{
			IDirect3DVertexShader9* vs = nullptr;
			if (SUCCEEDED(m_pDevice->CreateVertexShader(code, &vs)))
				s->vs = vs;
		}
		else
		{
			IDirect3DPixelShader9* ps = nullptr;
			if (SUCCEEDED(m_pDevice->CreatePixelShader(code, &ps)))
				s->ps = ps;
		}
	}
}

bool CRenderBackendDX9::Reset(const RHI_PresentationParams& params)
{
	if(!m_pDevice)
		return false;

	D3DFORMAT backBufferFmt = m_backBufferFmt;
	D3DFORMAT depthStencilFmt = D3DFMT_UNKNOWN;

	if(!DetermineDepthAndBackBufferFormatsFromPresentParams(params, backBufferFmt, depthStencilFmt))
		return false;

	UINT refreshHz = params.FullscreenRefreshHz ? params.FullscreenRefreshHz : m_DesktopRefreshRate;

	FillPresentParams(params, backBufferFmt, depthStencilFmt, refreshHz);

	InvalidateUserRTVDSVOnReset();
	ReleaseNativeDeviceResources();
	DestroyFullscreenGeometry();

	HRESULT hr = m_pDevice->Reset(&m_PP);
	if(FAILED(hr))
	{
		Print("! [DX9] Reset failed (0x%08x)", hr);
		return false;
	}

	RecreateNativeDeviceResources();
	CreateFullscreenGeometry();
	CacheBackBufferDimensions();
	CreateFullscreenGeometry();

	m_pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &m_AdapterID);
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_DesktopMode);
	CacheDeviceCapsFromD3D();

	Print("* [DX9] Device reset successfully: %dx%d %s, interval=%d", m_backBufferWidth,
																	  m_backBufferHeight,
																	  params.Windowed ? "windowed" : "fullscreen",
																	  params.SyncInterval);
	return true;
}

RHI_DeviceStatus CRenderBackendDX9::CheckDeviceStatus() const
{
	if (!m_pDevice || m_deviceRemoved) 
		return RHI_DeviceStatus::Lost;

	const HRESULT hr = m_pDevice->TestCooperativeLevel();

	switch(hr)
	{
	case D3D_OK:
		return RHI_DeviceStatus::OK;

	case D3DERR_DEVICENOTRESET:
		// Устройство потеряно, но может быть восстановлено через Reset().
		// Классический случай: fullscreen alt-tab, смена разрешения монитора.
		return RHI_DeviceStatus::NeedReset;

	case D3DERR_DEVICELOST:
		// Устройство потеряно безвозвратно или ещё не готово к reset.
		// D3D9 часто мигает между LOST и NOTRESET несколько кадров —
		// движок должен просто подождать.
		return RHI_DeviceStatus::Lost;

	default:
		Print("! [DX9] TestCooperativeLevel returned unexpected 0x%08x", hr);
		return RHI_DeviceStatus::Lost;
	}
}

void CRenderBackendDX9::Present()
{
	if (!m_pDevice) 
		return;

	const HRESULT hr = m_pDevice->PresentEx(nullptr, nullptr, nullptr, nullptr, 0);

	if (hr == D3DERR_DEVICEREMOVED)
	{
		m_deviceRemoved = true;
		return;
	}

	if (FAILED(hr) && hr != D3DERR_DEVICELOST && hr != D3DERR_DEVICENOTRESET)
		Print("! [DX9] PresentEx failed (0x%08x)", hr);
}

void CRenderBackendDX9::OnFrameBegin()
{
	m_pDevice->BeginScene();
}

void CRenderBackendDX9::OnFrameEnd()
{
	m_pDevice->EndScene();
}

void CRenderBackendDX9::Clear(uint32_t clearFlags, const Float4 color, float depth, uint8_t stencil)
{
	if(!m_pDevice)
		return;

	DWORD d3dFlags = 0;
	if(clearFlags & RHI_CLEAR_TARGET)
		d3dFlags |= D3DCLEAR_TARGET;
	if(clearFlags & RHI_CLEAR_ZBUFFER)
		d3dFlags |= D3DCLEAR_ZBUFFER;
	if(clearFlags & RHI_CLEAR_STENCIL)
		d3dFlags |= D3DCLEAR_STENCIL;

	D3DCOLOR d3dColor = D3DCOLOR_COLORVALUE(color.x, color.y, color.z, color.w);
	m_pDevice->Clear(0, nullptr, d3dFlags, d3dColor, depth, stencil);
}

void CRenderBackendDX9::GetAvailableResolutions(RHI_Format format, std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const
{
	outResolutions.clear();
	if(!m_pD3D)
		return;

	D3DFORMAT d3dFmt = RHIToD3DFormat(format);
	if(d3dFmt == D3DFMT_UNKNOWN)
		return;

	UINT adapter = D3DADAPTER_DEFAULT;
	UINT modeCount = m_pD3D->GetAdapterModeCount(adapter, d3dFmt);

	for(UINT i = 0; i < modeCount; ++i)
	{
		D3DDISPLAYMODE mode;
		if(FAILED(m_pD3D->EnumAdapterModes(adapter, d3dFmt, i, &mode)))
			continue;

		auto it = std::find_if(outResolutions.begin(), outResolutions.end(),
							   [&](const auto& r)
							   { return r.first == mode.Width && r.second == mode.Height; });
		if(it == outResolutions.end())
			outResolutions.emplace_back(mode.Width, mode.Height);
	}
}

RHI_Format CRenderBackendDX9::GetBackBufferFormat() const
{
	return D3DFormatToRHI(m_backBufferFmt);
}

void* CRenderBackendDX9::GetShaderNativeHandle(RHI_ShaderHandle h)
{
	DX9Shader* sh = GetShader(h);
	if (!sh) return nullptr;
	return sh->isVertex ? static_cast<void*>(sh->vs) : static_cast<void*>(sh->ps);
}

void* CRenderBackendDX9::GetInputLayoutNativeHandle(RHI_InputLayoutHandle h)
{
	DX9InputLayout* l = GetInputLayout(h);
	return l ? static_cast<void*>(l->decl) : nullptr;
}
////////////////////////////////////////////////////////////////////////////////
