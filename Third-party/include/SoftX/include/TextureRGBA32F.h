/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
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
    explicit TextureRGBA32F(uint2 size, uint numMips = 1): resolution(size), mipChain(std::max(1u, numMips))
    {
        __m128 zero = _mm_setzero_ps();
        for (auto& level : mipChain) {
            const uint mipIndex = static_cast<uint>(&level - mipChain.data());
            const uint w = MipWidth(mipIndex);
            const uint h = MipHeight(mipIndex);
            level.resize(w * h);
            for (auto& p : level)
                _mm_store_ps(reinterpret_cast<float*>(&p), zero);
        }
    }

    SOFTX_FORCE_INLINE __m128* GetRawPixels(uint level = 0) {
        level = std::min(level, (uint)mipChain.size() - 1);
        return mipChain[level].data();
    }
    SOFTX_FORCE_INLINE const __m128* GetRawPixels(uint level = 0) const {
        level = std::min(level, (uint)mipChain.size() - 1);
        return mipChain[level].data();
    }

    SOFTX_FORCE_INLINE __m128 Read(const uint2& coords) const {
        assert(coords.x < resolution.x&& coords.y < resolution.y);
        return mipChain[0][coords.y * resolution.x + coords.x];
    }

    SOFTX_FORCE_INLINE __m128 Read(const uint& index) const {
        assert(index < (uint)mipChain[0].size());
        return mipChain[0][index];
    }

    SOFTX_FORCE_INLINE __m128 SampleRaw(const float2& uv) const {
        uint x = (uint)(uv.x * resolution.x);
        uint y = (uint)(uv.y * resolution.y);
        if (x >= resolution.x) x = resolution.x - 1;
        if (y >= resolution.y) y = resolution.y - 1;
        return mipChain[0][y * resolution.x + x];
    }

    SOFTX_FORCE_INLINE float4 Sample(const float2& uv) const override {
        __m128 color = SampleRaw(uv);
        return float4(color);
    }

    SOFTX_FORCE_INLINE __m128 FetchRaw(const int& x, const int& y) const override {
        int2 coords = int2(x, y);
        coords.x = AfterMath::clamp(x, 0, (int)resolution.x - 1);
        coords.y = AfterMath::clamp(y, 0, (int)resolution.y - 1);
        return mipChain[0][uint(coords.y) * resolution.x + uint(coords.x)];
    }

    SOFTX_FORCE_INLINE __m128 SampleBilinearRaw(float2 uv) const {
        return SampleBilinearRaw(uv, 0);
    }

    SOFTX_FORCE_INLINE float4 SampleBilinear(const float2& uv) const override {
        return float4(SampleBilinearRaw(uv));
    }

    SOFTX_FORCE_INLINE void StreamWrite(uint2 coords, __m128 color) {
        StreamWrite(coords, color, 0);
    }

    SOFTX_FORCE_INLINE void StreamWrite(uint index, __m128 color) {
        assert(index < (uint)mipChain[0].size());
        _mm_stream_ps(reinterpret_cast<float*>(&mipChain[0][index]), color);
    }

    SOFTX_FORCE_INLINE uint MipCount() const override {
        return (uint)mipChain.size();
    }

    SOFTX_FORCE_INLINE uint MipWidth(uint level) const {
        level = std::min(level, (uint)mipChain.size() - 1);
        return std::max(1u, resolution.x >> level);
    }

    SOFTX_FORCE_INLINE uint MipHeight(uint level) const {
        level = std::min(level, (uint)mipChain.size() - 1);
        return std::max(1u, resolution.y >> level);
    }

    SOFTX_FORCE_INLINE __m128 FetchRaw(const int& x, const int& y, const uint& level) const override {
        uint mipLevel = std::min(level, (uint)mipChain.size() - 1);
        uint w = MipWidth(mipLevel);
        uint h = MipHeight(mipLevel);
        int2 coords = int2(x, y);
        coords.x = AfterMath::clamp(x, 0, (int)w - 1);
        coords.y = AfterMath::clamp(y, 0, (int)h - 1);
        return mipChain[mipLevel][uint(coords.y) * w + uint(coords.x)];
    }

    SOFTX_FORCE_INLINE __m128 SampleBilinearRaw(const float2& uv, const uint& level) const {
        uint mipLevel = std::min(level, (uint)mipChain.size() - 1);
        uint w = MipWidth(mipLevel);
        uint h = MipHeight(mipLevel);

        float fx = uv.x * w - 0.5f;
        float fy = uv.y * h - 0.5f;

        int x0 = (int)std::floor(fx);
        int y0 = (int)std::floor(fy);

        float tx = fx - x0;
        float ty = fy - y0;

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

        return _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(c00, w00), _mm_mul_ps(c10, w10)),
            _mm_add_ps(_mm_mul_ps(c01, w01), _mm_mul_ps(c11, w11)));
    }

    SOFTX_FORCE_INLINE float4 SampleLevel(const float2& uv, const float& lod) const override {
        uint level = (uint)(lod + 0.5f);  // nearest mip
        level = std::max(0u, std::min(level, (uint)mipChain.size() - 1));
        return float4(SampleBilinearRaw(uv, level));
    }

    SOFTX_FORCE_INLINE void StreamWrite(const uint2& coords, const __m128& color, const uint& level) {
        uint mipLevel = std::min(level, (uint)mipChain.size() - 1);
        uint w = MipWidth(mipLevel);
        uint index = coords.y * w + coords.x;
        _mm_stream_ps(reinterpret_cast<float*>(&mipChain[mipLevel][index]), color);
    }

    void GenerateMips() {
        mipChain.resize(1);
        uint w = resolution.x;
        uint h = resolution.y;

        while (w > 1 || h > 1) {
            uint nw = std::max(1u, w / 2);
            uint nh = std::max(1u, h / 2);
            std::vector<__m128> level(nw * nh);
            const auto& prev = mipChain.back();

            for (uint y = 0; y < nh; ++y) {
                for (uint x = 0; x < nw; ++x) {
                    uint base = 2 * y * w + 2 * x;
                    __m128 a = prev[base];
                    __m128 b = prev[base + 1];
                    __m128 c = prev[base + w];
                    __m128 d = prev[base + w + 1];

                    __m128 sum = _mm_add_ps(
                        _mm_add_ps(a, b),
                        _mm_add_ps(c, d));
                    level[y * nw + x] = _mm_mul_ps(sum, _mm_set1_ps(0.25f));
                }
            }
            mipChain.push_back(std::move(level));
            w = nw;
            h = nh;
        }
    }

    SOFTX_FORCE_INLINE uint Width() const override {
        return resolution.x;
    }

    SOFTX_FORCE_INLINE uint Height() const override {
        return resolution.y;
    }

    void SaveToTGA(const char* filename) const {
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

private:
    uint2 resolution;
    std::vector<std::vector<__m128>> mipChain;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
