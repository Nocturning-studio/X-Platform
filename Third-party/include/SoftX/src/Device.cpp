/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "ThreadPoolManager.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

Device::Device(const PresentParameters& params, size_t numThreads): presentParams(params),
                                                                    backBuffer(std::make_shared<FrameBuffer>(params.BackBufferSize)),
                                                                    frontBuffer(std::make_shared<FrameBuffer>(params.BackBufferSize)),
                                                                    depthBuffer(std::make_shared<DepthBuffer>(params.BackBufferSize)),
                                                                    immediateContext(std::make_unique<DeviceContext>())
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

Device::~Device()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    DestroyOutputConsole();
}

Device Device::CreateHeadless(uint2 backBufferSize)
{
    PresentParameters params;
    params.BackBufferSize = backBufferSize;
    params.Headless = true;
    return Device(params);
}

void Device::SetupOutputConsole()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hStdOut, &csbi))
    {
        hConsoleBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                   0,
                                                   nullptr,
                                                   CONSOLE_TEXTMODE_BUFFER,
                                                   nullptr);

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

void Device::DestroyOutputConsole()
{
    if (hConsoleBuffer)
    {
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hStdOut != INVALID_HANDLE_VALUE)
            SetConsoleActiveScreenBuffer(hStdOut);

        CloseHandle(hConsoleBuffer);
    }
}

void Device::Reset(const PresentParameters& newParams)
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
        backBuffer = std::make_shared<FrameBuffer>(presentParams.BackBufferSize);
        frontBuffer = std::make_shared<FrameBuffer>(presentParams.BackBufferSize);
        depthBuffer = std::make_shared<DepthBuffer>(presentParams.BackBufferSize);

        immediateContext->SetRenderTarget(backBuffer, false);
        immediateContext->SetDepthBuffer(depthBuffer);
    }
    else
    {
        backBuffer = std::make_shared<FrameBuffer>(uint2(1, 1));
        frontBuffer.reset();
        depthBuffer = std::make_shared<DepthBuffer>(uint2(1, 1));

        immediateContext->SetRenderTarget(backBuffer, false);
        immediateContext->SetDepthBuffer(depthBuffer);
    }

    if (consoleWillBeActive)
        SetupOutputConsole();
}

void Device::SetDeviceContext(std::unique_ptr<DeviceContext> ctx) {
    immediateContext = std::move(ctx);
}

void Device::PresentToWindow()
{
    PROFILE_SCOPE("Device::PresentToWindow");

    if (presentParams.hDeviceWindow == nullptr)
        return;

    HDC hdc = GetDC(presentParams.hDeviceWindow);
    if (hdc)
    {
        RECT clientRect;
        GetClientRect(presentParams.hDeviceWindow, &clientRect);
        int2 dstSize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
        frontBuffer->PresentBitmap(hdc, int2(0, 0), dstSize);
        ReleaseDC(presentParams.hDeviceWindow, hdc);
    }
}

void Device::PresentToConsole()
{
    PROFILE_SCOPE("Device::PresentToConsole");

    if (hConsoleBuffer == nullptr)
        return;

    frontBuffer->PresentASCII(hConsoleBuffer, presentParams.ConsoleSize);
}

void Device::Present()
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

SOFTX_END
/////////////////////////////////////////////////////////////////
