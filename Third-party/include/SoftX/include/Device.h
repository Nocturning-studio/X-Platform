/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <functional>
#include <windows.h>

#include "DeviceContext.h"
#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API Device
{
public:
    explicit Device(const PresentParameters& params);
    ~Device();

    Device(Device&&) = default;
    Device& operator=(Device&&) = default;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device CreateHeadless(uint2 backBufferSize);

    SOFTX_FORCE_INLINE void SetDeviceContext(DeviceContext ctx) { immediateContext = std::move(ctx); }
    SOFTX_FORCE_INLINE DeviceContext& GetDeviceContext() { return immediateContext; }
    SOFTX_FORCE_INLINE const DeviceContext& GetDeviceContext() const { return immediateContext; }

    SOFTX_FORCE_INLINE DeviceContext& GetImmediateContext() { return immediateContext; }
    SOFTX_FORCE_INLINE const DeviceContext& GetImmediateContext() const { return immediateContext; }

    SOFTX_FORCE_INLINE std::unique_ptr<DeviceContext> CreateDeferredContext() { return std::make_unique<DeviceContext>(); }

    SOFTX_FORCE_INLINE std::shared_ptr<FrameBuffer> GetBackBuffer() { return backBuffer; }
    SOFTX_FORCE_INLINE std::shared_ptr<DepthBuffer> GetDepthBuffer() { return depthBuffer; }

    SOFTX_FORCE_INLINE PresentParameters& GetPresentParams() { return presentParams; }
    SOFTX_FORCE_INLINE const PresentParameters& GetPresentParams() const { return presentParams; }

    void Present();

private:
    void SetupOutputConsole();
    void DestroyOutputConsole();

    void PresentToWindow();
    void PresentToConsole();

private:
    PresentParameters presentParams;
    std::shared_ptr<FrameBuffer> backBuffer;
    std::shared_ptr<DepthBuffer> depthBuffer;
    DeviceContext immediateContext;
    HANDLE hConsoleBuffer = nullptr;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
