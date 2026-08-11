/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <memory>

#include "DeviceContext.h"
#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API Device
{
public:
    explicit Device(const PresentParameters& params, size_t numThreads = 0);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Device&&) noexcept;
    Device& operator=(Device&&) noexcept;

    static Device CreateHeadless(uint2 backBufferSize);

    void SetDeviceContext(std::unique_ptr<DeviceContext> ctx);
    DeviceContext& GetDeviceContext();
    const DeviceContext& GetDeviceContext() const;

    DeviceContext& GetImmediateContext();
    const DeviceContext& GetImmediateContext() const;

    std::unique_ptr<DeviceContext> CreateDeferredContext()
    {
        return std::make_unique<DeviceContext>();
    }

    std::shared_ptr<Texture> GetBackBuffer();
    std::shared_ptr<DepthBuffer> GetDepthBuffer();

    PresentParameters& GetPresentParams();
    const PresentParameters& GetPresentParams() const;

    void Reset(const PresentParameters& newParams);
    void Present();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
