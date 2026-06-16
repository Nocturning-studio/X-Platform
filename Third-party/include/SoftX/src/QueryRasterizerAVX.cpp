/////////////////////////////////////////////////////////////////
// SoftX – Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "pch.h"

#include <SoftX.h>
#include "RasterizerCommon.h"
#include "QueryRasterizerAVX.h"

#include <immintrin.h>
#include <nmmintrin.h>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

uint QueryRasterizerAVX::RasterizeTriangle(
    const VertexOutput& v0,
    const VertexOutput& v1,
    const VertexOutput& v2,
    const RasterizerState& state,
    DepthBuffer& depthBuffer,
    const ConstantBuffer& /*cb*/,
    uint2 tileMin,
    uint2 tileMax)
{
    float minX = std::min({ v0.Position.x, v1.Position.x, v2.Position.x });
    float maxX = std::max({ v0.Position.x, v1.Position.x, v2.Position.x });
    float minY = std::min({ v0.Position.y, v1.Position.y, v2.Position.y });
    float maxY = std::max({ v0.Position.y, v1.Position.y, v2.Position.y });

    int iMinX = std::max((int)tileMin.x, (int)std::floor(minX));
    int iMaxX = std::min((int)tileMax.x, (int)std::ceil(maxX));
    int iMinY = std::max((int)tileMin.y, (int)std::floor(minY));
    int iMaxY = std::min((int)tileMax.y, (int)std::ceil(maxY));

    if (iMinX > iMaxX || iMinY > iMaxY) UNLIKELY
        return 0;

    float area2 = RasterizerCommon::EdgeFunction(v0.Position, v1.Position, v2.Position);
    const CullMode cull = state.cullMode;
    if (cull == CullMode::Back && area2 < 0) return 0;
    if (cull == CullMode::Front && area2 > 0) return 0;
    if (std::abs(area2) < 1e-6f) UNLIKELY return 0;

    float4 dx01 = v1.Position - v0.Position;
    float4 dx12 = v2.Position - v1.Position;
    float4 dx20 = v0.Position - v2.Position;

    __m256 v0x = _mm256_set1_ps(v0.Position.x);
    __m256 v0y = _mm256_set1_ps(v0.Position.y);
    __m256 v1x = _mm256_set1_ps(v1.Position.x);
    __m256 v1y = _mm256_set1_ps(v1.Position.y);
    __m256 v2x = _mm256_set1_ps(v2.Position.x);
    __m256 v2y = _mm256_set1_ps(v2.Position.y);
    __m256 v0z = _mm256_set1_ps(v0.Position.z);
    __m256 v1z = _mm256_set1_ps(v1.Position.z);
    __m256 v2z = _mm256_set1_ps(v2.Position.z);

    __m256 invArea = _mm256_set1_ps(1.0f / area2);
    __m256 dx01v = _mm256_set1_ps(dx01.x);
    __m256 dy01v = _mm256_set1_ps(dx01.y);
    __m256 dx12v = _mm256_set1_ps(dx12.x);
    __m256 dy12v = _mm256_set1_ps(dx12.y);
    __m256 dx20v = _mm256_set1_ps(dx20.x);
    __m256 dy20v = _mm256_set1_ps(dx20.y);

    __m256 zero = _mm256_setzero_ps();
    __m256 tileMinXv = _mm256_set1_ps(float(tileMin.x));
    __m256 tileMaxXv = _mm256_set1_ps(float(tileMax.x));

    __m256 f01StepX = _mm256_set1_ps(8.0f * dx01.y);
    __m256 f12StepX = _mm256_set1_ps(8.0f * dx12.y);
    __m256 f20StepX = _mm256_set1_ps(8.0f * dx20.y);
    __m256 f01StepY = _mm256_set1_ps(-dx01.x);
    __m256 f12StepY = _mm256_set1_ps(-dx12.x);
    __m256 f20StepY = _mm256_set1_ps(-dx20.x);

    const int simdStartX = (iMinX / 8) * 8;

    __m256 f01Row, f12Row, f20Row;
    {
        __m256 initX = _mm256_set_ps(
            simdStartX + 7.5f, simdStartX + 6.5f,
            simdStartX + 5.5f, simdStartX + 4.5f,
            simdStartX + 3.5f, simdStartX + 2.5f,
            simdStartX + 1.5f, simdStartX + 0.5f);
        __m256 initY = _mm256_set1_ps(iMinY + 0.5f);

        f01Row = _mm256_sub_ps(_mm256_mul_ps(_mm256_sub_ps(initX, v0x), dy01v),
            _mm256_mul_ps(_mm256_sub_ps(initY, v0y), dx01v));
        f12Row = _mm256_sub_ps(_mm256_mul_ps(_mm256_sub_ps(initX, v1x), dy12v),
            _mm256_mul_ps(_mm256_sub_ps(initY, v1y), dx12v));
        f20Row = _mm256_sub_ps(_mm256_mul_ps(_mm256_sub_ps(initX, v2x), dy20v),
            _mm256_mul_ps(_mm256_sub_ps(initY, v2y), dx20v));
    }

    uint width = depthBuffer.Width();
    uint32_t visibleCount = 0;

    for (int y = iMinY; y <= iMaxY; ++y)
    {
        int x = simdStartX;
        __m256 f01 = f01Row, f12 = f12Row, f20 = f20Row;

        for (; x + 7 < (int)width && x <= iMaxX - 7;
            x += 8,
            f01 = _mm256_add_ps(f01, f01StepX),
            f12 = _mm256_add_ps(f12, f12StepX),
            f20 = _mm256_add_ps(f20, f20StepX))
        {
            if (x > (int)tileMax.x || x + 7 < (int)tileMin.x)
                continue;

            // Inside test
            __m256 inside;
            if (area2 > 0)
                inside = _mm256_and_ps(
                    _mm256_and_ps(_mm256_cmp_ps(f01, zero, _CMP_GE_OQ),
                        _mm256_cmp_ps(f12, zero, _CMP_GE_OQ)),
                    _mm256_cmp_ps(f20, zero, _CMP_GE_OQ));
            else
                inside = _mm256_and_ps(
                    _mm256_and_ps(_mm256_cmp_ps(f01, zero, _CMP_LE_OQ),
                        _mm256_cmp_ps(f12, zero, _CMP_LE_OQ)),
                    _mm256_cmp_ps(f20, zero, _CMP_LE_OQ));

            if (_mm256_movemask_ps(inside) == 0)
                continue;

            // Barycenter coords
            __m256 alpha = _mm256_mul_ps(f12, invArea);
            __m256 beta = _mm256_mul_ps(f20, invArea);
            __m256 gamma = _mm256_mul_ps(f01, invArea);

            // Linear interpolation z
            __m256 z = _mm256_add_ps(
                _mm256_add_ps(_mm256_mul_ps(alpha, v0z),
                    _mm256_mul_ps(beta, v1z)),
                _mm256_mul_ps(gamma, v2z));

            // Depth read
            __m128 d_lo = depthBuffer.Read4(uint2(x, y));
            __m128 d_hi = depthBuffer.Read4(uint2(x + 4, y));
            __m256 depths = _mm256_set_m128(d_hi, d_lo);

            // Depth test
            __m256 depthCmp;
            switch (state.depthFunc)
            {
            case ComparisonFunc::Never:        depthCmp = _mm256_setzero_ps(); break;
            case ComparisonFunc::Less:         depthCmp = _mm256_cmp_ps(z, depths, _CMP_LT_OQ); break;
            case ComparisonFunc::Equal:        depthCmp = _mm256_cmp_ps(z, depths, _CMP_EQ_OQ); break;
            case ComparisonFunc::LessEqual:    depthCmp = _mm256_cmp_ps(z, depths, _CMP_LE_OQ); break;
            case ComparisonFunc::Greater:      depthCmp = _mm256_cmp_ps(z, depths, _CMP_GT_OQ); break;
            case ComparisonFunc::NotEqual:     depthCmp = _mm256_cmp_ps(z, depths, _CMP_NEQ_OQ); break;
            case ComparisonFunc::GreaterEqual: depthCmp = _mm256_cmp_ps(z, depths, _CMP_GE_OQ); break;
            case ComparisonFunc::Always:       depthCmp = _mm256_castsi256_ps(_mm256_set1_epi32(-1)); break;
            default:                           depthCmp = _mm256_cmp_ps(z, depths, _CMP_LT_OQ); break;
            }

            // Tile mask
            __m256 pxf = _mm256_set_ps(float(x + 7), float(x + 6), float(x + 5), float(x + 4),
                float(x + 3), float(x + 2), float(x + 1), float(x + 0));
            __m256 tileMask = _mm256_and_ps(
                _mm256_cmp_ps(pxf, tileMinXv, _CMP_GE_OQ),
                _mm256_cmp_ps(pxf, tileMaxXv, _CMP_LE_OQ));

            __m256 finalMask = _mm256_and_ps(_mm256_and_ps(depthCmp, inside), tileMask);
            int mask = _mm256_movemask_ps(finalMask);
            if (mask == 0) continue;

            visibleCount += _mm_popcnt_u32(mask);

            if (state.depthWriteEnable)
            {
                __m128 mask_lo = _mm256_castps256_ps128(finalMask);
                __m128 mask_hi = _mm256_extractf128_ps(finalMask, 1);
                depthBuffer.Write4(uint2(x, y), _mm256_castps256_ps128(z), mask_lo);
                depthBuffer.Write4(uint2(x + 4, y), _mm256_extractf128_ps(z, 1), mask_hi);
            }
        }

        f01Row = _mm256_add_ps(f01Row, f01StepY);
        f12Row = _mm256_add_ps(f12Row, f12StepY);
        f20Row = _mm256_add_ps(f20Row, f20StepY);

        for (; x <= iMaxX; ++x)
        {
            if (x < (int)tileMin.x || x >(int)tileMax.x)
                continue;

            float2 p((float)x + 0.5f, (float)y + 0.5f);
            float f0 = RasterizerCommon::EdgeFunction(v1.Position, v2.Position, p);
            float f1 = RasterizerCommon::EdgeFunction(v2.Position, v0.Position, p);
            float f2 = RasterizerCommon::EdgeFunction(v0.Position, v1.Position, p);

            if ((area2 > 0 && (f0 < 0 || f1 < 0 || f2 < 0)) ||
                (area2 < 0 && (f0 > 0 || f1 > 0 || f2 > 0)))
                continue;

            float a = f0 / area2;
            float b = f1 / area2;
            float c = f2 / area2;
            float z = a * v0.Position.z + b * v1.Position.z + c * v2.Position.z;

            uint idx = y * width + x;
            bool depthPass = false;
            switch (state.depthFunc)
            {
            case ComparisonFunc::Never:        depthPass = false; break;
            case ComparisonFunc::Less:         depthPass = z < depthBuffer.At(idx); break;
            case ComparisonFunc::Equal:        depthPass = z == depthBuffer.At(idx); break;
            case ComparisonFunc::LessEqual:    depthPass = z <= depthBuffer.At(idx); break;
            case ComparisonFunc::Greater:      depthPass = z > depthBuffer.At(idx); break;
            case ComparisonFunc::NotEqual:     depthPass = z != depthBuffer.At(idx); break;
            case ComparisonFunc::GreaterEqual: depthPass = z >= depthBuffer.At(idx); break;
            case ComparisonFunc::Always:       depthPass = true; break;
            }
            if (depthPass)
            {
                if (state.depthWriteEnable)
                    depthBuffer.At(idx) = z;
                ++visibleCount;
            }
        }
    }

    return visibleCount;
}

SOFTX_END
/////////////////////////////////////////////////////////////////
