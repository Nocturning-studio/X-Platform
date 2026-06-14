#include "pch.h"

#include "RasterizerCommon.h"
#include <SoftX/SoftX.h>
#include <xmmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>

SOFTX_BEGIN

uint32_t QueryRasterizerSSE::RasterizeTriangle(
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

    const int x0fp = RasterizerCommon::ToFixed(v0.Position.x);
    const int y0fp = RasterizerCommon::ToFixed(v0.Position.y);
    const int x1fp = RasterizerCommon::ToFixed(v1.Position.x);
    const int y1fp = RasterizerCommon::ToFixed(v1.Position.y);
    const int x2fp = RasterizerCommon::ToFixed(v2.Position.x);
    const int y2fp = RasterizerCommon::ToFixed(v2.Position.y);

    int64_t area2Int = RasterizerCommon::EdgeFunctionInt(x0fp, y0fp, x1fp, y1fp, x2fp, y2fp);

    // Culling
    const CullMode cull = state.cullMode;
    if (cull == CullMode::Back && area2Int < 0) return 0;
    if (cull == CullMode::Front && area2Int > 0) return 0;
    if (area2Int == 0) UNLIKELY return 0;

    // --- normalizing, inside test always must be f >= 0 ---
    const int normSign = (area2Int > 0) ? 1 : -1;
    if (area2Int < 0)
        area2Int = -area2Int;

    int stepX01 = RasterizerCommon::SUBPIXEL_STEP * (y1fp - y0fp);
    int stepX12 = RasterizerCommon::SUBPIXEL_STEP * (y2fp - y1fp);
    int stepX20 = RasterizerCommon::SUBPIXEL_STEP * (y0fp - y2fp);
    int stepY01 = -RasterizerCommon::SUBPIXEL_STEP * (x1fp - x0fp);
    int stepY12 = -RasterizerCommon::SUBPIXEL_STEP * (x2fp - x1fp);
    int stepY20 = -RasterizerCommon::SUBPIXEL_STEP * (x0fp - x2fp);

    const int simdStartX = (iMinX / 4) * 4;
    int64_t f01Row = normSign * RasterizerCommon::EdgeFunctionInt(
        x0fp, y0fp, x1fp, y1fp,
        RasterizerCommon::PixelCentre(simdStartX),
        RasterizerCommon::PixelCentre(iMinY));
    int64_t f12Row = normSign * RasterizerCommon::EdgeFunctionInt(
        x1fp, y1fp, x2fp, y2fp,
        RasterizerCommon::PixelCentre(simdStartX),
        RasterizerCommon::PixelCentre(iMinY));
    int64_t f20Row = normSign * RasterizerCommon::EdgeFunctionInt(
        x2fp, y2fp, x0fp, y0fp,
        RasterizerCommon::PixelCentre(simdStartX),
        RasterizerCommon::PixelCentre(iMinY));

    if (normSign == -1) {
        stepX01 = -stepX01; stepX12 = -stepX12; stepX20 = -stepX20;
        stepY01 = -stepY01; stepY12 = -stepY12; stepY20 = -stepY20;
    }

    const uint width = depthBuffer.Width();
    uint32_t visibleCount = 0;

    const __m128 invAreaV = _mm_set1_ps(1.0f / float(area2Int));
    const __m128i minusOne = _mm_set1_epi32(-1);
    const __m128 tileMinXv = _mm_set1_ps(float(tileMin.x));
    const __m128 tileMaxXv = _mm_set1_ps(float(tileMax.x));

    const __m128 v0z = _mm_set1_ps(v0.Position.z);
    const __m128 v1z = _mm_set1_ps(v1.Position.z);
    const __m128 v2z = _mm_set1_ps(v2.Position.z);

    const __m128i s01X4 = _mm_set1_epi32(4 * stepX01);
    const __m128i s12X4 = _mm_set1_epi32(4 * stepX12);
    const __m128i s20X4 = _mm_set1_epi32(4 * stepX20);

    for (int y = iMinY; y <= iMaxY; ++y)
    {
        __m128i f01 = _mm_add_epi32(
            _mm_set1_epi32(static_cast<int32_t>(f01Row)),
            _mm_set_epi32(3 * stepX01, 2 * stepX01, stepX01, 0));
        __m128i f12 = _mm_add_epi32(
            _mm_set1_epi32(static_cast<int32_t>(f12Row)),
            _mm_set_epi32(3 * stepX12, 2 * stepX12, stepX12, 0));
        __m128i f20 = _mm_add_epi32(
            _mm_set1_epi32(static_cast<int32_t>(f20Row)),
            _mm_set_epi32(3 * stepX20, 2 * stepX20, stepX20, 0));

        int x = simdStartX;
        for (; x + 3 < (int)width && x <= iMaxX - 3;
            x += 4,
            f01 = _mm_add_epi32(f01, s01X4),
            f12 = _mm_add_epi32(f12, s12X4),
            f20 = _mm_add_epi32(f20, s20X4))
        {
            if (x > (int)tileMax.x || x + 3 < (int)tileMin.x)
                continue;

            __m128i insideI = _mm_and_si128(
                _mm_and_si128(_mm_cmpgt_epi32(f01, minusOne),
                    _mm_cmpgt_epi32(f12, minusOne)),
                _mm_cmpgt_epi32(f20, minusOne));

            if (_mm_testz_si128(insideI, insideI))
                continue;

            __m128 alpha = _mm_mul_ps(_mm_cvtepi32_ps(f12), invAreaV);
            __m128 beta = _mm_mul_ps(_mm_cvtepi32_ps(f20), invAreaV);
            __m128 gamma = _mm_mul_ps(_mm_cvtepi32_ps(f01), invAreaV);

            __m128 z = _mm_add_ps(_mm_add_ps(
                _mm_mul_ps(alpha, v0z),
                _mm_mul_ps(beta, v1z)),
                _mm_mul_ps(gamma, v2z));

            __m128 depths = depthBuffer.Read4(uint2(x, y));

            __m128 depthCmp;
            switch (state.depthFunc)
            {
            case ComparisonFunc::Never:        depthCmp = _mm_setzero_ps(); break;
            case ComparisonFunc::Less:         depthCmp = _mm_cmplt_ps(z, depths); break;
            case ComparisonFunc::Equal:        depthCmp = _mm_cmpeq_ps(z, depths); break;
            case ComparisonFunc::LessEqual:    depthCmp = _mm_cmple_ps(z, depths); break;
            case ComparisonFunc::Greater:      depthCmp = _mm_cmpgt_ps(z, depths); break;
            case ComparisonFunc::NotEqual:     depthCmp = _mm_cmpneq_ps(z, depths); break;
            case ComparisonFunc::GreaterEqual: depthCmp = _mm_cmpge_ps(z, depths); break;
            case ComparisonFunc::Always:       depthCmp = _mm_castsi128_ps(_mm_set1_epi32(-1)); break;
            default:                           depthCmp = _mm_cmplt_ps(z, depths); break;
            }

            __m128 pxf = _mm_set_ps(float(x + 3), float(x + 2), float(x + 1), float(x + 0));
            __m128 tileMask = _mm_and_ps(
                _mm_cmpge_ps(pxf, tileMinXv),
                _mm_cmple_ps(pxf, tileMaxXv));

            __m128 inside = _mm_castsi128_ps(insideI);
            __m128 finalMask = _mm_and_ps(_mm_and_ps(depthCmp, inside), tileMask);

            int mask = _mm_movemask_ps(finalMask);
            if (mask == 0)
                continue;

            visibleCount += _mm_popcnt_u32(mask);

            if (state.depthWriteEnable)
                depthBuffer.Write4(uint2(x, y), z, finalMask);
        }

        f01Row += stepY01;
        f12Row += stepY12;
        f20Row += stepY20;

        for (; x <= iMaxX; ++x)
        {
            if (x < (int)tileMin.x || x >(int)tileMax.x)
                continue;

            const int64_t sf01 = normSign * RasterizerCommon::EdgeFunctionInt(
                x0fp, y0fp, x1fp, y1fp,
                RasterizerCommon::PixelCentre(x),
                RasterizerCommon::PixelCentre(y));
            const int64_t sf12 = normSign * RasterizerCommon::EdgeFunctionInt(
                x1fp, y1fp, x2fp, y2fp,
                RasterizerCommon::PixelCentre(x),
                RasterizerCommon::PixelCentre(y));
            const int64_t sf20 = normSign * RasterizerCommon::EdgeFunctionInt(
                x2fp, y2fp, x0fp, y0fp,
                RasterizerCommon::PixelCentre(x),
                RasterizerCommon::PixelCentre(y));

            if (sf01 < 0 || sf12 < 0 || sf20 < 0)
                continue;

            const float fa = float(sf12) / float(area2Int);
            const float fb = float(sf20) / float(area2Int);
            const float fc = float(sf01) / float(area2Int);
            float z = fa * v0.Position.z + fb * v1.Position.z + fc * v2.Position.z;

            const uint idx = y * width + x;
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
