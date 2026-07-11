/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API ITexture
{
public:
    virtual ~ITexture() = default;

    virtual uint Width() const = 0;
    virtual uint Height() const = 0;
    virtual uint MipCount() const { return 1; }
    uint2 Size() const { return uint2(Width(), Height()); }

    virtual float4 Sample(const float2& uv) const = 0;
    virtual float4 SampleBilinear(const float2& uv) const = 0;
    virtual float4 SampleLevel(const float2& uv, const float& lod) const = 0;

    virtual __m128 FetchRaw(const int& x, const int& y) const = 0;
    virtual __m128 FetchRaw(const int& x, const int& y, const uint& level) const = 0;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
