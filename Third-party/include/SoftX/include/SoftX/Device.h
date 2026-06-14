#pragma once

#include <functional>
#include <windows.h>

#include "DeviceContext.h"
#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
#include "ThreadPool.h"

SOFTX_BEGIN

class SOFTX_API Device
{
public:
    explicit Device(const PresentParameters& params);
    ~Device() = default;

    void SetDeviceContext(DeviceContext ctx);
    DeviceContext& GetDeviceContext();
    const DeviceContext& GetDeviceContext() const;

    DeviceContext& GetImmediateContext()
    {
        return immediateContext;
    }
    const DeviceContext& GetImmediateContext() const
    {
        return immediateContext;
    }

    std::unique_ptr<DeviceContext> CreateDeferredContext();

    void Present();

    Framebuffer& GetBackBuffer();
    const Framebuffer& GetBackBuffer() const;

    PresentParameters& GetPresentParams();
    const PresentParameters& GetPresentParams() const;

private:
    PresentParameters presentParams;
    Framebuffer backBuffer;
    DepthBuffer depthBuffer;
    DeviceContext immediateContext;
};

SOFTX_END
