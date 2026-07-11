/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>
#include <xmmintrin.h>

#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API DepthBuffer
{
public:
    enum class HiZReduction
    {
        Min,
        Max
    };

    struct Level
    {
        uint2 resolution;
        uint widthPadded;
        std::vector<__m128> blocks;
    };

    explicit DepthBuffer(uint2 size, uint numMips = 1)
    {
        assert(size.x % 2 == 0 && size.y % 2 == 0 && "DepthBuffer dimensions must be multiples of 2");

        uint maxMips = CalcMaxMips(size.x, size.y);
        uint actualMips = (numMips == 0) ? maxMips : std::min(numMips, maxMips);

        mipChain.resize(actualMips);

        uint w = size.x, h = size.y;
        for (uint i = 0; i < actualMips; ++i)
        {
            auto& lvl = mipChain[i];
            lvl.resolution = uint2(w, h);
            lvl.widthPadded = (w + 3) & ~3;
            lvl.blocks.resize(lvl.widthPadded * h / 4);
            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
        }

        Clear(1.0f);
    }

    void Clear(float depth)
    {
        __m128 depth4 = _mm_set1_ps(depth);
        auto& lvl = mipChain[0];
        __m128* ptr = lvl.blocks.data();
        size_t count = lvl.blocks.size();

        size_t i = 0;
        for (; i < count; ++i)
            _mm_stream_ps(reinterpret_cast<float*>(ptr + i), depth4);
        _mm_sfence();

        for (uint m = 1; m < MipCount(); ++m)
        {
            auto& lv = mipChain[m];
            ptr = lv.blocks.data();
            count = lv.blocks.size();
            for (i = 0; i < count; ++i)
                _mm_stream_ps(reinterpret_cast<float*>(ptr + i), depth4);
            _mm_sfence();
        }
    }

    SOFTX_FORCE_INLINE float Read(const int2& coords) const
    {
        return FloatPtr()[coords.y * widthPadded0() + coords.x];
    }

    SOFTX_FORCE_INLINE void Write(const int2& coords, const float& depth)
    {
        FloatPtr()[coords.y * widthPadded0() + coords.x] = depth;
    }

    SOFTX_FORCE_INLINE float& At(const int2& coords)
    {
        return FloatPtr()[coords.y * widthPadded0() + coords.x];
    }

    SOFTX_FORCE_INLINE const float& At(const int2& coords) const
    {
        return FloatPtr()[coords.y * widthPadded0() + coords.x];
    }

    SOFTX_FORCE_INLINE float& At(const uint& index)
    {
        uint x = index % resolution0().x;
        uint y = index / resolution0().x;
        return At(int2(x, y));
    }

    SOFTX_FORCE_INLINE const float& At(const uint& index) const
    {
        uint x = index % resolution0().x;
        uint y = index / resolution0().x;
        return At(int2(x, y));
    }

    SOFTX_FORCE_INLINE __m128 Read4(const uint2& coords) const
    {
        SOFTX_VERIFY(coords.x % 4 == 0);
        auto& lvl = mipChain[0];
        SOFTX_VERIFY(coords.x + 3u < lvl.widthPadded && coords.y < lvl.resolution.y);
        uint blockIdx = (coords.y * lvl.widthPadded + coords.x) / 4u;
        return lvl.blocks[blockIdx];
    }

    SOFTX_FORCE_INLINE void Write4(const uint2& coords, const __m128& depths, const __m128& mask)
    {
        SOFTX_VERIFY(coords.x % 4 == 0);
        auto& lvl = mipChain[0];
        SOFTX_VERIFY(coords.x + 3u < lvl.widthPadded && coords.y < lvl.resolution.y);
        uint blockIdx = (coords.y * lvl.widthPadded + coords.x) / 4u;
        __m128& block = lvl.blocks[blockIdx];
        block = _mm_or_ps(_mm_and_ps(depths, mask), _mm_andnot_ps(mask, block));
    }

    SOFTX_FORCE_INLINE __m128 Test4(const uint2& coords, const __m128& depth4, const __m128& activeMask) const
    {
        __m128 buffered = Read4(coords);
        __m128 passed = _mm_cmplt_ps(depth4, buffered);
        return _mm_and_ps(passed, activeMask);
    }

    SOFTX_FORCE_INLINE uint Width()  const { return resolution0().x; }
    SOFTX_FORCE_INLINE uint Height() const { return resolution0().y; }
    SOFTX_FORCE_INLINE uint WidthPadded() const { return widthPadded0(); }
    SOFTX_FORCE_INLINE uint2 Size() const { return resolution0(); }

    SOFTX_FORCE_INLINE float* Data() { return FloatPtr(); }
    SOFTX_FORCE_INLINE const float* Data() const { return FloatPtr(); }

    SOFTX_FORCE_INLINE uint MipCount() const { return (uint)mipChain.size(); }

    SOFTX_FORCE_INLINE uint2 MipSize(uint level) const
    {
        level = std::min(level, (uint)mipChain.size() - 1);
        return mipChain[level].resolution;
    }

    SOFTX_FORCE_INLINE uint MipWidth(uint level) const { return MipSize(level).x; }
    SOFTX_FORCE_INLINE uint MipHeight(uint level) const { return MipSize(level).y; }

    SOFTX_FORCE_INLINE float Read(const int2& coords, const uint& level) const
    {
        int2 sampleCoords = coords;
        uint mipLevel = std::min(level, (uint)mipChain.size() - 1);
        auto& lvl = mipChain[mipLevel];
        sampleCoords.x = AfterMath::clamp(sampleCoords.x, 0, (int)lvl.resolution.x - 1);
        sampleCoords.y = AfterMath::clamp(sampleCoords.y, 0, (int)lvl.resolution.y - 1);
        return lvl.blocks[(sampleCoords.y * lvl.widthPadded + sampleCoords.x) / 4].m128_f32[sampleCoords.x % 4];
    }

    SOFTX_FORCE_INLINE __m128 Read4(const uint2& coords, const uint& level) const
    {
        uint mipLevel = std::min(level, (uint)mipChain.size() - 1);
        auto& lvl = mipChain[mipLevel];
        uint blockIdx = (coords.y * lvl.widthPadded + coords.x) / 4u;
        return lvl.blocks[blockIdx];
    }

    void GenerateHiZ(HiZReduction mode = HiZReduction::Min)
    {
        for (uint m = 1; m < MipCount(); ++m)
        {
            const auto& prev = mipChain[m - 1];
            auto& curr = mipChain[m];
            uint nw = curr.resolution.x;
            uint nh = curr.resolution.y;
            uint pwp = prev.widthPadded;

            for (uint y = 0; y < nh; ++y)
            {
                for (uint x = 0; x < nw; ++x)
                {
                    uint px = x * 2;
                    uint py = y * 2;

                    float d00 = prev.blocks[(py * pwp + px) / 4].m128_f32[px % 4];
                    float d10 = prev.blocks[(py * pwp + px + 1) / 4].m128_f32[(px + 1) % 4];
                    float d01 = prev.blocks[((py + 1) * pwp + px) / 4].m128_f32[px % 4];
                    float d11 = prev.blocks[((py + 1) * pwp + px + 1) / 4].m128_f32[(px + 1) % 4];

                    float reduced;
                    if (mode == HiZReduction::Min)
                        reduced = std::min({ d00, d10, d01, d11 });
                    else
                        reduced = std::max({ d00, d10, d01, d11 });

                    uint idx = y * curr.widthPadded + x;
                    uint blockIdx = idx / 4;
                    uint comp = idx % 4;
                    curr.blocks[blockIdx].m128_f32[comp] = reduced;
                }
            }
        }
    }

private:
    std::vector<Level> mipChain;

    SOFTX_FORCE_INLINE const uint2& resolution0() const { return mipChain[0].resolution; }
    SOFTX_FORCE_INLINE uint widthPadded0() const { return mipChain[0].widthPadded; }

    SOFTX_FORCE_INLINE float* FloatPtr() { return reinterpret_cast<float*>(mipChain[0].blocks.data()); }
    SOFTX_FORCE_INLINE const float* FloatPtr() const { return reinterpret_cast<const float*>(mipChain[0].blocks.data()); }

    SOFTX_FORCE_INLINE static uint CalcMaxMips(uint w, uint h)
    {
        uint count = 1;
        while (w > 1 || h > 1)
        {
            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
            ++count;
        }
        return count;
    }
};

SOFTX_END
/////////////////////////////////////////////////////////////////
