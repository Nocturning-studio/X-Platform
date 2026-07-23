/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include <fstream>
#include <iostream>

#include "ThreadPoolManager.h"
#include "../include/SoftX.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

void TextureRGBA32F::GenerateMips()
{
    const Level& baseLevel = levels[0];
    uint w = baseLevel.resolution.x;
    uint h = baseLevel.resolution.y;

    std::vector<Level> newLevels;
    std::vector<__m128> newPixels;

    uint basePixelCount = w * h;
    newPixels.resize(basePixelCount);
    const __m128* srcBase = pixels.data() + baseLevel.offset;
    std::copy(srcBase, srcBase + basePixelCount, newPixels.begin());

    Level lvl0;
    lvl0.resolution = uint2(w, h);
    lvl0.offset = 0;
    newLevels.push_back(lvl0);

    uint totalPixels = basePixelCount;

    while (w > 1 || h > 1)
    {
        uint nw = std::max(1u, w / 2);
        uint nh = std::max(1u, h / 2);
        uint levelSize = nw * nh;

        newPixels.resize(totalPixels + levelSize);

        const Level& prevLevel = newLevels.back();
        const __m128* prevData = newPixels.data() + prevLevel.offset;

        __m128* currData = newPixels.data() + totalPixels;

        for (uint y = 0; y < nh; ++y)
        {
            for (uint x = 0; x < nw; ++x)
            {
                uint base = 2 * y * w + 2 * x;
                __m128 a = prevData[base];
                __m128 b = prevData[base + 1];
                __m128 c = prevData[base + w];
                __m128 d = prevData[base + w + 1];

                __m128 sum = _mm_add_ps(
                    _mm_add_ps(a, b),
                    _mm_add_ps(c, d));
                currData[y * nw + x] = _mm_mul_ps(sum, _mm_set1_ps(0.25f));
            }
        }

        Level lvl;
        lvl.resolution = uint2(nw, nh);
        lvl.offset = totalPixels;
        newLevels.push_back(lvl);

        totalPixels += levelSize;
        w = nw;
        h = nh;
    }

    levels = std::move(newLevels);
    pixels = std::move(newPixels);
}

void TextureRGBA32F::SaveToTGA(const char* filename) const
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
    if (!file) {
        std::cerr << "Cannot open file for writing: " << filename << std::endl;
        return;
    }
    file.write(reinterpret_cast<const char*>(header), 18);

    for (uint y = 0; y < h; ++y) {
        for (uint x = 0; x < w; ++x) {
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
