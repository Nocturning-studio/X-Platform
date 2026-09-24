/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include <fstream>
#include <iostream>

#include "../include/SoftX.h"
#include "ThreadUtils.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

void Texture::Clear(float4 color, uint level)
{
    level = std::min(level, mipCount - 1);
    const Level& lvl = levels[level];
    size_t levelPixels = static_cast<size_t>(lvl.resolution.x) * lvl.resolution.y;
    __m128* dest = pixels + lvl.offset;
    __m128 col = color.simd_;

    const uint32_t numThreads = ThreadPoolManager::Get().threadCount();
    if (numThreads <= 1 || levelPixels < 64 * 64)
    {
        for (size_t i = 0; i < levelPixels; ++i)
            _mm_stream_ps(reinterpret_cast<float*>(dest + i), col);
    }
    else
    {
        std::atomic<size_t> next(0);
        for (uint32_t t = 0; t < numThreads; ++t)
        {
            ThreadPoolManager::Get().enqueue([&]() 
                {
                size_t i;
                while ((i = next.fetch_add(1024)) < levelPixels)
                {
                    size_t end = std::min(i + 1024, levelPixels);
                    for (; i < end; ++i)
                        _mm_stream_ps(reinterpret_cast<float*>(dest + i), col);
                }
                });
        }
        ThreadPoolManager::Get().wait();
    }
    _mm_sfence();
}

void Texture::GenerateMips()
{
    if (mipCount <= 1)
        return;

    const Level& baseLevel = levels[0];
    uint w = baseLevel.resolution.x;
    uint h = baseLevel.resolution.y;

    uint totalPixelsNeeded = w * h;
    uint tmpW = w, tmpH = h;
    while (tmpW > 1 || tmpH > 1)
    {
        tmpW = std::max(1u, tmpW / 2);
        tmpH = std::max(1u, tmpH / 2);
        totalPixelsNeeded += tmpW * tmpH;
    }

    __m128* newPixels = new __m128[totalPixelsNeeded];
    Level* newLevels = new Level[mipCount];

    uint basePixelCount = w * h;
    std::copy(pixels + baseLevel.offset, pixels + baseLevel.offset + basePixelCount, newPixels);

    newLevels[0].resolution = uint2(w, h);
    newLevels[0].offset = 0;

    uint currentOffset = basePixelCount;
    uint levelIndex = 1;
    uint prevW = w, prevH = h;
    const __m128* prevData = newPixels;

    while (prevW > 1 || prevH > 1)
    {
        uint nw = std::max(1u, prevW / 2);
        uint nh = std::max(1u, prevH / 2);

        for (uint y = 0; y < nh; ++y)
        {
            for (uint x = 0; x < nw; ++x)
            {
                uint base = 2 * y * prevW + 2 * x;
                __m128 a = prevData[base];
                __m128 b = prevData[base + 1];
                __m128 c = prevData[base + prevW];
                __m128 d = prevData[base + prevW + 1];

                __m128 sum = _mm_add_ps(
                    _mm_add_ps(a, b),
                    _mm_add_ps(c, d));
                newPixels[currentOffset + y * nw + x] = _mm_mul_ps(sum, _mm_set1_ps(0.25f));
            }
        }

        newLevels[levelIndex].resolution = uint2(nw, nh);
        newLevels[levelIndex].offset = currentOffset;

        currentOffset += nw * nh;
        prevW = nw;
        prevH = nh;
        prevData = newPixels + currentOffset - nw * nh;
        ++levelIndex;
    }

    delete[] pixels;
    delete[] levels;
    pixels = newPixels;
    pixelCount = totalPixelsNeeded;
    levels = newLevels;
}

void Texture::SaveToTGA(const char* filename) const
{
    uint w = Width();
    uint h = Height();

    uint8_t header[18] = { 0 };
    header[2] = 2;
    header[12] = w & 0xFF;
    header[13] = (w >> 8) & 0xFF;
    header[14] = h & 0xFF;
    header[15] = (h >> 8) & 0xFF;
    header[16] = 32;
    header[17] = 8 | (1 << 5);

    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << "Cannot open file for writing: " << filename << std::endl;
        return;
    }
    file.write(reinterpret_cast<const char*>(header), 18);

    for (uint y = 0; y < h; ++y)
    {
        for (uint x = 0; x < w; ++x)
        {
            __m128 color = Read(uint2(x, y));
            float rgba[4];
            _mm_storeu_ps(rgba, color);
            uint8_t b = static_cast<uint8_t>(AfterMath::clamp(rgba[2] * 255.0f, 0.0f, 255.0f));
            uint8_t g = static_cast<uint8_t>(AfterMath::clamp(rgba[1] * 255.0f, 0.0f, 255.0f));
            uint8_t r = static_cast<uint8_t>(AfterMath::clamp(rgba[0] * 255.0f, 0.0f, 255.0f));
            uint8_t a = static_cast<uint8_t>(AfterMath::clamp(rgba[3] * 255.0f, 0.0f, 255.0f));
            uint8_t pixel[4] = { b, g, r, a };
            file.write(reinterpret_cast<const char*>(pixel), 4);
        }
    }
    file.close();
    std::cout << "Texture saved to " << filename << std::endl;
}

SOFTX_END
/////////////////////////////////////////////////////////////////
