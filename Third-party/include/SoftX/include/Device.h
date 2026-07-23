/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <functional>
#include <future>

#include "DeviceContext.h"
#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
/////////////////////////////////////////////////////////////////
typedef void* HANDLE;
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API Device
{
public:
    explicit Device(const PresentParameters& params, size_t numThreads = 0);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Device&&) = default;
    Device& operator=(Device&&) = default;

    static Device CreateHeadless(uint2 backBufferSize);

    void SetDeviceContext(std::unique_ptr<DeviceContext> ctx);
    DeviceContext& GetDeviceContext() { return *immediateContext; }
    const DeviceContext& GetDeviceContext() const { return *immediateContext; }

    DeviceContext& GetImmediateContext() { return *immediateContext; }
    const DeviceContext& GetImmediateContext() const { return *immediateContext; }

    std::unique_ptr<DeviceContext> CreateDeferredContext() { return std::make_unique<DeviceContext>(); }

    SOFTX_FORCE_INLINE std::shared_ptr<FrameBuffer> GetBackBuffer() { return backBuffer; }
    SOFTX_FORCE_INLINE std::shared_ptr<DepthBuffer> GetDepthBuffer() { return depthBuffer; }

    SOFTX_FORCE_INLINE PresentParameters& GetPresentParams() { return presentParams; }
    SOFTX_FORCE_INLINE const PresentParameters& GetPresentParams() const { return presentParams; }

    void Reset(const PresentParameters& newParams);

    void Present();

private:
    void SetupOutputConsole();
    void DestroyOutputConsole();

    void PresentToWindow();
    void PresentToConsole();

private:
    std::unique_ptr<DeviceContext> immediateContext;
    std::shared_ptr<FrameBuffer> backBuffer;
    std::shared_ptr<FrameBuffer> frontBuffer;
    std::shared_ptr<DepthBuffer> depthBuffer;
    HANDLE hConsoleBuffer = nullptr;
    PresentParameters presentParams;
    mutable std::mutex m_mutex;
    std::future<void> pendingPresent;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
