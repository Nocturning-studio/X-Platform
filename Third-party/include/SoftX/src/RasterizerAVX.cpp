/////////////////////////////////////////////////////////////////
// SoftX – Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "pch.h"

#include <SoftX.h>
#include "RasterizerCommon.h"
#include "RasterizerAVX.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

void RasterizerAVX::RasterizeTriangle(const VertexOutput& v0,
                                      const VertexOutput& v1,
                                      const VertexOutput& v2,
                                      const RasterizerState& state,
                                      DepthBuffer& depthBuffer,
                                      IRenderTarget* renderTarget,
                                      const PixelShader& ps,
                                      const ConstantBuffer& cb,
                                      const TextureTable* tt,
                                      uint2 tileMin,
                                      uint2 tileMax)
{
    float minX = std::min({v0.Position.x, v1.Position.x, v2.Position.x});
    float maxX = std::max({v0.Position.x, v1.Position.x, v2.Position.x});
    float minY = std::min({v0.Position.y, v1.Position.y, v2.Position.y});
    float maxY = std::max({v0.Position.y, v1.Position.y, v2.Position.y});

    int iMinX = std::max((int)tileMin.x, (int)std::floor(minX));
    int iMaxX = std::min((int)tileMax.x, (int)std::ceil(maxX));
    int iMinY = std::max((int)tileMin.y, (int)std::floor(minY));
    int iMaxY = std::min((int)tileMax.y, (int)std::ceil(maxY));

    if (iMinX > iMaxX || iMinY > iMaxY) UNLIKELY
        return;

    float area2 = RasterizerCommon::EdgeFunction(v0.Position, v1.Position, v2.Position);
    CullMode cull = state.cullMode;
    if (cull == CullMode::Back  && area2 < 0) return;
    if (cull == CullMode::Front && area2 > 0) return;
    if (std::abs(area2) < 1e-6f) UNLIKELY return;

    // Triangle edges
    float4 dx01 = v1.Position - v0.Position;
    float4 dx12 = v2.Position - v1.Position;
    float4 dx20 = v0.Position - v2.Position;

    // Broadcast vertex positions for edge function initialization
    __m256 v0x = _mm256_set1_ps(v0.Position.x);
    __m256 v0y = _mm256_set1_ps(v0.Position.y);
    __m256 v1x = _mm256_set1_ps(v1.Position.x);
    __m256 v1y = _mm256_set1_ps(v1.Position.y);
    __m256 v2x = _mm256_set1_ps(v2.Position.x);
    __m256 v2y = _mm256_set1_ps(v2.Position.y);

    __m256 v0z = _mm256_set1_ps(v0.Position.z);
    __m256 v1z = _mm256_set1_ps(v1.Position.z);
    __m256 v2z = _mm256_set1_ps(v2.Position.z);

    __m256 v0cr = _mm256_set1_ps(v0.Color.x);
    __m256 v0cg = _mm256_set1_ps(v0.Color.y);
    __m256 v0cb = _mm256_set1_ps(v0.Color.z);
    __m256 v0ca = _mm256_set1_ps(v0.Color.w);
    __m256 v1cr = _mm256_set1_ps(v1.Color.x);
    __m256 v1cg = _mm256_set1_ps(v1.Color.y);
    __m256 v1cb = _mm256_set1_ps(v1.Color.z);
    __m256 v1ca = _mm256_set1_ps(v1.Color.w);
    __m256 v2cr = _mm256_set1_ps(v2.Color.x);
    __m256 v2cg = _mm256_set1_ps(v2.Color.y);
    __m256 v2cb = _mm256_set1_ps(v2.Color.z);
    __m256 v2ca = _mm256_set1_ps(v2.Color.w);

    __m256 v0nx = _mm256_set1_ps(v0.Normal.x);
    __m256 v0ny = _mm256_set1_ps(v0.Normal.y);
    __m256 v0nz = _mm256_set1_ps(v0.Normal.z);
    __m256 v1nx = _mm256_set1_ps(v1.Normal.x);
    __m256 v1ny = _mm256_set1_ps(v1.Normal.y);
    __m256 v1nz = _mm256_set1_ps(v1.Normal.z);
    __m256 v2nx = _mm256_set1_ps(v2.Normal.x);
    __m256 v2ny = _mm256_set1_ps(v2.Normal.y);
    __m256 v2nz = _mm256_set1_ps(v2.Normal.z);

    __m256 v0u = _mm256_set1_ps(v0.UV.x);
    __m256 v0v = _mm256_set1_ps(v0.UV.y);
    __m256 v1u = _mm256_set1_ps(v1.UV.x);
    __m256 v1v = _mm256_set1_ps(v1.UV.y);
    __m256 v2u = _mm256_set1_ps(v2.UV.x);
    __m256 v2v = _mm256_set1_ps(v2.UV.y);

    __m256 invArea = _mm256_set1_ps(1.0f / area2);
    __m256 dx01v   = _mm256_set1_ps(dx01.x);
    __m256 dy01v   = _mm256_set1_ps(dx01.y);
    __m256 dx12v   = _mm256_set1_ps(dx12.x);
    __m256 dy12v   = _mm256_set1_ps(dx12.y);
    __m256 dx20v   = _mm256_set1_ps(dx20.x);
    __m256 dy20v   = _mm256_set1_ps(dx20.y);

    // Triangle constants, hoisted out of loops
    __m256 v0w  = _mm256_set1_ps(v0.Position.w);
    __m256 v1w  = _mm256_set1_ps(v1.Position.w);
    __m256 v2w  = _mm256_set1_ps(v2.Position.w);
    __m256 ones = _mm256_set1_ps(1.0f);
    __m256 zero = _mm256_setzero_ps();

    __m256 tileMinXv = _mm256_set1_ps((float)tileMin.x);
    __m256 tileMaxXv = _mm256_set1_ps((float)tileMax.x);

    // Incremental edge functions
    // When x → x+8:  Δf = +8 * dy
    // When y → y+1:  Δf = -dx
    __m256 f01StepX = _mm256_set1_ps( 8.0f * dx01.y);
    __m256 f12StepX = _mm256_set1_ps( 8.0f * dx12.y);
    __m256 f20StepX = _mm256_set1_ps( 8.0f * dx20.y);
    __m256 f01StepY = _mm256_set1_ps(-dx01.x);
    __m256 f12StepY = _mm256_set1_ps(-dx12.x);
    __m256 f20StepY = _mm256_set1_ps(-dx20.x);

    // SIMD start X aligned to multiple of 8
    const int simdStartX = (iMinX / 8) * 8;

    // Initialize edge functions for the first row
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

    uint width = 0;
    if(renderTarget)
        width = renderTarget->Width();
    else
        width = depthBuffer.Width();

    for (int y = iMinY; y <= iMaxY; ++y)
    {
        int x;
        __m256 f01, f12, f20;

        // Increment f in the for expression; continue does not break accumulation
        for (x = simdStartX, f01 = f01Row, f12 = f12Row, f20 = f20Row;
             x + 7 < (int)width && x <= iMaxX - 7;
             x += 8,
             f01 = _mm256_add_ps(f01, f01StepX),
             f12 = _mm256_add_ps(f12, f12StepX),
             f20 = _mm256_add_ps(f20, f20StepX))
        {
            if (x > (int)tileMax.x || x + 7 < (int)tileMin.x)
                continue;

            // f01/f12/f20 already computed incrementally — 3 adds instead of 6 mul + 6 sub
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

            __m256 alpha = _mm256_mul_ps(f12, invArea);
            __m256 beta  = _mm256_mul_ps(f20, invArea);
            __m256 gamma = _mm256_mul_ps(f01, invArea);

            // Perspective-correct weights
            __m256 pw0 = _mm256_mul_ps(alpha, v0w);
            __m256 pw1 = _mm256_mul_ps(beta,  v1w);
            __m256 pw2 = _mm256_mul_ps(gamma, v2w);

            __m256 pwSum    = _mm256_add_ps(_mm256_add_ps(pw0, pw1), pw2);
            __m256 invPwSum = _mm256_div_ps(ones, pwSum);

            // z: linear interpolation (perspective correction not needed)
            __m256 z = _mm256_add_ps(
                _mm256_add_ps(_mm256_mul_ps(alpha, v0z),
                              _mm256_mul_ps(beta,  v1z)),
                              _mm256_mul_ps(gamma, v2z));

            // Attributes: perspective-correct
#define PLERP256(a0, a1, a2)                                              \
    _mm256_mul_ps(                                                        \
        _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(pw0, a0),              \
                                    _mm256_mul_ps(pw1, a1)),              \
                                    _mm256_mul_ps(pw2, a2)),              \
        invPwSum)

            __m256 r  = PLERP256(v0cr, v1cr, v2cr);
            __m256 g  = PLERP256(v0cg, v1cg, v2cg);
            __m256 b  = PLERP256(v0cb, v1cb, v2cb);
            __m256 a  = PLERP256(v0ca, v1ca, v2ca);
            __m256 nx = PLERP256(v0nx, v1nx, v2nx);
            __m256 ny = PLERP256(v0ny, v1ny, v2ny);
            __m256 nz = PLERP256(v0nz, v1nz, v2nz);
            __m256 u  = PLERP256(v0u,  v1u,  v2u);
            __m256 v  = PLERP256(v0v,  v1v,  v2v);

#undef PLERP256

            // Depth read: two __m128 blocks → __m256
            // x is multiple of 8 → x and x+4 are both multiples of 4, Read4 works correctly
            __m128 d_lo = depthBuffer.Read4(uint2(x,     y));
            __m128 d_hi = depthBuffer.Read4(uint2(x + 4, y));
            __m256 depths = _mm256_set_m128(d_hi, d_lo);

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

            // inside is already a SIMD mask
            __m256 pxf = _mm256_set_ps(float(x+7), float(x+6), float(x+5), float(x+4),
                                        float(x+3), float(x+2), float(x+1), float(x+0));
            __m256 tileMask  = _mm256_and_ps(_mm256_cmp_ps(pxf, tileMinXv, _CMP_GE_OQ),
                                              _mm256_cmp_ps(pxf, tileMaxXv, _CMP_LE_OQ));
            __m256 finalMask = _mm256_and_ps(_mm256_and_ps(depthCmp, inside), tileMask);
            int depthMask    = _mm256_movemask_ps(finalMask);
            if (depthMask == 0)
                continue;

            if (state.depthWriteEnable)
            {
                __m128 mask_lo = _mm256_castps256_ps128(finalMask);
                __m128 mask_hi = _mm256_extractf128_ps(finalMask, 1);
                depthBuffer.Write4(uint2(x, y), _mm256_castps256_ps128(z), mask_lo);
                depthBuffer.Write4(uint2(x + 4, y), _mm256_extractf128_ps(z, 1), mask_hi);
            }

            // Scalar loop for shading only
            alignas(32) float zArr[8], rArr[8], gArr[8], bArr[8], aArr[8];
            alignas(32) float uArr[8], vArr[8], nxArr[8], nyArr[8], nzArr[8];
            _mm256_store_ps(zArr,  z);
            _mm256_store_ps(rArr,  r);  
            _mm256_store_ps(gArr,  g);
            _mm256_store_ps(bArr,  b);  
            _mm256_store_ps(aArr,  a);
            _mm256_store_ps(uArr,  u);  
            _mm256_store_ps(vArr,  v);
            _mm256_store_ps(nxArr, nx); 
            _mm256_store_ps(nyArr, ny);
            _mm256_store_ps(nzArr, nz);

            for (int i = 0; i < 8; ++i)
            {
                if (!(depthMask & (1 << i))) continue;
                uint px = x + i;
                if (px < tileMin.x || px > tileMax.x) continue;
                if (renderTarget == nullptr) continue;

                VertexOutput frag;
                frag.Position = float4((float)px, (float)y, zArr[i], 1.0f);
                frag.Color    = float4(rArr[i], gArr[i], bArr[i], aArr[i]);
                frag.Normal   = float3(nxArr[i], nyArr[i], nzArr[i]);
                frag.UV       = float2(uArr[i], vArr[i]);
                renderTarget->SetPixel(uint2(px, y), ps(frag, cb, *tt));
            }
        }

        // Step to next row
        f01Row = _mm256_add_ps(f01Row, f01StepY);
        f12Row = _mm256_add_ps(f12Row, f12StepY);
        f20Row = _mm256_add_ps(f20Row, f20StepY);

        // Scalar fallback for remaining pixels
        for (; x <= iMaxX; ++x)
        {
            if (x < (int)tileMin.x || x > (int)tileMax.x)
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

            VertexOutput frag = RasterizerCommon::Trilerp(v0, v1, v2, a, b, c);
            uint idx = y * width + x;

            bool depthPass = false;
            switch (state.depthFunc)
            {
            case ComparisonFunc::Never:        depthPass = false; break;
            case ComparisonFunc::Less:         depthPass = frag.Position.z <  depthBuffer.At(idx); break;
            case ComparisonFunc::Equal:        depthPass = frag.Position.z == depthBuffer.At(idx); break;
            case ComparisonFunc::LessEqual:    depthPass = frag.Position.z <= depthBuffer.At(idx); break;
            case ComparisonFunc::Greater:      depthPass = frag.Position.z >  depthBuffer.At(idx); break;
            case ComparisonFunc::NotEqual:     depthPass = frag.Position.z != depthBuffer.At(idx); break;
            case ComparisonFunc::GreaterEqual: depthPass = frag.Position.z >= depthBuffer.At(idx); break;
            case ComparisonFunc::Always:       depthPass = true; break;
            }
            if (depthPass)
            {
                if (state.depthWriteEnable)
                    depthBuffer.At(idx) = frag.Position.z;
                if(renderTarget != nullptr)
                    renderTarget->SetPixel(uint2(x, y), ps(frag, cb, *tt));
            }
        }
    }
}

SOFTX_END
/////////////////////////////////////////////////////////////////
