/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "pch.h"

#include "../include/SoftX.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

Device::Device(const PresentParameters& params): presentParams(params), 
                                                 backBuffer(std::make_shared<FrameBuffer>(params.BackBufferSize)),
                                                 depthBuffer(std::make_shared<DepthBuffer>(params.BackBufferSize))
{
    if (presentParams.Output == PresentationMode::Console)
        SetupOutputConsole();
}

Device::~Device()
{
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
        backBuffer->PresentBitmap(hdc, int2(0, 0), dstSize);
        ReleaseDC(presentParams.hDeviceWindow, hdc);
    }
}

void Device::PresentToConsole()
{
    PROFILE_SCOPE("Device::PresentToConsole");

    if (hConsoleBuffer == nullptr)
        return;

    backBuffer->PresentASCII(hConsoleBuffer, presentParams.ConsoleSize);
}

void Device::Present()
{
    PROFILE_SCOPE("Device::Present");

    if (presentParams.Headless)
        return;

    if (presentParams.Output == PresentationMode::Console)
        PresentToConsole();
    else
        PresentToWindow();
}

SOFTX_END
/////////////////////////////////////////////////////////////////
