#include "pch.h"

#include <SoftX/SoftX.h>

SOFTX_BEGIN

Device::Device(const PresentParameters& params)
    : presentParams(params), 
    backBuffer(params.BackBufferSize), 
    depthBuffer(params.BackBufferSize)
{
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
