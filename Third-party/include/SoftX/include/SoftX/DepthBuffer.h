#pragma once

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>
#include <xmmintrin.h>

#include "LibInternal.h"
#include "ThirdPartyIncluding.h"

SOFTX_BEGIN

class SOFTX_API DepthBuffer
{
public:
    explicit DepthBuffer(uint2 size)
        : resolution(size),
          widthPadded((size.x + 3) & ~3), // Round up to multiple of 4
          blocks(widthPadded * size.y / 4)
    {
        Clear(1.0f);
    }

    // Clear the entire depth buffer with a single value
    void Clear(float depth)
    {
        __m128 depth4 = _mm_set1_ps(depth);
        __m128* ptr = blocks.data();
        size_t count = blocks.size();

        size_t i = 0;
        // Streaming write – bypass cache, maximum speed
        for (; i < count; ++i)
            _mm_stream_ps(reinterpret_cast<float*>(ptr + i), depth4);
        _mm_sfence();
    }

    // Scalar access (backward compatibility)
    float Read(int2 coords) const
    {
        return FloatPtr()[coords.y * widthPadded + coords.x];
    }

    void Write(int2 coords, float depth)
    {
        FloatPtr()[coords.y * widthPadded + coords.x] = depth;
    }

    float& At(int2 coords)
    {
        return FloatPtr()[coords.y * widthPadded + coords.x];
    }

    const float& At(int2 coords) const
    {
        return FloatPtr()[coords.y * widthPadded + coords.x];
    }

    // Linear index access (using original width)
    float& At(uint index)
    {
        uint x = index % resolution.x;
        uint y = index / resolution.x;
        return At(int2(x, y));
    }

    const float& At(uint index) const
    {
        uint x = index % resolution.x;
        uint y = index / resolution.x;
        return At(int2(x, y));
    }

    // SIMD block access
    // Reads 4 depth values starting from coords (horizontally).
    // coords.x must be a multiple of 4 – used in SSE rasterizer.
    __m128 Read4(uint2 coords) const
    {
        assert(coords.x % 4 == 0);
        assert(coords.x + 3u < widthPadded && coords.y < resolution.y);
        uint blockIdx = (coords.y * widthPadded + coords.x) / 4u;
        return blocks[blockIdx];
    }

    // Writes 4 depth values with mask (1 = overwrite).
    // Used in SSE rasterizer after depth test.
    void Write4(uint2 coords, __m128 depths, __m128 mask)
    {
        assert(coords.x % 4 == 0);
        assert(coords.x + 3u < widthPadded && coords.y < resolution.y);
        uint blockIdx = (coords.y * widthPadded + coords.x) / 4u;
        __m128& block = blocks[blockIdx];

        // Write only pixels where mask = 0xFFFFFFFF
        // block = (depths & mask) | (block & ~mask)
        block = _mm_or_ps(_mm_and_ps(depths, mask), _mm_andnot_ps(mask, block));
    }

    // Depth test for 4 pixels: returns mask of passing pixels (z < buffer).
    // depth4 – interpolated z, activeMask – tile coverage mask.
    __m128 Test4(uint2 coords, __m128 depth4, __m128 activeMask) const
    {
        __m128 buffered = Read4(coords);
        __m128 passed = _mm_cmplt_ps(depth4, buffered); // z < bufferZ
        return _mm_and_ps(passed, activeMask);
    }

    // Getters
    uint Width() const
    {
        return resolution.x;
    }

    uint Height() const
    {
        return resolution.y;
    }

    uint WidthPadded() const
    {
        return widthPadded;
    }

    uint2 Size() const
    {
        return resolution;
    }

    // Raw float* access – for scalar rasterizer
    float* Data()
    {
        return FloatPtr();
    }

    const float* Data() const
    {
        return FloatPtr();
    }

private:
    float* FloatPtr()
    {
        return reinterpret_cast<float*>(blocks.data());
    }

    const float* FloatPtr() const
    {
        return reinterpret_cast<const float*>(blocks.data());
    }

    uint2 resolution;
    uint widthPadded;           // multiple of 4 – for block alignment
    std::vector<__m128> blocks; // each block = 4 float depths
};

SOFTX_END
