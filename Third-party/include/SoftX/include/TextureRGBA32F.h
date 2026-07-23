/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <algorithm>
#include <cassert>
#include <vector>
#include <xmmintrin.h>

#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
#include "TextureInterface.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API TextureRGBA32F : public ITexture
{
    public:
    explicit TextureRGBA32F(uint2 size, uint numMips = 1)
    {
        uint actualMips = std::max(1u, numMips);
        levels.resize(actualMips);

        uint totalPixels = 0;
        for (uint i = 0; i < actualMips; ++i)
        {
            uint w = std::max(1u, size.x >> i);
            uint h = std::max(1u, size.y >> i);
            levels[i].resolution = uint2(w, h);
            levels[i].offset = totalPixels;
            totalPixels += w * h;
        }

        pixels.resize(totalPixels);

        __m128 zero = _mm_setzero_ps();
        __m128* ptr = pixels.data();
        for (size_t i = 0; i < totalPixels; ++i)
            _mm_stream_ps(reinterpret_cast<float*>(ptr + i), zero);
        _mm_sfence();
    }

    SOFTX_FORCE_INLINE __m128* GetRawPixels(uint level = 0)
    {
        level = std::min(level, static_cast<uint>(levels.size()) - 1);
        return pixels.data() + levels[level].offset;
    }
    SOFTX_FORCE_INLINE const __m128* GetRawPixels(uint level = 0) const
    {
        level = std::min(level, static_cast<uint>(levels.size()) - 1);
        return pixels.data() + levels[level].offset;
    }

    SOFTX_FORCE_INLINE __m128 Read(uint2 coords) const
    {
        const Level& lvl0 = levels[0];
        assert(coords.x < lvl0.resolution.x && coords.y < lvl0.resolution.y);
        return pixels[lvl0.offset + coords.y * lvl0.resolution.x + coords.x];
    }

    SOFTX_FORCE_INLINE __m128 Read(uint index) const
    {
        const Level& lvl0 = levels[0];
        assert(index < lvl0.resolution.x * lvl0.resolution.y);
        return pixels[lvl0.offset + index];
    }

    SOFTX_FORCE_INLINE __m128 SampleRaw(float2 uv) const
    {
        const Level& lvl0 = levels[0];
        uint x = static_cast<uint>(uv.x * static_cast<float>(lvl0.resolution.x));
        uint y = static_cast<uint>(uv.y * static_cast<float>(lvl0.resolution.y));
        if (x >= lvl0.resolution.x)
            x = lvl0.resolution.x - 1;
        if (y >= lvl0.resolution.y)
            y = lvl0.resolution.y - 1;
        return pixels[lvl0.offset + y * lvl0.resolution.x + x];
    }

    SOFTX_FORCE_INLINE __m128 SampleBilinearRaw(float2 uv) const
    {
        return SampleBilinearRaw(uv, 0);
    }

    SOFTX_FORCE_INLINE __m128 SampleBilinearRaw(float2 uv, uint level) const
    {
        uint mipLevel = std::min(level, static_cast<uint>(levels.size()) - 1);
        const Level& lvl = levels[mipLevel];
        uint w = lvl.resolution.x;
        uint h = lvl.resolution.y;

        float fx = uv.x * static_cast<float>(w) - 0.5f;
        float fy = uv.y * static_cast<float>(h) - 0.5f;

        int x0 = static_cast<int>(std::floor(fx));
        int y0 = static_cast<int>(std::floor(fy));

        float tx = fx - static_cast<float>(x0);
        float ty = fy - static_cast<float>(y0);

        __m128 c00 = FetchRaw(x0, y0, mipLevel);
        __m128 c10 = FetchRaw(x0 + 1, y0, mipLevel);
        __m128 c01 = FetchRaw(x0, y0 + 1, mipLevel);
        __m128 c11 = FetchRaw(x0 + 1, y0 + 1, mipLevel);

        __m128 wtx = _mm_set1_ps(tx);
        __m128 wty = _mm_set1_ps(ty);
        __m128 one = _mm_set1_ps(1.0f);
        __m128 w1tx = _mm_sub_ps(one, wtx);
        __m128 w1ty = _mm_sub_ps(one, wty);

        __m128 w00 = _mm_mul_ps(w1tx, w1ty);
        __m128 w10 = _mm_mul_ps(wtx, w1ty);
        __m128 w01 = _mm_mul_ps(w1tx, wty);
        __m128 w11 = _mm_mul_ps(wtx, wty);

        return _mm_add_ps(_mm_add_ps(_mm_mul_ps(c00, w00), _mm_mul_ps(c10, w10)),
                          _mm_add_ps(_mm_mul_ps(c01, w01), _mm_mul_ps(c11, w11)));
    }

    SOFTX_FORCE_INLINE void StreamWrite(uint2 coords, __m128 color)
    {
        StreamWrite(coords, color, 0);
    }

    SOFTX_FORCE_INLINE void StreamWrite(uint index, __m128 color)
    {
        const Level& lvl0 = levels[0];
        assert(index < lvl0.resolution.x * lvl0.resolution.y);
        _mm_stream_ps(reinterpret_cast<float*>(&pixels[lvl0.offset + index]), color);
    }

    SOFTX_FORCE_INLINE void StreamWrite(uint2 coords, __m128 color, uint level)
    {
        uint mipLevel = std::min(level, static_cast<uint>(levels.size()) - 1);
        const Level& lvl = levels[mipLevel];
        uint index = coords.y * lvl.resolution.x + coords.x;
        _mm_stream_ps(reinterpret_cast<float*>(&pixels[lvl.offset + index]), color);
    }

    SOFTX_FORCE_INLINE uint MipWidth(uint level) const
    {
        level = std::min(level, static_cast<uint>(levels.size()) - 1);
        return levels[level].resolution.x;
    }

    SOFTX_FORCE_INLINE uint MipHeight(uint level) const
    {
        level = std::min(level, static_cast<uint>(levels.size()) - 1);
        return levels[level].resolution.y;
    }

    float4 Sample(float2 uv) const override
    {
        __m128 color = SampleRaw(uv);
        return float4(color);
    }

    float4 SampleBilinear(float2 uv) const override
    {
        return float4(SampleBilinearRaw(uv));
    }

    float4 SampleLevel(float2 uv, float lod) const override
    {
        uint level = static_cast<uint>(lod + 0.5f);
        level = std::min(level, static_cast<uint>(levels.size()) - 1);
        return float4(SampleBilinearRaw(uv, level));
    }

    __m128 FetchRaw(int x, int y) const override
    {
        const Level& lvl0 = levels[0];
        x = AfterMath::clamp(x, 0, static_cast<int>(lvl0.resolution.x) - 1);
        y = AfterMath::clamp(y, 0, static_cast<int>(lvl0.resolution.y) - 1);
        return pixels[lvl0.offset + y * lvl0.resolution.x + x];
    }

    __m128 FetchRaw(int x, int y, uint level) const override
    {
        uint mipLevel = std::min(level, static_cast<uint>(levels.size()) - 1);
        const Level& lvl = levels[mipLevel];
        x = AfterMath::clamp(x, 0, static_cast<int>(lvl.resolution.x) - 1);
        y = AfterMath::clamp(y, 0, static_cast<int>(lvl.resolution.y) - 1);
        return pixels[lvl.offset + y * lvl.resolution.x + x];
    }

    uint Width() const override
    {
        return levels[0].resolution.x;
    }
    uint Height() const override
    {
        return levels[0].resolution.y;
    }
    uint MipCount() const override
    {
        return static_cast<uint>(levels.size());
    }
    uint2 Size() const override
    {
        return levels[0].resolution;
    }

    void GenerateMips();
    void SaveToTGA(const char* filename) const;

    private:
    struct Level
    {
        uint2 resolution;
        uint offset;
    };

    std::vector<__m128> pixels;
    std::vector<Level> levels;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
