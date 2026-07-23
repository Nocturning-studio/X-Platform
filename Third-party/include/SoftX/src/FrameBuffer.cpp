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

// Inspired by Onigiri :D
// https://www.youtube.com/watch?v=n4zUgtDk95w
// https://github.com/ArtemOnigiri/Console3D/blob/main/ConsoleRayTracing.cpp
void FrameBuffer::PresentASCII(HANDLE hConsole, uint2 consoleSize)
{
    std::lock_guard<std::mutex> lock(mutex);
    PROFILE_SCOPE("FrameBuffer::PresentASCII");

    if (!hConsole || consoleSize.x == 0 || consoleSize.y == 0)
        return;

    uint srcW = resolution.x;
    uint srcH = resolution.y;
    uint dstW = consoleSize.x;
    uint dstH = consoleSize.y;

    static const char gradient[] = " .:!/r(l1Z4H9W8$@";
    static const int gradientSize = static_cast<int>(std::size(gradient)) - 2;

    std::vector<char> charBuffer(dstW * dstH);

    float scaleX = static_cast<float>(srcW) / dstW;
    float scaleY = static_cast<float>(srcH) / dstH;

    const uint totalPixels = dstW * dstH;
    std::atomic<uint> pixelIndex(0);

    auto Task = [&]()
    {
        while (true)
        {
            uint idx = pixelIndex.fetch_add(1);
            if (idx >= totalPixels) break;

            uint y = idx / dstW;
            uint x = idx % dstW;

            uint srcYStart = static_cast<uint>(y * scaleY);
            uint srcYEnd = (y == dstH - 1) ? srcH : static_cast<uint>((y + 1) * scaleY);
            if (srcYEnd == 0) srcYEnd = 1;

            uint srcXStart = static_cast<uint>(x * scaleX);
            uint srcXEnd = (x == dstW - 1) ? srcW : static_cast<uint>((x + 1) * scaleX);
            if (srcXEnd == 0) srcXEnd = 1;

            float rSum = 0, gSum = 0, bSum = 0;
            uint samples = 0;

            for (uint sy = srcYStart; sy < srcYEnd && sy < srcH; ++sy)
            {
                for (uint sx = srcXStart; sx < srcXEnd && sx < srcW; ++sx)
                {
                    __m128 color = Read(uint2(sx, sy));
                    float rgba[4];
                    _mm_storeu_ps(rgba, color);
                    rSum += rgba[0];
                    gSum += rgba[1];
                    bSum += rgba[2];
                    ++samples;
                }
            }

            if (samples > 0)
            {
                rSum /= samples;
                gSum /= samples;
                bSum /= samples;

                float luminance = 0.2126f * rSum + 0.7152f * gSum + 0.0722f * bSum;
                luminance = AfterMath::clamp(luminance, 0.0f, 1.0f);

                int gradIdx = static_cast<int>(luminance * gradientSize);
                gradIdx = AfterMath::clamp(gradIdx, 0, gradientSize);
                charBuffer[idx] = gradient[gradIdx];
            }
            else
            {
                charBuffer[idx] = ' ';
            }
        }
    };
    ThreadUtils::DispatchWorkers(Task);

    DWORD written = 0;
    COORD coord = { 0, 0 };
    WriteConsoleOutputCharacterA(hConsole, charBuffer.data(), static_cast<DWORD>(totalPixels), coord, &written);
}

void FrameBuffer::PresentBitmap(HDC hdc, int2 dstPos, int2 dstSize)
{
    std::lock_guard<std::mutex> lock(mutex);
    PROFILE_SCOPE("FrameBuffer::PresentBitmap");

    int dstW = (dstSize.x == -1) ? (int)resolution.x : dstSize.x;
    int dstH = (dstSize.y == -1) ? (int)resolution.y : dstSize.y;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)resolution.x;
    bmi.bmiHeader.biHeight = -(LONG)resolution.y;   // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
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

bool FrameBuffer::SaveTGA(const char* filename)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::ofstream file(filename, std::ios::binary);
    if (!file)
        return false;

    uint8_t header[18] = { 0 };
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

SOFTX_END
/////////////////////////////////////////////////////////////////
