// test.cpp
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <d3d9.h>
#include <d3dx9.h>
#include <xrRHI/xrRHI.h>
#include <xrMath/xrMath.h>

// Для простоты определим макрос проверки
#define TEST_ASSERT(cond, msg)                                                                                         \
	if (!(cond))                                                                                                       \
	{                                                                                                                  \
		printf("FAILED: %s\n", msg);                                                                                   \
		return 1;                                                                                                      \
	}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main()
{
	xrRHI::Print("Starting RHI test with samplers...\n");

	// 1. Создаём окно
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
	xrRHI::Print("xrRHI.dll loaded.\n");

	// 3. Получаем фабричную функцию
	typedef xrRHI::IRenderBackend* (*CreateBackendFunc)(xrRHI::BackendType);
	CreateBackendFunc createBackend = (CreateBackendFunc)GetProcAddress(hDLL, "CreateRenderBackend");
	TEST_ASSERT(createBackend, "Failed to get CreateRenderBackend");

	// 4. Создаём бекенд DirectX9
	xrRHI::IRenderBackend* backend = createBackend(xrRHI::BackendType::DirectX9);
	TEST_ASSERT(backend, "Failed to create backend");
	xrRHI::Print("Backend created.\n");

	// 5. Создаём устройство
	bool created = backend->CreateDevice(hWnd, true, 800, 600, 0); // 0 = immediate
	TEST_ASSERT(created, "CreateDevice failed");
	xrRHI::Print("Device created.\n");

	// 6. Создаём простую текстуру 2x2 RGBA8
	xrRHI::TextureDesc texDesc;
	texDesc.width = 2;
	texDesc.height = 2;
	texDesc.depth = 1;
	texDesc.mipLevels = 1;
	texDesc.format = xrRHI::RHI_Format::RGBA8_UNORM;
	texDesc.isRenderTarget = false;
	texDesc.isDepthStencil = false;
	texDesc.isCubeMap = false;

	// Данные текстуры: 4 пикселя (красный, зелёный, синий, белый)
	uint32_t pixelData[4] = {
		0xFF0000FF, // красный (ABGR? в D3D9 обычно A8R8G8B8, значит порядок: 0xAARRGGBB)
		0xFF00FF00, // зелёный
		0xFFFF0000, // синий
		0xFFFFFFFF	// белый
	};
	// В D3D9 формат A8R8G8B8, поэтому данные должны быть в порядке A,R,G,B (как в DWORD).
	// Для простоты используем тот же порядок, который ожидается.

	xrRHI::RHITexture texture = backend->CreateTexture(texDesc, pixelData);
	TEST_ASSERT(texture, "CreateTexture failed");
	xrRHI::Print("Texture created.\n");

	// 7. Создаём сэмплер с линейной фильтрацией
	xrRHI::SamplerDesc sampDesc;
	sampDesc.minFilter = xrRHI::RHI_Filter::Linear;
	sampDesc.magFilter = xrRHI::RHI_Filter::Linear;
	sampDesc.mipFilter = xrRHI::RHI_Filter::Linear;
	sampDesc.addressU = xrRHI::RHI_TextureAddress::Wrap;
	sampDesc.addressV = xrRHI::RHI_TextureAddress::Wrap;
	sampDesc.addressW = xrRHI::RHI_TextureAddress::Wrap;
	sampDesc.mipLODBias = 0.0f;
	sampDesc.maxAnisotropy = 1;
	sampDesc.borderColor = fvec4{0, 0, 0, 0};

	xrRHI::RHISampler sampler = backend->CreateSampler(sampDesc);
	TEST_ASSERT(sampler, "CreateSampler failed");
	xrRHI::Print("Sampler created.\n");

	// 8. Устанавливаем текстуру и сэмплер на стадию 0
	backend->SetTexture(0, texture, sampler);
	xrRHI::Print("Texture and sampler set.\n");

	// 9. Очищаем экран и делаем Present
	fvec4 clearColor{0.2f, 0.2f, 0.8f, 1.0f};
	backend->Clear(xrRHI::RHI_CLEAR_TARGET | xrRHI::RHI_CLEAR_ZBUFFER, clearColor, 1.0f, 0);
	backend->Present();
	xrRHI::Print("Clear and Present done.\n");

	// 10. Очистка ресурсов
	backend->DestroySampler(sampler);
	backend->DestroyTexture(texture);
	backend->DestroyDevice();
	delete backend;
	FreeLibrary(hDLL);
	DestroyWindow(hWnd);

	xrRHI::Print("All tests passed!\n");

	Sleep(1000000);

	return 0;
}
