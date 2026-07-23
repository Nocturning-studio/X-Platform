/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <algorithm>
#include <mutex>
#include <cstdint>
#include <vector>
#include <windows.h>
#include <xmmintrin.h>
#include <emmintrin.h>

#include "RenderTargetInterface.h"
#include "ThirdPartyIncluding.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API FrameBuffer : public IRenderTarget
{
public:
    explicit FrameBuffer(uint2 size) : resolution(size), pixelsStorage(size.x * size.y, 0)
    {
    }

    uint32_t* GetRawPixels() { return pixelsStorage.data(); }
    const uint32_t* GetRawPixels() const { return pixelsStorage.data(); }

    static uint32_t PackColor(float4 c)
    {
        auto toByte = [](float f) -> uint8_t {
            int v = int(f * 255.0f + 0.5f);
            return uint8_t(AfterMath::clamp(v, 0, 255));
        };
        uint8_t r = toByte(c.x);
        uint8_t g = toByte(c.y);
        uint8_t b = toByte(c.z);
        uint8_t a = toByte(c.w);
        return static_cast<uint32_t>((a << 24) | (r << 16) | (g << 8) | b);
    }

    SOFTX_FORCE_INLINE static __m128 UnpackColor(uint32_t bgra)
    {
        uint8_t r = (bgra >> 16) & 0xFF;
        uint8_t g = (bgra >> 8) & 0xFF;
        uint8_t b = (bgra >> 0) & 0xFF;
        uint8_t a = (bgra >> 24) & 0xFF;
        const float inv255 = 1.0f / 255.0f;
        return _mm_set_ps(a * inv255, b * inv255, g * inv255, r * inv255);
    }

    void SetPixel(uint2 coords, const float4& color) override
    {
        uint index = coords.y * resolution.x + coords.x;
        pixelsStorage[index] = PackColor(color);
    }

    SOFTX_FORCE_INLINE __m128 Read(uint2 coords) const
    {
        uint32_t bg = pixelsStorage[coords.y * resolution.x + coords.x];
        return UnpackColor(bg);
    }

    void Clear(const float4& color) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        uint32_t bg = PackColor(color);
        size_t count = pixelsStorage.size();
        size_t i = 0;

        const __m128i bg4 = _mm_set1_epi32(static_cast<int>(bg));
        for (; i + 4 <= count; i += 4)
        {
            std::memcpy(&pixelsStorage[i], &bg4, sizeof(bg4));
        }

        for (; i < count; ++i)
        {
            pixelsStorage[i] = bg;
        }
    }

    SOFTX_FORCE_INLINE uint Width() const override { return resolution.x; }
    SOFTX_FORCE_INLINE uint Height() const override { return resolution.y; }
    SOFTX_FORCE_INLINE uint2 Size() const override { return resolution; }

    void PresentBitmap(HDC hdc, int2 dstPos, int2 dstSize);
    void PresentASCII(HANDLE hConsole, uint2 consoleSize);

    bool SaveTGA(const char* filename);

private:
    uint2 resolution;
    std::vector<uint32_t> pixelsStorage;   // BGRA
    std::mutex mutex;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
