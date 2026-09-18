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

    explicit DepthBuffer(uint2 size, uint numMips = 1)
    {
        assert(size.x % 2 == 0 && size.y % 2 == 0 && "DepthBuffer dimensions must be multiples of 2");

        uint maxMips = CalcMaxMips(size.x, size.y);
        uint actualMips = (numMips == 0) ? maxMips : std::min(numMips, maxMips);

        levels.resize(actualMips);

        uint totalBlocks = 0;
        uint w = size.x, h = size.y;
        for (uint i = 0; i < actualMips; ++i)
        {
            Level& lvl = levels[i];
            lvl.resolution = uint2(w, h);
            lvl.widthPadded = (w + 3) & ~3;
            lvl.offset = totalBlocks;
            totalBlocks += (lvl.widthPadded * h) / 4;
            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
        }

        blocks.resize(totalBlocks);
        Clear(1.0f);
    }

    DepthBuffer(const DepthBuffer& other) : levels(other.levels)
    {
        const size_t blockCount = other.blocks.size();
        blocks.resize(blockCount);

        const __m128* src = other.blocks.data();
        __m128* dst = blocks.data();

        for (size_t i = 0; i < blockCount; ++i) {
            _mm_stream_ps(reinterpret_cast<float*>(&dst[i]), src[i]);
        }
        _mm_sfence();
    }
    DepthBuffer& operator=(const DepthBuffer& other) = delete;

    SOFTX_FORCE_INLINE uint Width()  const { return levels[0].resolution.x; }
    SOFTX_FORCE_INLINE uint Height() const { return levels[0].resolution.y; }
    SOFTX_FORCE_INLINE uint WidthPadded() const { return levels[0].widthPadded; }
    SOFTX_FORCE_INLINE uint2 Size() const { return levels[0].resolution; }

    SOFTX_FORCE_INLINE float* Data() { return FloatPtr(); }
    SOFTX_FORCE_INLINE const float* Data() const { return FloatPtr(); }

    SOFTX_FORCE_INLINE uint MipCount() const { return static_cast<uint>(levels.size()); }

    SOFTX_FORCE_INLINE uint2 MipSize(uint level) const
    {
        level = std::min(level, static_cast<uint>(levels.size()) - 1);
        return levels[level].resolution;
    }

    SOFTX_FORCE_INLINE uint MipWidth(uint level) const { return MipSize(level).x; }
    SOFTX_FORCE_INLINE uint MipHeight(uint level) const { return MipSize(level).y; }

    void Clear(float depth)
    {
        __m128 depth4 = _mm_set1_ps(depth);
        __m128* ptr = blocks.data();
        size_t count = blocks.size();

        for (size_t i = 0; i < count; ++i)
            _mm_stream_ps(reinterpret_cast<float*>(ptr + i), depth4);
        _mm_sfence();
    }

    SOFTX_FORCE_INLINE float Read(int2 coords) const
    {
        SOFTX_VERIFY(coords.x >= 0 && coords.x < static_cast<int>(Width()));
        SOFTX_VERIFY(coords.y >= 0 && coords.y < static_cast<int>(Height()));
        return FloatPtr()[coords.y * WidthPadded() + coords.x];
    }

    SOFTX_FORCE_INLINE float Read(int2 coords, uint level) const
    {
        int2 sampleCoords = coords;
        uint mipLevel = std::min(level, static_cast<uint>(levels.size()) - 1);
        const Level& lvl = levels[mipLevel];
        sampleCoords.x = AfterMath::clamp(sampleCoords.x, 0, static_cast<int>(lvl.resolution.x) - 1);
        sampleCoords.y = AfterMath::clamp(sampleCoords.y, 0, static_cast<int>(lvl.resolution.y) - 1);

        const __m128* data = LevelData(mipLevel);
        return data[(sampleCoords.y * lvl.widthPadded + sampleCoords.x) / 4].m128_f32[sampleCoords.x % 4];
    }

    SOFTX_FORCE_INLINE void Write(int2 coords, float depth)
    {
        SOFTX_VERIFY(coords.x >= 0 && coords.x < static_cast<int>(Width()));
        SOFTX_VERIFY(coords.y >= 0 && coords.y < static_cast<int>(Height()));
        FloatPtr()[coords.y * WidthPadded() + coords.x] = depth;
    }

    SOFTX_FORCE_INLINE float& At(int2 coords)
    {
        SOFTX_VERIFY(coords.x >= 0 && coords.x < static_cast<int>(Width()));
        SOFTX_VERIFY(coords.y >= 0 && coords.y < static_cast<int>(Height()));
        return FloatPtr()[coords.y * WidthPadded() + coords.x];
    }

    SOFTX_FORCE_INLINE const float& At(int2 coords) const
    {
        SOFTX_VERIFY(coords.x >= 0 && coords.x < static_cast<int>(Width()));
        SOFTX_VERIFY(coords.y >= 0 && coords.y < static_cast<int>(Height()));
        return FloatPtr()[coords.y * WidthPadded() + coords.x];
    }

    SOFTX_FORCE_INLINE float& At(uint index)
    {
        SOFTX_VERIFY(index < Width()* Height());
        uint x = index % Size().x;
        uint y = index / Size().x;
        return At(int2(static_cast<int>(x), static_cast<int>(y)));
    }

    SOFTX_FORCE_INLINE const float& At(uint index) const
    {
        SOFTX_VERIFY(index < Width()* Height());
        uint x = index % Size().x;
        uint y = index / Size().x;
        return At(int2(static_cast<int>(x), static_cast<int>(y)));
    }

    SOFTX_FORCE_INLINE __m128 Read4(uint2 coords) const
    {
        SOFTX_VERIFY(coords.x % 4 == 0);
        const Level& lvl = levels[0];

        SOFTX_VERIFY(coords.x < lvl.resolution.x);
        SOFTX_VERIFY(coords.x + 3u < lvl.resolution.x);
        SOFTX_VERIFY(coords.x + 3u < lvl.widthPadded && coords.y < lvl.resolution.y);

        uint blockIdx = (coords.y * lvl.widthPadded + coords.x) / 4u;
        return LevelData(0)[blockIdx];
    }

    SOFTX_FORCE_INLINE __m128 Read4(uint2 coords, uint level) const
    {
        uint mipLevel = std::min(level, static_cast<uint>(levels.size()) - 1);
        const Level& lvl = levels[mipLevel];
        SOFTX_VERIFY(coords.x % 4 == 0);

        SOFTX_VERIFY(coords.x < lvl.resolution.x);
        SOFTX_VERIFY(coords.x + 3u < lvl.resolution.x);
        SOFTX_VERIFY(coords.x + 3u < lvl.widthPadded && coords.y < lvl.resolution.y);

        uint blockIdx = (coords.y * lvl.widthPadded + coords.x) / 4u;
        return LevelData(mipLevel)[blockIdx];
    }

    SOFTX_FORCE_INLINE void Write4(uint2 coords, __m128 depths, __m128 mask)
    {
        SOFTX_VERIFY(coords.x % 4 == 0);
        const Level& lvl = levels[0];

        SOFTX_VERIFY(coords.x < lvl.resolution.x);
        SOFTX_VERIFY(coords.x + 3u < lvl.resolution.x);
        SOFTX_VERIFY(coords.x + 3u < lvl.widthPadded && coords.y < lvl.resolution.y);

        uint blockIdx = (coords.y * lvl.widthPadded + coords.x) / 4u;
        __m128& block = LevelData(0)[blockIdx];
        block = _mm_or_ps(_mm_and_ps(depths, mask), _mm_andnot_ps(mask, block));
    }

    SOFTX_FORCE_INLINE __m128 Compare4(uint2 coords, __m128 depth4, __m128 activeMask) const
    {
        __m128 buffered = Read4(coords);
        __m128 passed = _mm_cmplt_ps(depth4, buffered);
        return _mm_and_ps(passed, activeMask);
    }

    void GenerateHiZ(HiZReduction mode = HiZReduction::Min)
    {
        for (uint m = 1; m < MipCount(); ++m)
        {
            const Level& prev = levels[m - 1];
            const Level& curr = levels[m];
            const __m128* prevData = LevelData(m - 1);
            __m128* currData = LevelData(m);

            uint nw = curr.resolution.x;
            uint nh = curr.resolution.y;
            uint pwp = prev.widthPadded;

            for (uint y = 0; y < nh; ++y)
            {
                for (uint x = 0; x < nw; ++x)
                {
                    uint px = x * 2;
                    uint py = y * 2;

                    float d00 = prevData[(py * pwp + px) / 4].m128_f32[px % 4];
                    float d10 = prevData[(py * pwp + px + 1) / 4].m128_f32[(px + 1) % 4];
                    float d01 = prevData[((py + 1) * pwp + px) / 4].m128_f32[px % 4];
                    float d11 = prevData[((py + 1) * pwp + px + 1) / 4].m128_f32[(px + 1) % 4];

                    float reduced = (mode == HiZReduction::Min)
                        ? std::min({ d00, d10, d01, d11 })
                        : std::max({ d00, d10, d01, d11 });

                    uint idx = y * curr.widthPadded + x;
                    currData[idx / 4].m128_f32[idx % 4] = reduced;
                }
            }
        }
    }

private:
    struct Level
    {
        uint2 resolution;
        uint widthPadded;
        uint offset;
    };

    std::vector<__m128> blocks;
    std::vector<Level> levels;

    SOFTX_FORCE_INLINE float* FloatPtr() { return reinterpret_cast<float*>(blocks.data()); }
    SOFTX_FORCE_INLINE const float* FloatPtr() const { return reinterpret_cast<const float*>(blocks.data()); }

    SOFTX_FORCE_INLINE __m128* LevelData(uint level) { return blocks.data() + levels[level].offset; }
    SOFTX_FORCE_INLINE const __m128* LevelData(uint level) const { return blocks.data() + levels[level].offset; }

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
