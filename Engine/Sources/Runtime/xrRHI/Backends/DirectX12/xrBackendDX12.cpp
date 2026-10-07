////////////////////////////////////////////////////////////////////////////////
// Created: 07.10.2026 19:53:07
// Author: NS_Deathman
// File: xrBackendDX12.cpp
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "xrBackendDX12.h"
////////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
////////////////////////////////////////////////////////////////////////////////
namespace
{
// ============================================================================
// Format mapping
// ============================================================================
//
// Маппинг RHI_Format -> DXGI_FORMAT для тех форматов, которые нужны на этом
// этапе (back buffer). Полный маппинг добавим вместе с CreateTexture.
//
// Замечание про RGBA8_UNORM: в D3D9-бэкенде этот формат маппится в
// D3DFMT_A8R8G8B8 (в памяти — BGRA). Чтобы сохранить поведение между
// бэкендами, используем B8G8R8A8_UNORM (это и предпочтительный формат
// для flip-model swap chain в DX12).
//
DXGI_FORMAT RHIToDXGIFormat(RHI_Format fmt)
{
	switch (fmt)
	{
	case RHI_Format::RGBA8_UNORM:       return DXGI_FORMAT_B8G8R8A8_UNORM;
	case RHI_Format::RGBA8_UNORM_SRGB:  return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	case RHI_Format::R10G10B10A2_UNORM: return DXGI_FORMAT_R10G10B10A2_UNORM;
	case RHI_Format::RGBA16_FLOAT:      return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case RHI_Format::R16G16B16A16_UNORM:return DXGI_FORMAT_R16G16B16A16_UNORM;
	case RHI_Format::R32G32B32A32_FLOAT:return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case RHI_Format::R32_FLOAT:         return DXGI_FORMAT_R32_FLOAT;
	case RHI_Format::D16_UNORM:         return DXGI_FORMAT_D16_UNORM;
	case RHI_Format::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case RHI_Format::D32_FLOAT:         return DXGI_FORMAT_D32_FLOAT;
	case RHI_Format::BC1_UNORM:         return DXGI_FORMAT_BC1_UNORM;
	case RHI_Format::BC3_UNORM:         return DXGI_FORMAT_BC3_UNORM;
	case RHI_Format::BC5_UNORM:         return DXGI_FORMAT_BC5_UNORM;
	default:                            return DXGI_FORMAT_UNKNOWN;
	}
}

RHI_Format DXGIFormatToRHI(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_B8G8R8A8_UNORM:       return RHI_Format::RGBA8_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:  return RHI_Format::RGBA8_UNORM_SRGB;
	case DXGI_FORMAT_R8G8B8A8_UNORM:       return RHI_Format::RGBA8_UNORM;
	case DXGI_FORMAT_R10G10B10A2_UNORM:    return RHI_Format::R10G10B10A2_UNORM;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:   return RHI_Format::RGBA16_FLOAT;
	case DXGI_FORMAT_R16G16B16A16_UNORM:   return RHI_Format::R16G16B16A16_UNORM;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:   return RHI_Format::R32G32B32A32_FLOAT;
	case DXGI_FORMAT_R32_FLOAT:            return RHI_Format::R32_FLOAT;
	case DXGI_FORMAT_D16_UNORM:            return RHI_Format::D16_UNORM;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:    return RHI_Format::D24_UNORM_S8_UINT;
	case DXGI_FORMAT_D32_FLOAT:            return RHI_Format::D32_FLOAT;
	default:                               return RHI_Format::Unknown;
	}
}

// ============================================================================
// Debug layer
// ============================================================================
void EnableD3D12DebugLayer()
{
#if defined(_DEBUG) || defined(DX12_ENABLE_DEBUG_LAYER)
	ID3D12Debug* dbg = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg))) && dbg)
	{
		dbg->EnableDebugLayer();

		// GPU-based validation — очень полезно на старте, но жрёт fps.
		// Включаем только в самых ранних сборках.
#if defined(DX12_ENABLE_GPU_VALIDATION)
		ID3D12Debug1* dbg1 = nullptr;
		if (SUCCEEDED(dbg->QueryInterface(IID_PPV_ARGS(&dbg1))) && dbg1)
		{
			dbg1->SetEnableGPUBasedValidation(TRUE);
			dbg1->Release();
		}
#endif
		dbg->Release();
	}
	else
	{
		Msg("! [DX12] Debug layer unavailable — install Graphics Tools");
	}
#endif
}

const char* HrStr(HRESULT hr)
{
	// Простой однострочный форматтер. Полноценный FormatMessage не тащим.
	static thread_local char buf[32];
	sprintf_s(buf, "0x%08X", (unsigned)hr);
	return buf;
}

} // namespace
////////////////////////////////////////////////////////////////////////////////

CRenderBackendDX12::CRenderBackendDX12()  = default;
CRenderBackendDX12::~CRenderBackendDX12()
{
	DestroyDevice();
}

// ============================================================================
// CreateDevice
// ============================================================================

bool CRenderBackendDX12::CreateDevice(HWND hWnd, const RHI_PresentationParams& params)
{
	if (m_pDevice)
	{
		Msg("! [DX12] CreateDevice: device already exists");
		return false;
	}
	if (!hWnd)
	{
		Msg("! [DX12] CreateDevice: hWnd is null");
		return false;
	}

	m_hWnd           = hWnd;
	m_windowed       = params.Windowed;
	m_syncInterval   = params.SyncInterval;
	m_backBufferWidth  = params.BackBufferWidth;
	m_backBufferHeight = params.BackBufferHeight;

	const bool enableDebug =
#if defined(_DEBUG) || defined(DX12_ENABLE_DEBUG_LAYER)
		true;
#else
		false;
#endif
	EnableD3D12DebugLayer();

	if (!CreateFactory(enableDebug))
		return false;

	IDXGIAdapter4* adapter = nullptr;
	if (!SelectAdapter(&adapter))
		return false;
	m_pAdapter = adapter;

	if (!CreateD3D12Device(adapter, enableDebug))
		return false;

	if (!CreateCommandQueue())
		return false;

	if (!CreateSwapChain(hWnd, params))
		return false;

	if (!CreateRTVHeapAndBackBuffers())
		return false;

	if (!CreateFrameSyncObjects())
		return false;

	CacheDeviceCaps();

	Msg("* [DX12] Device created: %ux%u %s, %u back buffers, interval=%u", m_backBufferWidth, 
																		   m_backBufferHeight,
																		   m_windowed ? "windowed" : "fullscreen",
																		   m_backBufferCount, 
																		   m_syncInterval);

	return true;
}

// ============================================================================
// CreateFactory / SelectAdapter / CreateDevice / CreateCommandQueue
// ============================================================================

bool CRenderBackendDX12::CreateFactory(bool enableDebug)
{
	UINT flags = 0;
	if (enableDebug)
		flags |= DXGI_CREATE_FACTORY_DEBUG;

	HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_pFactory));
	if (FAILED(hr) || !m_pFactory)
	{
		Msg("! [DX12] CreateDXGIFactory2 failed (%s)", HrStr(hr));

		// Fallback — без debug-флага (иногда debug-слой не установлен).
		hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_pFactory));
		if (FAILED(hr) || !m_pFactory)
		{
			Msg("! [DX12] CreateDXGIFactory2 fallback failed (%s)", HrStr(hr));
			return false;
		}
	}
	return true;
}

bool CRenderBackendDX12::SelectAdapter(IDXGIAdapter4** outAdapter)
{
	*outAdapter = nullptr;

	// Пытаемся получить "high performance" адаптер (дискретную карту).
	IDXGIAdapter4* adapter = nullptr;
	HRESULT hr = m_pFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));

	if (FAILED(hr) || !adapter)
	{
		// Fallback на стандартный EnumAdapters.
		IDXGIAdapter* a0 = nullptr;
		hr = m_pFactory->EnumAdapters(0, &a0);
		if (FAILED(hr) || !a0)
		{
			Msg("! [DX12] No adapters found (%s)", HrStr(hr));
			return false;
		}
		a0->QueryInterface(IID_PPV_ARGS(&adapter));
		a0->Release();
	}

	if (!adapter)
	{
		Msg("! [DX12] Failed to acquire IDXGIAdapter4");
		return false;
	}

	DXGI_ADAPTER_DESC3 desc{};
	if (SUCCEEDED(adapter->GetDesc3(&desc)))
	{
		Msg("* [DX12] Adapter [vendor:%04X]-[device:%04X]: %ls",
			desc.VendorId, desc.DeviceId, desc.Description);
	}
	else
	{
		Msg("* [DX12] Adapter selected (no Desc3 available)");
	}

	*outAdapter = adapter;
	return true;
}

bool CRenderBackendDX12::CreateD3D12Device(IDXGIAdapter4* adapter, bool enableDebug)
{
	(void)enableDebug;

	HRESULT hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice));
	if (FAILED(hr) || !m_pDevice)
	{
		Msg("! [DX12] D3D12CreateDevice failed (%s)", HrStr(hr));
		return false;
	}

	// Debug name для device — удобно видеть в PIX / RenderDoc.
	if (m_pDevice && enableDebug)
		m_pDevice->SetName(L"xrRHI::DX12::Device");

	return true;
}

bool CRenderBackendDX12::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC qd{};
	qd.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
	qd.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	qd.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
	qd.NodeMask = 0;

	HRESULT hr = m_pDevice->CreateCommandQueue(&qd, IID_PPV_ARGS(&m_pCommandQueue));
	if (FAILED(hr) || !m_pCommandQueue)
	{
		Msg("! [DX12] CreateCommandQueue failed (%s)", HrStr(hr));
		return false;
	}
	return true;
}

// ============================================================================
// CreateSwapChain
// ============================================================================

bool CRenderBackendDX12::CreateSwapChain(HWND hWnd, const RHI_PresentationParams& params)
{
	// Определяем формат бэкбуфера.
	m_backBufferFormat = RHIToDXGIFormat(params.BackBufferFormat);
	if (m_backBufferFormat == DXGI_FORMAT_UNKNOWN)
	{
		Msg("! [DX12] Unsupported backbuffer format (%u), falling back to B8G8R8A8_UNORM", (unsigned)params.BackBufferFormat);
		m_backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	}

	// Размеры бэкбуфера.
	u32 width  = params.BackBufferWidth  ? params.BackBufferWidth  : 0;
	u32 height = params.BackBufferHeight ? params.BackBufferHeight : 0;

	if (width == 0 || height == 0)
	{
		RECT rc{};
		GetClientRect(hWnd, &rc);
		width  = (u32)(rc.right - rc.left);
		height = (u32)(rc.bottom - rc.top);
	}
	if (width == 0 || height == 0)
	{
		Msg("! [DX12] CreateSwapChain: zero size client area (%ux%u)", width, height);
		return false;
	}

	// BufferCount: FLIP-model требует >= 2. Ограничиваем 3.
	uint32_t bufferCount = params.BackBufferCount;
	if (bufferCount < 2) bufferCount = 2;
	if (bufferCount > kMaxBackBuffers) bufferCount = kMaxBackBuffers;

	DXGI_SWAP_CHAIN_DESC1 scd{};
	scd.Width              = width;
	scd.Height             = height;
	scd.Format             = m_backBufferFormat;
	scd.Stereo             = FALSE;
	scd.SampleDesc.Count   = 1;
	scd.SampleDesc.Quality = 0;
	scd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount        = bufferCount;
	scd.Scaling            = DXGI_SCALING_STRETCH;
	scd.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scd.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
	scd.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	IDXGISwapChain1* sc1 = nullptr;
	HRESULT hr = m_pFactory->CreateSwapChainForHwnd(m_pCommandQueue,           // команда-очередь, а не device!
													hWnd,
													&scd,
													nullptr,                   // fullscreen desc
													nullptr,                   // output
													&sc1);

	if (FAILED(hr) || !sc1)
	{
		Msg("! [DX12] CreateSwapChainForHwnd failed (%s)", HrStr(hr));
		return false;
	}

	hr = sc1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
	sc1->Release();
	if (FAILED(hr) || !m_pSwapChain)
	{
		Msg("! [DX12] QI for IDXGISwapChain4 failed (%s)", HrStr(hr));
		return false;
	}

	// Отключаем Alt+Enter-обработчик DXGI — переключение режимов делаем сами.
	m_pFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

	m_backBufferCount  = bufferCount;
	m_backBufferWidth  = width;
	m_backBufferHeight = height;
	m_frameIndex       = m_pSwapChain->GetCurrentBackBufferIndex();

	Msg("* [DX12] Swap chain created: %ux%u, fmt=%u, buffers=%u, flip-discard",
		width, height, (unsigned)m_backBufferFormat, bufferCount);

	return true;
}

// ============================================================================
// RTV heap + back buffers
// ============================================================================

bool CRenderBackendDX12::CreateRTVHeapAndBackBuffers()
{
	// RTV heap — non-shader-visible.
	D3D12_DESCRIPTOR_HEAP_DESC hd{};
	hd.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	hd.NumDescriptors = kMaxBackBuffers;
	hd.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hd.NodeMask       = 0;

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&m_rtvHeap));
	if (FAILED(hr) || !m_rtvHeap)
	{
		Msg("! [DX12] CreateDescriptorHeap(RTV) failed (%s)", HrStr(hr));
		return false;
	}

	m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE handle =
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	// Back buffers.
	//
	// ВАЖНО: swap chain держит ref на back buffer'ы; мы не делаем AddRef
	// сами (GetBuffer возвращает +1 ref, который мы должны Release при
	// пересоздании). Release — в ReleaseSwapChainResources().
	//
	for (uint32_t i = 0; i < m_backBufferCount; ++i)
	{
		hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
		if (FAILED(hr) || !m_backBuffers[i])
		{
			Msg("! [DX12] SwapChain::GetBuffer(%u) failed (%s)", i, HrStr(hr));
			return false;
		}

		m_backBufferRTVHandles[i] = handle;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format        = m_backBufferFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		// mipSlice / planeSlice = 0 по умолчанию.
		m_pDevice->CreateRenderTargetView(m_backBuffers[i], &rtvDesc, handle);

		handle.ptr += m_rtvDescriptorSize;
	}

	// Публичный хэндл back-buffer RTV. Пока просто id=0 — реальная таблица
	// слотов появится вместе с CreateRTV()/DestroyRTV().
	m_backBufferRTV = RHI_RenderTargetView{ 0 };

	return true;
}

// ============================================================================
// Frame sync objects
// ============================================================================

bool CRenderBackendDX12::CreateFrameSyncObjects()
{
	HRESULT hr;
	for (uint32_t i = 0; i < kFrameCount; ++i)
	{
		hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]));
		if (FAILED(hr) || !m_commandAllocators[i])
		{
			Msg("! [DX12] CreateCommandAllocator(%u) failed (%s)", i, HrStr(hr));
			return false;
		}

		m_fenceValues[i] = 0;
	}

	// Одна command list на весь бэкенд — этого достаточно для минимального
	// скелета. Позже заменим на пул.
	hr = m_pDevice->CreateCommandList(0,
									  D3D12_COMMAND_LIST_TYPE_DIRECT,
									  m_commandAllocators[m_frameIndex],
									  nullptr,
									  IID_PPV_ARGS(&m_commandList));
	if (FAILED(hr) || !m_commandList)
	{
		Msg("! [DX12] CreateCommandList failed (%s)", HrStr(hr));
		return false;
	}

	// Command list создаётся открытым — закрываем сразу.
	m_commandList->Close();

	hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(hr) || !m_fence)
	{
		Msg("! [DX12] CreateFence failed (%s)", HrStr(hr));
		return false;
	}

	m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (!m_fenceEvent)
	{
		Msg("! [DX12] CreateEvent failed (%u)", GetLastError());
		return false;
	}

	return true;
}

// ============================================================================
// Frame loop
// ============================================================================

void CRenderBackendDX12::OnFrameBegin()
{
	if (!m_pDevice || m_inFrame)
		return;

	// Reset allocator + list для текущего кадра.
	HRESULT hr = m_commandAllocators[m_frameIndex]->Reset();
	if (FAILED(hr))
	{
		Msg("! [DX12] CommandAllocator::Reset failed (%s)", HrStr(hr));
		return;
	}

	hr = m_commandList->Reset(m_commandAllocators[m_frameIndex], nullptr);
	if (FAILED(hr))
	{
		Msg("! [DX12] CommandList::Reset failed (%s)", HrStr(hr));
		return;
	}

	m_inFrame = true;
}

void CRenderBackendDX12::OnFrameEnd()
{
	if (!m_pDevice || !m_inFrame)
		return;

	HRESULT hr = m_commandList->Close();
	if (FAILED(hr))
	{
		Msg("! [DX12] CommandList::Close failed (%s)", HrStr(hr));
		m_inFrame = false;
		return;
	}

	ID3D12CommandList* lists[] = { m_commandList };
	m_pCommandQueue->ExecuteCommandLists(1, lists);

	m_inFrame = false;
}

void CRenderBackendDX12::Present()
{
	if (!m_pSwapChain || !m_pCommandQueue)
		return;

	HRESULT hr = m_pSwapChain->Present(m_syncInterval, 0);
	if (FAILED(hr))
	{
		Msg("! [DX12] Present failed (%s)", HrStr(hr));
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// Устройство потеряно — CheckDeviceStatus вернёт Lost.
		}
		return;
	}

	MoveToNextFrame();
}

// ============================================================================
// Frame throttling
// ============================================================================

void CRenderBackendDX12::WaitForFrame(uint32_t frameIdx)
{
	if (m_fence->GetCompletedValue() < m_fenceValues[frameIdx])
	{
		m_fence->SetEventOnCompletion(m_fenceValues[frameIdx], m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void CRenderBackendDX12::MoveToNextFrame()
{
	// Сигналим текущий кадр и двигаемся к следующему.
	const UINT64 currentValue = ++m_fenceValues[m_frameIndex];
	m_pCommandQueue->Signal(m_fence, currentValue);

	m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	WaitForFrame(m_frameIndex);
}

void CRenderBackendDX12::FlushGpu()
{
	if (!m_pCommandQueue || !m_fence) return;

	const UINT64 value = ++m_fenceValues[m_frameIndex];
	m_pCommandQueue->Signal(m_fence, value);

	if (m_fence->GetCompletedValue() < value)
	{
		m_fence->SetEventOnCompletion(value, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

// ============================================================================
// Device status
// ============================================================================

RHI_DeviceStatus CRenderBackendDX12::CheckDeviceStatus() const
{
	if (!m_pDevice)
		return RHI_DeviceStatus::Lost;

	const HRESULT hr = m_pDevice->GetDeviceRemovedReason();
	if (FAILED(hr))
	{
		Msg("! [DX12] Device removed (reason %s)", HrStr(hr));
		return RHI_DeviceStatus::Lost;
	}
	return RHI_DeviceStatus::OK;
}

// ============================================================================
// Reset (ResizeBuffers-подобный сценарий)
// ============================================================================
//
// В DX12 нет аналога D3D9 Reset(). Практический эквивалент — пересоздание
// swap chain ресурсов (back buffers + RTV). Делаем только то, что нужно
// для смены размера/формата. Смена windowed<->fullscreen через Reset()
// в этой версии не поддерживается — нужно Destroy/CreateDevice.
//
bool CRenderBackendDX12::Reset(const RHI_PresentationParams& params)
{
	if (!m_pDevice || !m_pSwapChain)
	{
		Msg("! [DX12] Reset: no device/swapchain");
		return false;
	}

	const DXGI_FORMAT newFmt = RHIToDXGIFormat(params.BackBufferFormat);
	const bool fmtChanged =
		(newFmt != DXGI_FORMAT_UNKNOWN) && (newFmt != m_backBufferFormat);

	const bool sizeChanged =
		(params.BackBufferWidth  != m_backBufferWidth) ||
		(params.BackBufferHeight != m_backBufferHeight);

	if (!fmtChanged && !sizeChanged)
	{
		// Ничего существенного не поменялось.
		m_syncInterval = params.SyncInterval;
		m_windowed     = params.Windowed;
		return true;
	}

	// Ждём завершения всех GPU-работ.
	FlushGpu();

	// Освобождаем back buffers и RTV-heap.
	ReleaseSwapChainResources();

	if (fmtChanged)
		m_backBufferFormat = newFmt;

	u32 width  = params.BackBufferWidth  ? params.BackBufferWidth  : m_backBufferWidth;
	u32 height = params.BackBufferHeight ? params.BackBufferHeight : m_backBufferHeight;

	const UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hr = m_pSwapChain->ResizeBuffers(
		0,                       // 0 = сохранить текущее число буферов
		width,
		height,
		m_backBufferFormat,
		flags);
	if (FAILED(hr))
	{
		Msg("! [DX12] ResizeBuffers failed (%s)", HrStr(hr));
		return false;
	}

	m_backBufferWidth  = width;
	m_backBufferHeight = height;
	m_syncInterval     = params.SyncInterval;
	m_windowed         = params.Windowed;
	m_frameIndex       = m_pSwapChain->GetCurrentBackBufferIndex();

	if (!CreateRTVHeapAndBackBuffers())
	{
		Msg("! [DX12] Reset: failed to recreate back buffers/RTVs");
		return false;
	}

	// Сбрасываем fence-историю, чтобы throttle не ждал старых значений.
	for (uint32_t i = 0; i < kFrameCount; ++i)
		m_fenceValues[i] = 0;

	Msg("* [DX12] Swap chain reset: %ux%u, fmt=%u", width, height,
		(unsigned)m_backBufferFormat);
	return true;
}

// ============================================================================
// Caps / presentation info
// ============================================================================

void CRenderBackendDX12::CacheDeviceCaps()
{
	m_DeviceCaps = {};

	if (m_pAdapter)
	{
		DXGI_ADAPTER_DESC3 desc{};
		if (SUCCEEDED(m_pAdapter->GetDesc3(&desc)))
		{
			m_DeviceCaps.VendorId    = desc.VendorId;
			m_DeviceCaps.DeviceId    = desc.DeviceId;
			// WideCharToMultiByte — самый простой путь.
			char buf[256] = {};
			WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1,
				buf, sizeof(buf) - 1, nullptr, nullptr);
			m_DeviceCaps.Description = buf;
		}

		// Текущий режим output'а (для полноэкранного — он же display mode).
		IDXGIOutput* out = nullptr;
		if (SUCCEEDED(m_pAdapter->EnumOutputs(0, &out)) && out)
		{
			DXGI_OUTPUT_DESC od{};
			if (SUCCEEDED(out->GetDesc(&od)))
			{
				m_DeviceCaps.DisplayWidth  = od.DesktopCoordinates.right  - od.DesktopCoordinates.left;
				m_DeviceCaps.DisplayHeight = od.DesktopCoordinates.bottom - od.DesktopCoordinates.top;
			}
			out->Release();
		}
	}

	// Общие аппаратные лимиты (DX12 не требует "caps", как D3D9, но
	// для совместимости с RHIDeviceCaps заполним разумными значениями).
	m_DeviceCaps.DisplayFormat          = GetBackBufferFormat();
	m_DeviceCaps.DisplayRefreshRate     = 60;    // TODO: из DXGI_MODE_DESC
	m_DeviceCaps.MaxTextureWidth        = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	m_DeviceCaps.MaxTextureHeight       = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	m_DeviceCaps.MaxVolumeExtent        = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
	m_DeviceCaps.MaxSimultaneousRTs     = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
	m_DeviceCaps.HasVertexShader        = true;
	m_DeviceCaps.HasPixelShader         = true;
	m_DeviceCaps.VertexShaderMajor      = 6;
	m_DeviceCaps.VertexShaderMinor      = 0;
	m_DeviceCaps.PixelShaderMajor       = 6;
	m_DeviceCaps.PixelShaderMinor       = 0;
	m_DeviceCaps.MaxVertexShaderConst   = 4096; // D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT
	m_DeviceCaps.HasDepthStencil        = true;
	m_DeviceCaps.MaxAnisotropy          = 16;
	m_DeviceCaps.MaxTextureBlendStages  = 16;
	m_DeviceCaps.MaxSimultaneousTextures= 128;
	m_DeviceCaps.HardwareTnL            = true;
	m_DeviceCaps.SupportsPureDevice     = true;
	m_DeviceCaps.SupportsNonPow2Textures= true;
	m_DeviceCaps.VertexCacheMethod      = 0;
	m_DeviceCaps.VertexCacheSize        = 16;
}

RHI_Format CRenderBackendDX12::GetBackBufferFormat() const
{
	return DXGIFormatToRHI(m_backBufferFormat);
}

void CRenderBackendDX12::GetAvailableResolutions(RHI_Format /*format*/,
	std::vector<std::pair<uint32_t, uint32_t>>& outResolutions) const
{
	// TODO: пройтись по IDXGIOutput::GetDisplayModeList.
	outResolutions.clear();
}

// ============================================================================
// DestroyDevice / release helpers
// ============================================================================

void CRenderBackendDX12::ReleaseSwapChainResources()
{
	for (uint32_t i = 0; i < kMaxBackBuffers; ++i)
	{
		if (m_backBuffers[i])
		{
			m_backBuffers[i]->Release();
			m_backBuffers[i] = nullptr;
		}
	}

	if (m_rtvHeap)
	{
		m_rtvHeap->Release();
		m_rtvHeap = nullptr;
	}

	m_backBufferCount = 0;
	m_rtvDescriptorSize = 0;
	m_backBufferRTV = {};
}

void CRenderBackendDX12::ReleaseFrameResources()
{
	for (uint32_t i = 0; i < kFrameCount; ++i)
	{
		if (m_commandAllocators[i])
		{
			m_commandAllocators[i]->Release();
			m_commandAllocators[i] = nullptr;
		}
		m_fenceValues[i] = 0;
	}

	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = nullptr;
	}

	if (m_fence)
	{
		m_fence->Release();
		m_fence = nullptr;
	}

	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
}

void CRenderBackendDX12::DestroyDevice()
{
	if (m_pDevice) FlushGpu();

	ReleaseSwapChainResources();
	ReleaseFrameResources();

	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
		m_pSwapChain = nullptr;
	}
	if (m_pCommandQueue)
	{
		m_pCommandQueue->Release();
		m_pCommandQueue = nullptr;
	}
	if (m_pDevice)
	{
		m_pDevice->Release();
		m_pDevice = nullptr;
	}
	if (m_pAdapter)
	{
		m_pAdapter->Release();
		m_pAdapter = nullptr;
	}
	if (m_pFactory)
	{
		m_pFactory->Release();
		m_pFactory = nullptr;
	}

	m_hWnd = nullptr;
	m_backBufferWidth  = 0;
	m_backBufferHeight = 0;
	m_backBufferFormat = DXGI_FORMAT_UNKNOWN;
	m_frameIndex       = 0;
	m_inFrame          = false;
}

// ============================================================================
// STUBS
// ============================================================================
//
// Всё ниже — временные реализации, чтобы класс линковался и движок мог
// пройти цикл CreateDevice -> OnFrameBegin -> Clear -> OnFrameEnd -> Present.
// Реальная работа — в следующих итерациях.
//
// ============================================================================

RHI_RenderTargetView CRenderBackendDX12::CreateRTV(RHI_TextureHandle, u32, u32) { return {}; }
RHI_DepthStencilView CRenderBackendDX12::CreateDSV(RHI_TextureHandle, u32, u32) { return {}; }
RHI_RenderTargetView CRenderBackendDX12::GetBackBufferRTV() const { return m_backBufferRTV; }
RHI_DepthStencilView CRenderBackendDX12::GetBackBufferDSV() const { return m_backBufferDSV; }
void CRenderBackendDX12::DestroyRTV(RHI_RenderTargetView) {}
void CRenderBackendDX12::DestroyDSV(RHI_DepthStencilView) {}

void CRenderBackendDX12::SetRenderTargets(const RHI_RenderTargetView*, uint32_t, RHI_DepthStencilView) {}
void CRenderBackendDX12::ClearRenderTarget(RHI_RenderTargetView, const fvec4&) {}
void CRenderBackendDX12::ClearDepthStencil(RHI_DepthStencilView, float, u8) {}
void CRenderBackendDX12::Clear(uint32_t, const fvec4, float, uint8_t) {}

RHI_TextureHandle CRenderBackendDX12::CreateTexture(const RHI_TextureDesc&, const void*) { return {}; }
void CRenderBackendDX12::DestroyTexture(RHI_TextureHandle) {}
bool CRenderBackendDX12::CheckFormatSupport(RHI_Format, bool, bool, bool) { return false; }
void* CRenderBackendDX12::GetTextureNativeHandle(RHI_TextureHandle) { return nullptr; }
bool CRenderBackendDX12::GetCubeMapFaceNative(RHI_TextureHandle, uint32_t, uint32_t, void**) { return false; }

RHI_BufferHandle CRenderBackendDX12::CreateVertexBuffer(const RHI_BufferDesc&, const void*) { return {}; }
RHI_BufferHandle CRenderBackendDX12::CreateIndexBuffer(const RHI_BufferDesc&, const void*) { return {}; }
void CRenderBackendDX12::DestroyBuffer(RHI_BufferHandle) {}
void* CRenderBackendDX12::LockBuffer(RHI_BufferHandle, uint32_t, uint32_t, uint32_t) { return nullptr; }
void CRenderBackendDX12::UnlockBuffer(RHI_BufferHandle) {}

RHI_InputLayoutHandle CRenderBackendDX12::CreateInputLayout(const RHI_InputLayoutDesc&) { return {}; }
void CRenderBackendDX12::DestroyInputLayout(RHI_InputLayoutHandle) {}

void CRenderBackendDX12::SetVertexBuffer(uint32_t, RHI_BufferHandle, uint32_t, uint32_t) {}
void CRenderBackendDX12::SetIndexBuffer(RHI_BufferHandle, RHI_IndexFormat) {}
void CRenderBackendDX12::SetInputLayout(RHI_InputLayoutHandle) {}
void CRenderBackendDX12::SetPrimitiveTopology(RHI_Topology) {}

void CRenderBackendDX12::SetBlendState(const RHI_BlendState& s) { m_blendCache = s; }
void CRenderBackendDX12::SetDepthStencilState(const RHI_DepthStencilState& s) { m_depthCache = s; }
void CRenderBackendDX12::SetRasterizerState(const RHI_RasterizerState& s) { m_rasterCache = s; }

void CRenderBackendDX12::SetViewport(const RHI_Viewport& vp) { m_viewportCache = vp; }
void CRenderBackendDX12::SetScissorRect(const RHI_Rect* r)
{
	if (r) { m_scissorCache = *r; m_scissorEnabled = true; }
	else   { m_scissorEnabled = false; }
}
bool CRenderBackendDX12::GetScissorRect(RHI_Rect& out) const
{
	if (!m_scissorEnabled) return false;
	out = m_scissorCache;
	return true;
}
void CRenderBackendDX12::InvalidateStateCache() {}

RHI_ShaderCompileResult CRenderBackendDX12::CompileShader(const RHI_ShaderCompileDesc& d)
{
	RHI_ShaderCompileResult r;
	r.errorMessage = "DX12 shader compilation not implemented yet";
	(void)d;
	return r;
}
bool CRenderBackendDX12::GetShaderBytecode(RHI_ShaderHandle, const void**, size_t*) { return false; }
RHI_ShaderHandle CRenderBackendDX12::CreateVertexShader(const void*, size_t, const char*) { return {}; }
RHI_ShaderHandle CRenderBackendDX12::CreatePixelShader(const void*, size_t, const char*) { return {}; }
void CRenderBackendDX12::SetVertexShader(RHI_ShaderHandle) {}
void CRenderBackendDX12::SetPixelShader(RHI_ShaderHandle) {}
void CRenderBackendDX12::SetSampler(uint32_t, const RHI_SamplerDesc&) {}
void CRenderBackendDX12::SetShaderResource(uint32_t, RHI_TextureHandle) {}
void CRenderBackendDX12::SetShaderConstants(RHI_ShaderType, uint32_t, const float*, uint32_t) {}
void CRenderBackendDX12::DestroyShader(RHI_ShaderHandle) {}

void CRenderBackendDX12::Draw(uint32_t, uint32_t) {}
void CRenderBackendDX12::DrawIndexed(uint32_t, uint32_t, uint32_t) {}
void CRenderBackendDX12::DrawFullscreen() {}
////////////////////////////////////////////////////////////////////////////////
