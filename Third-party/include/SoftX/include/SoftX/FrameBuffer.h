#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <windows.h>
#include <xmmintrin.h>
#include <emmintrin.h>

#include "RenderTargetInterface.h"
#include "ThirdPartyIncluding.h"
#include "ThreadPoolManager.h"

SOFTX_BEGIN

class SOFTX_API Framebuffer : public IRenderTarget
{
public:
    explicit Framebuffer(uint2 size) : resolution(size), pixelsStorage(size.x * size.y, 0)
    {
    }

    uint32_t* GetRawPixels() { return pixelsStorage.data(); }
    const uint32_t* GetRawPixels() const { return pixelsStorage.data(); }

    static uint32_t PackColor(const float4& c)
    {
        auto toByte = [](float f) -> uint8_t {
            int v = int(f * 255.0f + 0.5f);
            return uint8_t(clamp(v, 0, 255));
        };
        uint8_t r = toByte(c.x);
        uint8_t g = toByte(c.y);
        uint8_t b = toByte(c.z);
        uint8_t a = toByte(c.w);
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    static __m128 UnpackColor(uint32_t bgra)
    {
        uint8_t r = (bgra >> 16) & 0xFF;
        uint8_t g = (bgra >> 8) & 0xFF;
        uint8_t b = (bgra >> 0) & 0xFF;
        uint8_t a = (bgra >> 24) & 0xFF;
        const float inv255 = 1.0f / 255.0f;
        return _mm_set_ps(a * inv255, b * inv255, g * inv255, r * inv255);
    }

    void Clear(const float4& color) override
    {
        uint32_t bg = PackColor(color);
        size_t count = pixelsStorage.size();
        size_t i = 0;

        __m128i bg4 = _mm_set1_epi32(bg);
        for (; i + 4 <= count; i += 4)
        {
            _mm_stream_si128(reinterpret_cast<__m128i*>(pixelsStorage.data() + i), bg4);
        }
        _mm_sfence();

        for (; i < count; ++i)
        {
            pixelsStorage[i] = bg;
        }
    }

    void SetPixel(uint2 coords, const float4& color) override
    {
        uint index = coords.y * resolution.x + coords.x;
        pixelsStorage[index] = PackColor(color);
    }

    __m128 Read(uint2 coords) const
    {
        uint32_t bg = pixelsStorage[coords.y * resolution.x + coords.x];
        return UnpackColor(bg);
    }

    uint Width() const override { return resolution.x; }
    uint Height() const override { return resolution.y; }
    uint2 Size() const override { return resolution; }

    void Present(HDC hdc, int2 dstPos, int2 dstSize) const
    {
        PROFILE_SCOPE("Present framebuffer");

        int dstW = (dstSize.x == -1) ? (int)resolution.x : dstSize.x;
        int dstH = (dstSize.y == -1) ? (int)resolution.y : dstSize.y;

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = (LONG)resolution.x;
        bmi.bmiHeader.biHeight      = -(LONG)resolution.y;   // top-down
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        SetDIBitsToDevice(hdc,
                          dstPos.x, dstPos.y, 
                          dstW, dstH,
                          0, 0, 
                          0, 
                          resolution.y,
                          pixelsStorage.data(),
                          &bmi,
                          DIB_RGB_COLORS);
    }

    // Сохранение в TGA (без преобразований, данные уже в BGRA)
    bool SaveTGA(const char* filename) const
    {
        std::ofstream file(filename, std::ios::binary);
        if (!file)
            return false;

        uint8_t header[18] = {0};
        header[2] = 2;                           // Uncompressed true-color
        header[12] = resolution.x & 0xFF;        // width low byte
        header[13] = (resolution.x >> 8) & 0xFF; // width high byte
        header[14] = resolution.y & 0xFF;        // height low byte
        header[15] = (resolution.y >> 8) & 0xFF; // height high byte
        header[16] = 32;                         // bits per pixel (RGBA)
        header[17] = 8 | (1 << 5);               // 8 bits alpha, origin top-left
        file.write(reinterpret_cast<const char*>(header), 18);

        file.write(reinterpret_cast<const char*>(pixelsStorage.data()), pixelsStorage.size() * 4);
        file.close();
        return true;
    }

private:
    uint2 resolution;
    std::vector<uint32_t> pixelsStorage;   // BGRA
};

SOFTX_END
