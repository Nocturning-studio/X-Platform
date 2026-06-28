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

bool CRenderBackendDX9::DetermineDepthAndBackBufferFormatsFromPresentParams(const RHIPresentationParams& params,
	D3DFORMAT& outBackBufferFmt,
	D3DFORMAT& outDepthStencilFmt,
	bool autoFromDesktop)
{
	// --- Формат бэкбуфера ---
	if (params.BackBufferFormat != RHI_Format::Unknown)
	{
		outBackBufferFmt = RHIToD3DFormat(params.BackBufferFormat);
		if (outBackBufferFmt == D3DFMT_UNKNOWN)
		{
			Print("! [DX9] Unsupported backbuffer format %d, falling back", (int)params.BackBufferFormat);
			outBackBufferFmt = autoFromDesktop ? m_BackBufferFmt : m_BackBufferFmt; // в Reset сохраняем текущий
		}
	}
	else
	{
		// Если формат не задан – берём сохранённый (m_BackBufferFmt), он был определён при CreateDevice.
		// Для CreateDevice autoFromDesktop == true, там мы временно определим его ниже,
		// поэтому здесь полагаемся на ранее установленный m_BackBufferFmt.
		outBackBufferFmt = m_BackBufferFmt;
	}

	// --- Формат глубины/стенсила ---
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

	// SwapEffect
	switch (params.SwapEffect)
	{
	case RHI_SwapEffect::Flip:
		m_PP.SwapEffect = D3DSWAPEFFECT_FLIPEX;  // только для D3D9Ex
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

	// Fullscreen refresh
	if (!params.Windowed)
	{
		m_PP.FullScreen_RefreshRateInHz = (fullscreenRefreshHz > 0) ? fullscreenRefreshHz : 60;
	}
	else
	{
		m_PP.FullScreen_RefreshRateInHz = 0;
	}

	// Presentation interval
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

	// --- 1. D3D9Ex ---
	HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D);
	if (FAILED(hr) || !m_pD3D)
	{
		Print("! [DX9] Direct3DCreate9Ex failed (0x%08x)", hr);
		return false;
	}

	// --- 2. Информация об адаптере ---
	D3DADAPTER_IDENTIFIER9 adapterID;
	m_pD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterID);
	Print("* [DX9] GPU [vendor:%X]-[device:%X]: %s", adapterID.VendorId, adapterID.DeviceId, adapterID.Description);

	// Текущий режим дисплея (используем для автоформата)
	D3DDISPLAYMODE d3ddm;
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	m_BackBufferFmt = d3ddm.Format;            // сохраняем как "родной" формат рабочего стола
	m_DesktopRefreshRate = d3ddm.RefreshRate;  // сохраняем частоту

	// --- 3. Определяем форматы (авто-определение разрешено) ---
	D3DFORMAT backBufferFmt = D3DFMT_UNKNOWN;
	D3DFORMAT depthStencilFmt = D3DFMT_UNKNOWN;
	if (!DetermineDepthAndBackBufferFormatsFromPresentParams(params, backBufferFmt, depthStencilFmt, true))
		return false;

	m_BackBufferFmt = backBufferFmt; // обновляем на случай, если формат был задан явно

	// --- 4. Заполняем Presentation Parameters ---
	UINT refreshHz = params.FullscreenRefreshHz ? params.FullscreenRefreshHz : m_DesktopRefreshRate;
	FillPresentParams(params, backBufferFmt, depthStencilFmt, refreshHz);

	// --- 5. Vertex processing ---
	DWORD vertexProcessing = SelectVertexProcessing();

	// --- 6. Display Mode Ex (полноэкранный) ---
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

	// --- 7. Создание устройства ---
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

	// --- 8. Кэшируем Caps ---
	m_pDevice->GetDeviceCaps(&m_Caps);

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

bool CRenderBackendDX9::Reset(const RHIPresentationParams& params)
{
	if (!m_pDevice)
		return false;

	// Определяем форматы без авто-определения с рабочего стола (autoFromDesktop = false)
	D3DFORMAT backBufferFmt = m_BackBufferFmt;
	D3DFORMAT depthStencilFmt = D3DFMT_UNKNOWN;

	if (!DetermineDepthAndBackBufferFormatsFromPresentParams(params, backBufferFmt, depthStencilFmt, false))
		return false;

	// Частота обновления: приоритет у params, иначе сохранённая с рабочего стола
	UINT refreshHz = params.FullscreenRefreshHz ? params.FullscreenRefreshHz : m_DesktopRefreshRate;

	FillPresentParams(params, backBufferFmt, depthStencilFmt, refreshHz);

	HRESULT hr = m_pDevice->Reset(&m_PP);
	if (FAILED(hr))
	{
		Print("! [DX9] Reset failed (0x%08x)", hr);
		return false;
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

void CRenderBackendDX9::GetDeviceCaps(void* pCaps)
{
	if (pCaps)
		*(D3DCAPS9*)pCaps = m_Caps;
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

static size_t GetPixelSize(RHI_Format fmt)
{
    switch (fmt)
    {
    case RHI_Format::RGBA8_UNORM:   return 4;
    case RHI_Format::A8_UNORM:      return 1;
    case RHI_Format::R8_UNORM:      return 1;
    case RHI_Format::RGBA16_FLOAT:  return 8;  // 4 канала * 2 байта
    case RHI_Format::RG16_FLOAT:    return 4;  // 2 канала * 2 байта
    case RHI_Format::R16_FLOAT:     return 2;
    // Для глубинных форматов размер не нужен, т.к. initialData не передаётся
    default: return 0;
    }
}

RHITexture CRenderBackendDX9::CreateTexture(const TextureDesc& desc, const void* initialData)
{
	// Проверка корректности параметров
	if (desc.width == 0 || desc.height == 0)
	{
		Print("! [DX9] CreateTexture: invalid dimensions (%ux%u)", desc.width, desc.height);
		return nullptr;
	}

	D3DFORMAT d3dFmt = RHIToD3DFormat(desc.format);
	if (d3dFmt == D3DFMT_UNKNOWN)
	{
		Print("! [DX9] CreateTexture: unsupported format %d", (int)desc.format);
		return nullptr;
	}

	DWORD usage = 0;
	if (desc.isRenderTarget)
		usage |= D3DUSAGE_RENDERTARGET;
	if (desc.isDepthStencil)
		usage |= D3DUSAGE_DEPTHSTENCIL;

	// Всегда используем DEFAULT пул для всех текстур (рекомендуется для D3D9Ex)
	D3DPOOL pool = D3DPOOL_DEFAULT;

	IDirect3DTexture9* tex = nullptr;
	HRESULT hr = m_pDevice->CreateTexture(desc.width, desc.height, desc.mipLevels, usage, d3dFmt, pool, &tex, nullptr);
	if (FAILED(hr))
	{
		Print("! [DX9] CreateTexture failed (%s) for format %d", WinErrorToString(hr).c_str(), (int)desc.format);
		return nullptr;
	}

	RHITextureImpl* impl = new RHITextureImpl;
	impl->tex2D = tex;
	impl->texCube = nullptr;
	impl->surface = nullptr;
	impl->format = desc.format;
	impl->width = desc.width;
	impl->height = desc.height;
	impl->isRenderTarget = desc.isRenderTarget;
	impl->isDepthStencil = desc.isDepthStencil;

	// Загрузка начальных данных для обычных текстур (не рендер-таргет и не depth-stencil)
	if (initialData && !desc.isRenderTarget && !desc.isDepthStencil)
	{
		size_t pixelSize = GetPixelSize(desc.format);
		if (pixelSize > 0)
		{
			// Создаём временную текстуру в SYSTEMMEM пуле для загрузки данных
			IDirect3DTexture9* sysMemTex = nullptr;
			hr =
				m_pDevice->CreateTexture(desc.width, desc.height, 1, 0, d3dFmt, D3DPOOL_SYSTEMMEM, &sysMemTex, nullptr);
			if (FAILED(hr))
			{
				Print("! [DX9] Failed to create system memory texture for initial data");
			}
			else
			{
				// Заполняем системную текстуру
				D3DLOCKED_RECT locked;
				if (SUCCEEDED(sysMemTex->LockRect(0, &locked, nullptr, 0)))
				{
					const BYTE* src = (const BYTE*)initialData;
					BYTE* dst = (BYTE*)locked.pBits;
					size_t rowSize = desc.width * pixelSize;
					for (u32 y = 0; y < desc.height; ++y)
					{
						memcpy(dst, src, rowSize);
						src += rowSize;
						dst += locked.Pitch;
					}
					sysMemTex->UnlockRect(0);

					// Копируем из системной памяти в видеопамять (DEFAULT)
					hr = m_pDevice->UpdateTexture(sysMemTex, tex);
					if (FAILED(hr))
					{
						Print("! [DX9] UpdateTexture failed (0x%08x)", hr);
					}
				}
				else
				{
					Print("! [DX9] Failed to lock system memory texture");
				}
				sysMemTex->Release();
			}
		}
		else
		{
			Print("! [DX9] Cannot determine pixel size for format %d", (int)desc.format);
		}
	}

	return impl;
}

void CRenderBackendDX9::DestroyTexture(RHITexture texture)
{
	if (!texture)
		return;
	RHITextureImpl* impl = (RHITextureImpl*)texture;
	if (impl->tex2D)
		impl->tex2D->Release();
	if (impl->texCube)
		impl->texCube->Release();
	if (impl->surface)
		impl->surface->Release();
	delete impl;
}

RHISampler CRenderBackendDX9::CreateSampler(const SamplerDesc& desc)
{
	RHISamplerImpl* impl = new RHISamplerImpl;
	impl->desc = desc;
	return impl;
}

void CRenderBackendDX9::DestroySampler(RHISampler sampler)
{
	if (!sampler)
		return;
	RHISamplerImpl* impl = (RHISamplerImpl*)sampler;
	delete impl;
}

void CRenderBackendDX9::ApplySampler(u32 slot, RHISampler sampler)
{
	if (!sampler)
		return;

	RHISamplerImpl* impl = (RHISamplerImpl*)sampler;
	const SamplerDesc& d = impl->desc;

	// Фильтры
	D3DTEXTUREFILTERTYPE minFilter = D3DTEXF_POINT;
	D3DTEXTUREFILTERTYPE magFilter = D3DTEXF_POINT;
	D3DTEXTUREFILTERTYPE mipFilter = D3DTEXF_POINT;

	auto convertFilter = [](RHI_Filter f) -> D3DTEXTUREFILTERTYPE {
		switch (f)
		{
		case RHI_Filter::Point:
			return D3DTEXF_POINT;
		case RHI_Filter::Linear:
			return D3DTEXF_LINEAR;
		case RHI_Filter::Anisotropic:
			return D3DTEXF_ANISOTROPIC;
		default:
			return D3DTEXF_POINT;
		}
	};

	minFilter = convertFilter(d.minFilter);
	magFilter = convertFilter(d.magFilter);
	mipFilter = (d.mipFilter == RHI_Filter::None) ? D3DTEXF_NONE : convertFilter(d.mipFilter);

	m_pDevice->SetSamplerState(slot, D3DSAMP_MINFILTER, minFilter);
	m_pDevice->SetSamplerState(slot, D3DSAMP_MAGFILTER, magFilter);
	m_pDevice->SetSamplerState(slot, D3DSAMP_MIPFILTER, mipFilter);

	// Адресация
	auto convertAddress = [](RHI_TextureAddress addr) -> D3DTEXTUREADDRESS {
		switch (addr)
		{
		case RHI_TextureAddress::Wrap:
			return D3DTADDRESS_WRAP;
		case RHI_TextureAddress::Mirror:
			return D3DTADDRESS_MIRROR;
		case RHI_TextureAddress::Clamp:
			return D3DTADDRESS_CLAMP;
		case RHI_TextureAddress::Border:
			return D3DTADDRESS_BORDER;
		case RHI_TextureAddress::MirrorOnce:
			return D3DTADDRESS_MIRRORONCE;
		default:
			return D3DTADDRESS_WRAP;
		}
	};

	m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSU, convertAddress(d.addressU));
	m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSV, convertAddress(d.addressV));
	m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSW, convertAddress(d.addressW));

	// Анизотропия
	m_pDevice->SetSamplerState(slot, D3DSAMP_MAXANISOTROPY, d.maxAnisotropy);

	// MIP LOD bias
	m_pDevice->SetSamplerState(slot, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)(&d.mipLODBias)));

	// Border color
	if (d.addressU == RHI_TextureAddress::Border || d.addressV == RHI_TextureAddress::Border ||
		d.addressW == RHI_TextureAddress::Border)
	{
		D3DCOLOR borderColor = D3DCOLOR_COLORVALUE(d.borderColor.x, d.borderColor.y, d.borderColor.z, d.borderColor.w);
		m_pDevice->SetSamplerState(slot, D3DSAMP_BORDERCOLOR, borderColor);
	}
}

void CRenderBackendDX9::SetTexture(u32 slot, RHITexture texture, RHISampler sampler)
{
	if (!m_pDevice)
		return;

	// Применяем семплер (если есть)
	if (sampler)
		ApplySampler(slot, sampler);
	else
	{
		// Сброс на дефолтные значения (можно установить трилинейную фильтрацию)
		m_pDevice->SetSamplerState(slot, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_pDevice->SetSamplerState(slot, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_pDevice->SetSamplerState(slot, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		m_pDevice->SetSamplerState(slot, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
		m_pDevice->SetSamplerState(slot, D3DSAMP_MAXANISOTROPY, 1);
		m_pDevice->SetSamplerState(slot, D3DSAMP_MIPMAPLODBIAS, 0);
	}

	// Устанавливаем текстуру
	IDirect3DBaseTexture9* d3dTex = nullptr;
	if (texture)
	{
		RHITextureImpl* impl = (RHITextureImpl*)texture;
		d3dTex = impl->tex2D; // пока только 2D
	}
	m_pDevice->SetTexture(slot, d3dTex);
}

RHI_END
