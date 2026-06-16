/////////////////////////////////////////////////////////////////
// SoftX – Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
#include "TextureInterface.h"
#include "DepthBuffer.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API DepthTextureView : public ITexture
{
public:
    explicit DepthTextureView(const DepthBuffer* buffer) : m_buffer(buffer) {}

    uint Width() const override { return m_buffer->Width(); }
    uint Height() const override { return m_buffer->Height(); }

    float4 Sample(float2 uv) const override
    {
        // Простая Nearest-фильтрация
        uint x = (uint)(uv.x * m_buffer->Width());
        uint y = (uint)(uv.y * m_buffer->Height());
        float d = m_buffer->At(int2(x, y));
        return float4(d, d, d, 1.0f);
    }

    float4 SampleBilinear(float2 uv) const override
    {
        int w = m_buffer->Width();
        int h = m_buffer->Height();

        float fx = uv.x * w - 0.5f;
        float fy = uv.y * h - 0.5f;
        int x0 = (int)std::floor(fx);
        int y0 = (int)std::floor(fy);
        x0 = AfterMath::clamp(x0, 0, w - 1);
        y0 = AfterMath::clamp(y0, 0, h - 1);
        int x1 = AfterMath::clamp(x0 + 1, 0, w - 1);
        int y1 = AfterMath::clamp(y0 + 1, 0, h - 1);

        float tx = fx - x0;
        float ty = fy - y0;

        float d00 = m_buffer->At(int2(x0, y0));
        float d10 = m_buffer->At(int2(x1, y0));
        float d01 = m_buffer->At(int2(x0, y1));
        float d11 = m_buffer->At(int2(x1, y1));

        float d = (1 - tx) * (1 - ty) * d00 + tx * (1 - ty) * d10 +
            (1 - tx) * ty * d01 + tx * ty * d11;
        return float4(d, d, d, 1.0f);
    }

    __m128 FetchRaw(int x, int y) const override
    {
        float d = m_buffer->At(int2(x, y));
        return _mm_set_ps(1.0f, d, d, d);
    }

private:
    const DepthBuffer* m_buffer;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
