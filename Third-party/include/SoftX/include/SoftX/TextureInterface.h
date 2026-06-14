#pragma once

#include "LibInternal.h"
#include "ThirdPartyIncluding.h"

SOFTX_BEGIN

class SOFTX_API ITexture
{
public:
    virtual ~ITexture() = default;

    virtual uint Width() const = 0;
    virtual uint Height() const = 0;
    uint2 Size() const { return uint2(Width(), Height()); }

    virtual float4 Sample(float2 uv) const = 0;
    virtual float4 SampleBilinear(float2 uv) const = 0;

    virtual __m128 FetchRaw(int x, int y) const = 0;
};

SOFTX_END
