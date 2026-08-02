/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "ThreadUtils.h"
#include <Windows.h>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class Device::Impl
{
public:
    std::unique_ptr<DeviceContext> immediateContext;
    std::shared_ptr<Texture> backBuffer;
    std::shared_ptr<Texture> frontBuffer;
    std::shared_ptr<DepthBuffer> depthBuffer;
    HANDLE hConsoleBuffer = nullptr;
    PresentParameters presentParams;
    mutable std::mutex m_mutex;
    std::future<void> pendingPresent;

    Impl(const PresentParameters& params, size_t numThreads)
        : presentParams(params)
        , backBuffer(std::make_shared<Texture>(params.BackBufferSize))
        , frontBuffer(std::make_shared<Texture>(params.BackBufferSize))
        , depthBuffer(std::make_shared<DepthBuffer>(params.BackBufferSize))
        , immediateContext(std::make_unique<DeviceContext>())
    {
        presentParams.Validate();
        ThreadPoolManager::Initialize(numThreads);

        if (!presentParams.Headless)
        {
            immediateContext->SetRenderTarget(backBuffer, false);
            immediateContext->SetDepthBuffer(depthBuffer);
        }

        if (presentParams.Output == PresentationMode::Console)
            SetupOutputConsole();
    }

    ~Impl()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        DestroyOutputConsole();
    }

    void SetupOutputConsole()
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hStdOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hStdOut, &csbi))
        {
            hConsoleBuffer = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE,
                                                        0, nullptr,
                                                        CONSOLE_TEXTMODE_BUFFER,
                                                        nullptr );
            if (hConsoleBuffer != INVALID_HANDLE_VALUE)
            {
                SHORT w = static_cast<SHORT>(presentParams.ConsoleSize.x);
                SHORT h = static_cast<SHORT>(presentParams.ConsoleSize.y);

                SMALL_RECT minRect = { 0, 0, 1, 1 };
                SetConsoleWindowInfo(hConsoleBuffer, TRUE, &minRect);

                COORD bufSize = { w, h };
                SetConsoleScreenBufferSize(hConsoleBuffer, bufSize);

                SMALL_RECT targetRect = { 0, 0, w - 1, h - 1 };
                SetConsoleWindowInfo(hConsoleBuffer, TRUE, &targetRect);

                SetConsoleActiveScreenBuffer(hConsoleBuffer);
            }
        }
    }

    void DestroyOutputConsole()
    {
        if (hConsoleBuffer)
        {
            HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hStdOut != INVALID_HANDLE_VALUE)
                SetConsoleActiveScreenBuffer(hStdOut);
            CloseHandle(hConsoleBuffer);
            hConsoleBuffer = nullptr;
        }
    }

    void Reset(const PresentParameters& newParams)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (pendingPresent.valid())
            pendingPresent.wait();

        newParams.Validate();

        const bool consoleWasActive = (presentParams.Output == PresentationMode::Console && !presentParams.Headless);
        const bool consoleWillBeActive = (newParams.Output == PresentationMode::Console && !newParams.Headless);

        if (consoleWasActive)
            DestroyOutputConsole();

        presentParams = newParams;

        if (!presentParams.Headless)
        {
            backBuffer = std::make_shared<Texture>(presentParams.BackBufferSize);
            frontBuffer = std::make_shared<Texture>(presentParams.BackBufferSize);
            depthBuffer = std::make_shared<DepthBuffer>(presentParams.BackBufferSize);

            immediateContext->SetRenderTarget(backBuffer, false);
            immediateContext->SetDepthBuffer(depthBuffer);
        }
        else
        {
            backBuffer = std::make_shared<Texture>(uint2(1, 1));
            frontBuffer.reset();
            depthBuffer = std::make_shared<DepthBuffer>(uint2(1, 1));

            immediateContext->SetRenderTarget(backBuffer, false);
            immediateContext->SetDepthBuffer(depthBuffer);
        }

        if (consoleWillBeActive)
            SetupOutputConsole();
    }

    void Present()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        PROFILE_SCOPE("Device::Present");

        if (presentParams.Headless)
            return;

        if (pendingPresent.valid())
            pendingPresent.wait();

        std::swap(backBuffer, frontBuffer);
        immediateContext->SetRenderTarget(backBuffer, false);

        pendingPresent = ThreadPoolManager::Get().enqueueBackground([this]
        {
            if (presentParams.Output == PresentationMode::Console)
                PresentToConsole();
            else
                PresentToWindow();
        });
    }

    void PresentToWindow()
    {
        PROFILE_SCOPE("Device::PresentToWindow");
        if (presentParams.hDeviceWindow == nullptr)
            return;

        HDC hdc = GetDC(presentParams.hDeviceWindow);
        if (!hdc) return;

        RECT clientRect;
        GetClientRect(presentParams.hDeviceWindow, &clientRect);
        int dstW = clientRect.right - clientRect.left;
        int dstH = clientRect.bottom - clientRect.top;

        const Texture* tex = frontBuffer.get();
        const uint w = tex->Width();
        const uint h = tex->Height();
        const size_t totalPixels = static_cast<size_t>(w) * h;

        uint32_t* bgraPixels = static_cast<uint32_t*>(_aligned_malloc(totalPixels * sizeof(uint32_t), 16));
        if (!bgraPixels) return;

        const __m128* src = tex->GetRawPixels();
        const __m128 scale = _mm_set1_ps(255.0f);
        const __m128i shuffleMask = _mm_setr_epi8(8, 4, 0, 12,
                                                  -128, -128, -128, -128,
                                                  -128, -128, -128, -128,
                                                  -128, -128, -128, -128);

        std::atomic<uint32_t> nextRow(0);
        auto convertRow = [&]()
        {
            while (true)
            {
                uint32_t y = nextRow.fetch_add(1);
                if (y >= h) break;

                const __m128* srcRow = src + y * w;
                uint32_t* dstRow = bgraPixels + y * w;

                for (uint32_t x = 0; x < w; ++x)
                {
                    __m128 px = srcRow[x];
                    __m128 scaled = _mm_mul_ps(px, scale);
                    __m128i int32s = _mm_cvtps_epi32(scaled);
                    __m128i shuffled = _mm_shuffle_epi8(int32s, shuffleMask);
                    dstRow[x] = static_cast<uint32_t>(_mm_cvtsi128_si32(shuffled));
                }
            }
        };

        ThreadUtils::DispatchWorkers(convertRow);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = (LONG)w;
        bmi.bmiHeader.biHeight = -(LONG)h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        SetDIBitsToDevice(hdc, 0, 0, dstW, dstH, 0, 0, 0, h, bgraPixels, &bmi, DIB_RGB_COLORS);
        ReleaseDC(presentParams.hDeviceWindow, hdc);
        _aligned_free(bgraPixels);
    }

    void PresentToConsole()
    {
        PROFILE_SCOPE("Device::PresentToConsole");
        if (hConsoleBuffer == nullptr)
            return;

        const Texture* tex = frontBuffer.get();
        uint srcW = tex->Width();
        uint srcH = tex->Height();
        uint2 consoleSize = presentParams.ConsoleSize;
        uint dstW = consoleSize.x;
        uint dstH = consoleSize.y;

        static const char gradient[] = " .:!/r(l1Z4H9W8$@";
        static const int gradientSize = static_cast<int>(std::size(gradient)) - 2;

        std::vector<char> charBuffer(dstW * dstH);
        float scaleX = static_cast<float>(srcW) / dstW;
        float scaleY = static_cast<float>(srcH) / dstH;

        const uint totalPixels = dstW * dstH;
        std::atomic<uint> pixelIndex(0);

        auto Task = [&]()
        {
            while (true)
            {
                uint idx = pixelIndex.fetch_add(1);
                if (idx >= totalPixels) break;

                uint y = idx / dstW;
                uint x = idx % dstW;

                uint srcYStart = static_cast<uint>(y * scaleY);
                uint srcYEnd = (y == dstH - 1) ? srcH : static_cast<uint>((y + 1) * scaleY);
                if (srcYEnd == 0) srcYEnd = 1;

                uint srcXStart = static_cast<uint>(x * scaleX);
                uint srcXEnd = (x == dstW - 1) ? srcW : static_cast<uint>((x + 1) * scaleX);
                if (srcXEnd == 0) srcXEnd = 1;

                float rSum = 0, gSum = 0, bSum = 0;
                uint samples = 0;

                for (uint sy = srcYStart; sy < srcYEnd && sy < srcH; ++sy)
                {
                    for (uint sx = srcXStart; sx < srcXEnd && sx < srcW; ++sx)
                    {
                        __m128 color = tex->Read(uint2(sx, sy));
                        float rgba[4];
                        _mm_storeu_ps(rgba, color);
                        rSum += rgba[0];
                        gSum += rgba[1];
                        bSum += rgba[2];
                        ++samples;
                    }
                }

                if (samples > 0)
                {
                    rSum /= samples;
                    gSum /= samples;
                    bSum /= samples;

                    float luminance = 0.2126f * rSum + 0.7152f * gSum + 0.0722f * bSum;
                    luminance = AfterMath::clamp(luminance, 0.0f, 1.0f);

                    int gradIdx = static_cast<int>(luminance * gradientSize);
                    gradIdx = AfterMath::clamp(gradIdx, 0, gradientSize);
                    charBuffer[idx] = gradient[gradIdx];
                }
                else
                {
                    charBuffer[idx] = ' ';
                }
            }
        };
        ThreadUtils::DispatchWorkers(Task);

        DWORD written = 0;
        COORD coord = { 0, 0 };
        WriteConsoleOutputCharacterA(hConsoleBuffer, charBuffer.data(),
            static_cast<DWORD>(totalPixels), coord, &written);
    }
};

Device::Device(const PresentParameters& params, size_t numThreads)
    : pImpl(std::make_unique<Impl>(params, numThreads))
{
}

Device::~Device() = default;

Device::Device(Device&&) noexcept = default;
Device& Device::operator=(Device&&) noexcept = default;

Device Device::CreateHeadless(uint2 backBufferSize)
{
    PresentParameters params;
    params.BackBufferSize = backBufferSize;
    params.Headless = true;
    return Device(params);
}

void Device::SetDeviceContext(std::unique_ptr<DeviceContext> ctx)
{
    pImpl->immediateContext = std::move(ctx);
}

DeviceContext& Device::GetDeviceContext()
{
    return *pImpl->immediateContext;
}

const DeviceContext& Device::GetDeviceContext() const
{
    return *pImpl->immediateContext;
}

DeviceContext& Device::GetImmediateContext()
{
    return *pImpl->immediateContext;
}

const DeviceContext& Device::GetImmediateContext() const
{
    return *pImpl->immediateContext;
}

std::shared_ptr<Texture> Device::GetBackBuffer()
{
    return pImpl->backBuffer;
}

std::shared_ptr<DepthBuffer> Device::GetDepthBuffer()
{
    return pImpl->depthBuffer;
}

PresentParameters& Device::GetPresentParams()
{
    return pImpl->presentParams;
}

const PresentParameters& Device::GetPresentParams() const
{
    return pImpl->presentParams;
}

void Device::Reset(const PresentParameters & newParams)
{
    pImpl->Reset(newParams);
}

void Device::Present()
{
    pImpl->Present();
}

SOFTX_END
/////////////////////////////////////////////////////////////////
