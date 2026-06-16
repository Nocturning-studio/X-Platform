/////////////////////////////////////////////////////////////////
// SoftX – Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "pch.h"

#include <SoftX.h>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

Device::Device(const PresentParameters& params): presentParams(params), 
                                                 backBuffer(params.BackBufferSize), 
                                                 depthBuffer(params.BackBufferSize)
{
}

Device Device::CreateHeadless(uint2 backBufferSize)
{
    PresentParameters params;
    params.BackBufferSize = backBufferSize;
    params.Headless = true;
    return Device(params);
}

void Device::SetDeviceContext(DeviceContext ctx)
{
    immediateContext = std::move(ctx);
}

DeviceContext& Device::GetDeviceContext()
{
    return immediateContext;
}

const DeviceContext& Device::GetDeviceContext() const
{
    return immediateContext;
}

std::unique_ptr<DeviceContext> Device::CreateDeferredContext()
{
    return std::make_unique<DeviceContext>();
}

Framebuffer& Device::GetBackBuffer()
{
    return backBuffer;
}

const Framebuffer& Device::GetBackBuffer() const
{
    return backBuffer;
}

PresentParameters& Device::GetPresentParams()
{
    return presentParams;
}

const PresentParameters& Device::GetPresentParams() const
{
    return presentParams;
}

void Device::Present()
{
    PROFILE_SCOPE("Device::Present");

    if (presentParams.Headless || presentParams.hDeviceWindow == nullptr)
        return;

    HDC hdc = GetDC(presentParams.hDeviceWindow);
    if (hdc)
    {
        RECT clientRect;
        GetClientRect(presentParams.hDeviceWindow, &clientRect);
        int2 dstSize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
        backBuffer.Present(hdc, int2(0, 0), dstSize);
        ReleaseDC(presentParams.hDeviceWindow, hdc);
    }
}

SOFTX_END
/////////////////////////////////////////////////////////////////
