// test.cpp
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <d3d9.h>
#include <d3dx9.h>
#include <xrRHI/xrRHI.h>
#include <xrMath/xrMath.h>

#define TEST_ASSERT(cond, msg)                                                                                         \
	if (!(cond))                                                                                                       \
	{                                                                                                                  \
		printf("FAILED: %s\n", msg);                                                                                   \
		return 1;                                                                                                      \
	}

// Прототип функции создания бекенда
typedef xrRHI::IRenderBackend* (*CreateBackendFunc)(xrRHI::BackendType);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main()
{
	printf("Starting RHI functional test...\n");

	// 1. Создаём простое окно (необходимо для создания устройства)
	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"RHITestWindow";
	RegisterClassEx(&wc);

	HWND hWnd = CreateWindowEx(0, L"RHITestWindow", L"RHI Test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800,
							   600, NULL, NULL, hInstance, NULL);
	TEST_ASSERT(hWnd, "Failed to create window");

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// 2. Загружаем xrRHI.dll
	HMODULE hDLL = LoadLibrary(L"xrRHI.dll");
	TEST_ASSERT(hDLL, "Failed to load xrRHI.dll");
	printf("xrRHI.dll loaded.\n");

	// 3. Получаем адрес фабричной функции
	CreateBackendFunc createBackend = (CreateBackendFunc)GetProcAddress(hDLL, "CreateRenderBackend");
	TEST_ASSERT(createBackend, "Failed to find CreateRenderBackend export");
	printf("CreateRenderBackend found.\n");

	// 4. Создаём бекенд Direct3D9
	xrRHI::IRenderBackend* backend = createBackend(xrRHI::BackendType::DirectX9);
	TEST_ASSERT(backend, "Failed to create backend");
	printf("Backend created.\n");

	// 5. Создаём устройство (оконный режим, 800x600)
	bool created = backend->CreateDevice(hWnd, true, 800, 600, D3DPRESENT_INTERVAL_DEFAULT);
	TEST_ASSERT(created, "CreateDevice failed");
	printf("Device created successfully.\n");

	// 6. Получаем сырые указатели D3D для прямых вызовов
	IDirect3D9Ex* pD3D = (IDirect3D9Ex*)backend->GetD3DHandle();
	IDirect3DDevice9Ex* pDevice = (IDirect3DDevice9Ex*)backend->GetDeviceHandle();
	TEST_ASSERT(pD3D && pDevice, "Failed to get D3D handles");

	// 7. Получаем caps через бекенд
	D3DCAPS9 caps;
	backend->GetDeviceCaps(&caps);
	printf("Caps obtained.\n");

	// 8. Проверки, аналогичные X-Ray Engine
	// 8.1 Версия пиксельных шейдеров (минимально 3.0)
	int raster_major = (caps.PixelShaderVersion >> 8) & 0xFF;
	int raster_minor = caps.PixelShaderVersion & 0xFF;
	int raster_version = raster_major * 10 + raster_minor;
	TEST_ASSERT(raster_version >= 30, "Pixel shader version < 3.0");
	printf("Pixel shader version: %d.%d OK\n", raster_major, raster_minor);

	// 8.2 Количество инструкций пиксельного шейдера
	TEST_ASSERT(caps.PS20Caps.NumInstructionSlots >= 512, "Pixel shader instruction slots < 512");
	printf("PS instruction slots: %d OK\n", caps.PS20Caps.NumInstructionSlots);

	// 8.3 Количество одновременных рендер-таргетов
	TEST_ASSERT(caps.NumSimultaneousRTs >= 3, "NumSimultaneousRTs < 3");
	printf("NumSimultaneousRTs: %d OK\n", caps.NumSimultaneousRTs);

	// 8.4 Поддержка независимых битовых глубин для MRT
	TEST_ASSERT(caps.PrimitiveMiscCaps & D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS,
				"MRT independent bit depths not supported");
	printf("MRT independent bit depths supported.\n");

	// 8.5 Проверка поддержки форматов через CheckDeviceFormat
	// Для этого используем pD3D напрямую
	UINT adapter = D3DADAPTER_DEFAULT;
	D3DDEVTYPE devType = D3DDEVTYPE_HAL;
	D3DFORMAT targetFmt = D3DFMT_X8R8G8B8;

	// D3DFMT_D24X8 как текстура с использованием D3DUSAGE_DEPTHSTENCIL
	HRESULT hr =
		pD3D->CheckDeviceFormat(adapter, devType, targetFmt, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, D3DFMT_D24X8);
	TEST_ASSERT(SUCCEEDED(hr), "D3DFMT_D24X8 not supported as depth-stencil texture");
	printf("D3DFMT_D24X8 supported.\n");

	// D3DFMT_A16B16G16R16F с флагами фильтрации и пост-пиксельного шейдерного блендинга
	hr = pD3D->CheckDeviceFormat(adapter, devType, targetFmt, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE,
								 D3DFMT_A16B16G16R16F);
	TEST_ASSERT(SUCCEEDED(hr), "D3DFMT_A16B16G16R16F not supported for filtering");
	printf("D3DFMT_A16B16G16R16F (filter) supported.\n");

	hr = pD3D->CheckDeviceFormat(adapter, devType, targetFmt, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE,
								 D3DFMT_A16B16G16R16F);
	TEST_ASSERT(SUCCEEDED(hr), "D3DFMT_A16B16G16R16F not supported for post-pixel shader blending");
	printf("D3DFMT_A16B16G16R16F (post-blending) supported.\n");

	// 9. Дополнительная проверка: очистка и презентация (чтобы убедиться, что устройство рабочее)
	float clearColor[4] = {0.2f, 0.2f, 0.8f, 1.0f};
	backend->Clear(xrRHI::RHI_CLEAR_TARGET | xrRHI::RHI_CLEAR_ZBUFFER, clearColor, 1.0f, 0);
	backend->Present();
	printf("Clear and Present done.\n");

	// 10. Очистка
	backend->DestroyDevice();
	delete backend;
	FreeLibrary(hDLL);
	DestroyWindow(hWnd);

	printf("All tests passed successfully!\n");
	return 0;
}
