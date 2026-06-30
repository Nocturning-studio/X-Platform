#include "pch.h"
#include "xrBackendDX9.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

RHI_BEGIN

CRenderBackendDX9::CRenderBackendDX9() : m_pD3D(nullptr), m_pDevice(nullptr), m_hWnd(nullptr)
{
	ZeroMemory(&m_PP, sizeof(m_PP));
	m_BackBufferFmt = D3DFMT_UNKNOWN;
}

CRenderBackendDX9::~CRenderBackendDX9()
{
	ReleaseAllResources();
	DestroyDevice();
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

bool CRenderBackendDX9::DetermineDepthAndBackBufferFormatsFromPresentParams(const RHIPresentationParams& params,
	D3DFORMAT& outBackBufferFmt,
	D3DFORMAT& outDepthStencilFmt)
{
	if (params.BackBufferFormat != RHI_Format::Unknown)
	{
		outBackBufferFmt = RHIToD3DFormat(params.BackBufferFormat);
		if (outBackBufferFmt == D3DFMT_UNKNOWN)
		{
			Print("! [DX9] Unsupported backbuffer format %d, falling back", (int)params.BackBufferFormat);
			outBackBufferFmt = m_BackBufferFmt;
		}
	}
	else
	{
		outBackBufferFmt = m_BackBufferFmt;
	}

	outDepthStencilFmt = D3DFMT_UNKNOWN;
	if (params.EnableAutoDepthStencil)
	{
		if (params.DepthStencilFormat != RHI_Format::Unknown)
		{
			outDepthStencilFmt = RHIToD3DFormat(params.DepthStencilFormat);
			if (outDepthStencilFmt == D3DFMT_UNKNOWN)
				Print("! [DX9] Unsupported depth/stencil format %d, trying auto select", (int)params.DepthStencilFormat);
		}
		if (outDepthStencilFmt == D3DFMT_UNKNOWN)
			outDepthStencilFmt = SelectDepthStencilFormat(outBackBufferFmt);

		if (outDepthStencilFmt == D3DFMT_UNKNOWN)
		{
			Print("! [DX9] No suitable depth-stencil format found");
			return false;
		}
	}

	return true;
}

void CRenderBackendDX9::FillPresentParams(const RHIPresentationParams& params,
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

	switch (params.SwapEffect)
	{
	case RHI_SwapEffect::Flip:
		m_PP.SwapEffect = D3DSWAPEFFECT_FLIPEX;
		break;
	case RHI_SwapEffect::Discard:
	default:
		if (params.SwapEffect != RHI_SwapEffect::Discard)
			Print("! [DX9] Swap effect not supported, falling back to Discard");
		m_PP.SwapEffect = D3DSWAPEFFECT_DISCARD;
		break;
	}

	m_PP.hDeviceWindow = m_hWnd;
	m_PP.Windowed = params.Windowed;

	m_PP.EnableAutoDepthStencil = params.EnableAutoDepthStencil;
	if (params.EnableAutoDepthStencil)
		m_PP.AutoDepthStencilFormat = depthStencilFmt;

	m_PP.Flags = 0;

	if (!params.Windowed)
	{
		m_PP.FullScreen_RefreshRateInHz = (fullscreenRefreshHz > 0) ? fullscreenRefreshHz : 60;
	}
	else
	{
		m_PP.FullScreen_RefreshRateInHz = 0;
	}

	m_PP.PresentationInterval = (params.SyncInterval == 0)
		? D3DPRESENT_INTERVAL_IMMEDIATE
		: D3DPRESENT_INTERVAL_ONE;
}

DWORD CRenderBackendDX9::SelectVertexProcessing()
{
	DWORD vertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	D3DCAPS9 caps;
	if (m_pD3D && SUCCEEDED(m_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps)))
	{
		if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		{
			if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE)
				vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
			else
				vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
		}
	}
	return vertexProcessing;
}

bool CRenderBackendDX9::CreateDevice(HWND hWnd, const RHIPresentationParams& params)
{
	m_hWnd = hWnd;

	HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D);
	if (FAILED(hr) || !m_pD3D)
	{
		Print("! [DX9] Direct3DCreate9Ex failed (0x%08x)", hr);
		return false;
	}

	D3DADAPTER_IDENTIFIER9 adapterID;
	m_pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterID);
	Print("* [DX9] GPU [vendor:%X]-[device:%X]: %s", adapterID.VendorId, adapterID.DeviceId, adapterID.Description);

	D3DDISPLAYMODE d3ddm;
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	m_BackBufferFmt = d3ddm.Format;
	m_DesktopRefreshRate = d3ddm.RefreshRate;

	D3DFORMAT backBufferFmt = D3DFMT_UNKNOWN;
	D3DFORMAT depthStencilFmt = D3DFMT_UNKNOWN;
	if (!DetermineDepthAndBackBufferFormatsFromPresentParams(params, backBufferFmt, depthStencilFmt))
		return false;

	m_BackBufferFmt = backBufferFmt;

	UINT refreshHz = params.FullscreenRefreshHz ? params.FullscreenRefreshHz : m_DesktopRefreshRate;
	FillPresentParams(params, backBufferFmt, depthStencilFmt, refreshHz);

	DWORD vertexProcessing = SelectVertexProcessing();

	D3DDISPLAYMODEEX ModeEx;
	D3DDISPLAYMODEEX* pModeEx = nullptr;
	if (!params.Windowed)
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

	hr = m_pD3D->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		vertexProcessing | D3DCREATE_MULTITHREADED,
		&m_PP, pModeEx, &m_pDevice);
	if (FAILED(hr))
	{
		Print("! [DX9] CreateDeviceEx failed (0x%08x), trying without MULTITHREADED", hr);
		hr = m_pD3D->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
			vertexProcessing,
			&m_PP, pModeEx, &m_pDevice);
		if (FAILED(hr))
		{
			Print("! [DX9] Second attempt failed (0x%08x)", hr);
			return false;
		}
	}

	m_pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &m_AdapterID);
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_DesktopMode);
	CacheDeviceCapsFromD3D();

	Print("* [DX9] Device created successfully: %dx%d %s, interval=%d",
		params.BackBufferWidth, params.BackBufferHeight,
		params.Windowed ? "windowed" : "fullscreen",
		params.SyncInterval);
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

void CRenderBackendDX9::ReleaseAllResources()
{
	for (size_t i = 0; i < m_Textures.size(); ++i)
	{
		DX9Texture* tex = m_Textures[i];
		if (tex)
		{
			if (tex->tex2D)   tex->tex2D->Release();
			if (tex->texCube) tex->texCube->Release();
			if (tex->surface) tex->surface->Release();
			delete tex;
			m_Textures[i] = nullptr;
		}
	}
	m_Textures.clear();
	while (!m_FreeTextureIndices.empty())
		m_FreeTextureIndices.pop();

	for (size_t i = 0; i < m_Samplers.size(); ++i)
	{
		DX9Sampler* samp = m_Samplers[i];
		if (samp)
		{
			delete samp;
			m_Samplers[i] = nullptr;
		}
	}
	m_Samplers.clear();
	while (!m_FreeSamplerIndices.empty())
		m_FreeSamplerIndices.pop();

	for (size_t i = 0; i < m_Shaders.size(); ++i) {
		DX9Shader* shader = m_Shaders[i];
		if (shader) {
			shader->Release();
			delete shader;
			m_Shaders[i] = nullptr;
		}
	}
	m_Shaders.clear();
	while (!m_FreeShaderIndices.empty()) m_FreeShaderIndices.pop();

	for (size_t i = 0; i < m_ConstantBuffers.size(); ++i) {
		DX9ConstantBuffer* cb = m_ConstantBuffers[i];
		if (cb) {
			delete cb;
			m_ConstantBuffers[i] = nullptr;
		}
	}
	m_ConstantBuffers.clear();
	while (!m_FreeConstantBufferIndices.empty()) m_FreeConstantBufferIndices.pop();
}

bool CRenderBackendDX9::Reset(const RHIPresentationParams& params)
{
	if (!m_pDevice)
		return false;

	D3DFORMAT backBufferFmt = m_BackBufferFmt;
	D3DFORMAT depthStencilFmt = D3DFMT_UNKNOWN;

	if (!DetermineDepthAndBackBufferFormatsFromPresentParams(params, backBufferFmt, depthStencilFmt))
		return false;

	UINT refreshHz = params.FullscreenRefreshHz ? params.FullscreenRefreshHz : m_DesktopRefreshRate;

	FillPresentParams(params, backBufferFmt, depthStencilFmt, refreshHz);

	HRESULT hr = m_pDevice->Reset(&m_PP);
	if (FAILED(hr))
	{
		Print("! [DX9] Reset failed (0x%08x)", hr);
		return false;
	}

	m_pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &m_AdapterID);
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_DesktopMode);
	CacheDeviceCapsFromD3D();

	for (size_t i = 0; i < m_Shaders.size(); ++i) {
		DX9Shader* shader = m_Shaders[i];
		if (!shader) continue;

		shader->Release();

		if (shader->type == ShaderType::Vertex)
			m_pDevice->CreateVertexShader(reinterpret_cast<const DWORD*>(shader->bytecode.data()), &shader->vs);
		else if (shader->type == ShaderType::Pixel)
			m_pDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(shader->bytecode.data()), &shader->ps);
	}

	Print("* [DX9] Device reset successfully: %dx%d %s, interval=%d",
		params.BackBufferWidth, params.BackBufferHeight,
		params.Windowed ? "windowed" : "fullscreen",
		params.SyncInterval);
	return true;
}

void CRenderBackendDX9::Present()
{
	if (m_pDevice)
		m_pDevice->PresentEx(nullptr, nullptr, nullptr, nullptr, 0);
}

void CRenderBackendDX9::Clear(u32 clearFlags, const fvec4 color, float depth, u8 stencil)
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

	D3DCOLOR d3dColor = D3DCOLOR_COLORVALUE(color.x, color.y, color.z, color.w);
	m_pDevice->Clear(0, nullptr, d3dFlags, d3dColor, depth, stencil);
}

void CRenderBackendDX9::GetAvailableResolutions(RHI_Format format, std::vector<std::pair<u32, u32>>& outResolutions) const
{
	outResolutions.clear();
	if (!m_pD3D) return;

	D3DFORMAT d3dFmt = RHIToD3DFormat(format);
	if (d3dFmt == D3DFMT_UNKNOWN) return;

	UINT adapter = D3DADAPTER_DEFAULT;
	UINT modeCount = m_pD3D->GetAdapterModeCount(adapter, d3dFmt);

	for (UINT i = 0; i < modeCount; ++i)
	{
		D3DDISPLAYMODE mode;
		if (FAILED(m_pD3D->EnumAdapterModes(adapter, d3dFmt, i, &mode)))
			continue;

		auto it = std::find_if(outResolutions.begin(), outResolutions.end(),
			[&](const auto& r) { return r.first == mode.Width && r.second == mode.Height; });
		if (it == outResolutions.end())
			outResolutions.emplace_back(mode.Width, mode.Height);
	}
}

RHI_Format CRenderBackendDX9::GetBackBufferFormat() const
{
	return D3DFormatToRHI(m_BackBufferFmt);
}

RHI_END
