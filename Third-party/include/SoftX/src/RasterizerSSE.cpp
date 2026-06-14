#include "pch.h"

#include <SoftX/SoftX.h>
#include "RasterizerCommon.h"
#include <xmmintrin.h>
#include <smmintrin.h>

SOFTX_BEGIN

void RasterizerSSE::RasterizeTriangle(const VertexOutput& v0,
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

    // ── Fixed-point vertex coordinates (28.4 — 4 sub-pixel bits = 1/16 pixel) ──
    // delta_fp range: <= 2 * 3840 * 16 ≈ 2¹⁷, so init E <= 2³⁴ → int64 required.
    // Steps <= 16 * 2¹⁷ ≈ 2²¹ → int32 is sufficient.
    // SIMD accumulation over 480 iterations (4K / 8) <= 480 * 2²¹ ≈ 10⁹ < 2³¹ → safe.
    const int x0fp = RasterizerCommon::ToFixed(v0.Position.x);
    const int y0fp = RasterizerCommon::ToFixed(v0.Position.y);
    const int x1fp = RasterizerCommon::ToFixed(v1.Position.x);
    const int y1fp = RasterizerCommon::ToFixed(v1.Position.y);
    const int x2fp = RasterizerCommon::ToFixed(v2.Position.x);
    const int y2fp = RasterizerCommon::ToFixed(v2.Position.y);

    // Signed area in fixed-point² units (sign matches the float version)
    int64_t area2Int = RasterizerCommon::EdgeFunctionInt(x0fp, y0fp, x1fp, y1fp, x2fp, y2fp);

    const CullMode cull = state.cullMode;
    if (cull == CullMode::Back  && area2Int < 0) return;
    if (cull == CullMode::Front && area2Int > 0) return;
    if (area2Int == 0) UNLIKELY return;

    // ── Edge function steps ──────────────────────────────────────────────────
    // Pixel x → x+1:  ΔE = +S * Δy_fp
    // Row   y → y+1:  ΔE = -S * Δx_fp
    int stepX01 =  RasterizerCommon::SUBPIXEL_STEP * (y1fp - y0fp);
    int stepX12 =  RasterizerCommon::SUBPIXEL_STEP * (y2fp - y1fp);
    int stepX20 =  RasterizerCommon::SUBPIXEL_STEP * (y0fp - y2fp);
    int stepY01 = -RasterizerCommon::SUBPIXEL_STEP * (x1fp - x0fp);
    int stepY12 = -RasterizerCommon::SUBPIXEL_STEP * (x2fp - x1fp);
    int stepY20 = -RasterizerCommon::SUBPIXEL_STEP * (x0fp - x2fp);

    // SIMD start X aligned down to the nearest multiple of 4
    const int simdStartX = (iMinX / 4) * 4;

    // Edge function values at (simdStartX, iMinY)
    int64_t f01Row = RasterizerCommon::EdgeFunctionInt(
        x0fp, y0fp, x1fp, y1fp,
        RasterizerCommon::PixelCentre(simdStartX),
        RasterizerCommon::PixelCentre(iMinY));
    int64_t f12Row = RasterizerCommon::EdgeFunctionInt(
        x1fp, y1fp, x2fp, y2fp,
        RasterizerCommon::PixelCentre(simdStartX),
        RasterizerCommon::PixelCentre(iMinY));
    int64_t f20Row = RasterizerCommon::EdgeFunctionInt(
        x2fp, y2fp, x0fp, y0fp,
        RasterizerCommon::PixelCentre(simdStartX),
        RasterizerCommon::PixelCentre(iMinY));

    // ── Normalise to CCW — inside test is now always f >= 0 ─────────────────
    // Negating f and steps together leaves all ratios f/area2Int invariant,
    // so barycentric coordinates remain correct.
    const int normSign = (area2Int > 0) ? 1 : -1; // used in the scalar fallback
    if (area2Int < 0)
    {
        area2Int = -area2Int;
        f01Row = -f01Row; f12Row = -f12Row; f20Row = -f20Row;
        stepX01 = -stepX01; stepX12 = -stepX12; stepX20 = -stepX20;
        stepY01 = -stepY01; stepY12 = -stepY12; stepY20 = -stepY20;
    }

    // ── Float broadcasts of vertex attributes (needed for interpolation) ─────
    __m128 v0z  = _mm_set1_ps(v0.Position.z);
    __m128 v1z  = _mm_set1_ps(v1.Position.z);
    __m128 v2z  = _mm_set1_ps(v2.Position.z);
    __m128 v0w  = _mm_set1_ps(v0.Position.w);
    __m128 v1w  = _mm_set1_ps(v1.Position.w);
    __m128 v2w  = _mm_set1_ps(v2.Position.w);
    __m128 v0cr = _mm_set1_ps(v0.Color.x);
    __m128 v0cg = _mm_set1_ps(v0.Color.y);
    __m128 v0cb = _mm_set1_ps(v0.Color.z);
    __m128 v0ca = _mm_set1_ps(v0.Color.w);
    __m128 v1cr = _mm_set1_ps(v1.Color.x);
    __m128 v1cg = _mm_set1_ps(v1.Color.y);
    __m128 v1cb = _mm_set1_ps(v1.Color.z);
    __m128 v1ca = _mm_set1_ps(v1.Color.w);
    __m128 v2cr = _mm_set1_ps(v2.Color.x);
    __m128 v2cg = _mm_set1_ps(v2.Color.y);
    __m128 v2cb = _mm_set1_ps(v2.Color.z);
    __m128 v2ca = _mm_set1_ps(v2.Color.w);
    __m128 v0nx = _mm_set1_ps(v0.Normal.x);
    __m128 v0ny = _mm_set1_ps(v0.Normal.y);
    __m128 v0nz = _mm_set1_ps(v0.Normal.z);
    __m128 v1nx = _mm_set1_ps(v1.Normal.x);
    __m128 v1ny = _mm_set1_ps(v1.Normal.y);
    __m128 v1nz = _mm_set1_ps(v1.Normal.z);
    __m128 v2nx = _mm_set1_ps(v2.Normal.x);
    __m128 v2ny = _mm_set1_ps(v2.Normal.y);
    __m128 v2nz = _mm_set1_ps(v2.Normal.z);
    __m128 v0u  = _mm_set1_ps(v0.UV.x);
    __m128 v0v  = _mm_set1_ps(v0.UV.y);
    __m128 v1u  = _mm_set1_ps(v1.UV.x);
    __m128 v1v  = _mm_set1_ps(v1.UV.y);
    __m128 v2u  = _mm_set1_ps(v2.UV.x);
    __m128 v2v  = _mm_set1_ps(v2.UV.y);

    // Constants hoisted out of all loops
    const __m128 ones      = _mm_set1_ps(1.0f);
    // f_int / area2Int == f_float / area2_float — the S² factor cancels out
    const __m128 invAreaV  = _mm_set1_ps(1.0f / float(area2Int));
    const __m128 tileMinXv = _mm_set1_ps(float(tileMin.x));
    const __m128 tileMaxXv = _mm_set1_ps(float(tileMax.x));

    // Integer SIMD steps (4 lanes × step)
    const __m128i s01X4    = _mm_set1_epi32(4 * stepX01);
    const __m128i s12X4    = _mm_set1_epi32(4 * stepX12);
    const __m128i s20X4    = _mm_set1_epi32(4 * stepX20);
    // Inside test: f >= 0  ↔  f > -1 (one cmpgt instead of two cmpge/cmple)
    const __m128i minusOne = _mm_set1_epi32(-1);

    uint width = 0;
    if (renderTarget)
        width = renderTarget->Width();
    else
        width = depthBuffer.Width();

    for (int y = iMinY; y <= iMaxY; ++y)
    {
        // Initialise 4 lanes: lane i = f??Row + i * step??X
        // _mm_set_epi32(e3, e2, e1, e0) — e0 goes to lane 0, e3 to lane 3
        __m128i f01 = _mm_add_epi32(
            _mm_set1_epi32(static_cast<int32_t>(f01Row)),
            _mm_set_epi32(3 * stepX01, 2 * stepX01, stepX01, 0));
        __m128i f12 = _mm_add_epi32(
            _mm_set1_epi32(static_cast<int32_t>(f12Row)),
            _mm_set_epi32(3 * stepX12, 2 * stepX12, stepX12, 0));
        __m128i f20 = _mm_add_epi32(
            _mm_set1_epi32(static_cast<int32_t>(f20Row)),
            _mm_set_epi32(3 * stepX20, 2 * stepX20, stepX20, 0));

        int x;
        for (x = simdStartX;
             x + 3 < (int)width && x <= iMaxX - 3;
             x += 4,
             f01 = _mm_add_epi32(f01, s01X4),
             f12 = _mm_add_epi32(f12, s12X4),
             f20 = _mm_add_epi32(f20, s20X4))
        {
            if (x > (int)tileMax.x || x + 3 < (int)tileMin.x)
                continue;

            // Inside test: all three f >= 0 (CCW-normalised)
            // _mm_cmpgt_epi32(f, -1) returns 0xFFFFFFFF for lanes where f >= 0
            __m128i insideI = _mm_and_si128(
                _mm_and_si128(_mm_cmpgt_epi32(f01, minusOne),
                              _mm_cmpgt_epi32(f12, minusOne)),
                _mm_cmpgt_epi32(f20, minusOne));

            // _mm_testz_si128: ZF=1 if (insideI & insideI) == 0 — no lane passed
            if (_mm_testz_si128(insideI, insideI))
                continue;

            // Reinterpret int32 mask as float mask (bits are unchanged)
            __m128 inside = _mm_castsi128_ps(insideI);

            // Tile boundary mask (guards against data races at tile edges)
            // _mm_set_ps(e3, e2, e1, e0) — e0 goes to lane 0
            __m128 pxf      = _mm_set_ps(float(x + 3), float(x + 2), float(x + 1), float(x));
            __m128 tileMask = _mm_and_ps(_mm_cmpge_ps(pxf, tileMinXv),
                                          _mm_cmple_ps(pxf, tileMaxXv));

            // Barycentric coordinates: f_int / area2Int == f_float / area2_float
            // alpha — weight for v0 (opposite edge f12)
            // beta  — weight for v1 (opposite edge f20)
            // gamma — weight for v2 (opposite edge f01)
            __m128 alpha = _mm_mul_ps(_mm_cvtepi32_ps(f12), invAreaV);
            __m128 beta  = _mm_mul_ps(_mm_cvtepi32_ps(f20), invAreaV);
            __m128 gamma = _mm_mul_ps(_mm_cvtepi32_ps(f01), invAreaV);

            // Perspective-correct weights (pw = bary * 1/w)
            __m128 pw0 = _mm_mul_ps(alpha, v0w);
            __m128 pw1 = _mm_mul_ps(beta,  v1w);
            __m128 pw2 = _mm_mul_ps(gamma, v2w);
            __m128 pwSum    = _mm_add_ps(_mm_add_ps(pw0, pw1), pw2);
            __m128 invPwSum = _mm_div_ps(ones, pwSum);

            // z: linear interpolation (perspective correction not needed)
            __m128 z = _mm_add_ps(_mm_add_ps(_mm_mul_ps(alpha, v0z),
                                             _mm_mul_ps(beta,  v1z)),
                                             _mm_mul_ps(gamma, v2z));

            // Attributes: perspective-correct interpolation
#define PLERP128(a0, a1, a2) \
    _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(pw0, a0), \
                                     _mm_mul_ps(pw1, a1)), \
                                     _mm_mul_ps(pw2, a2)), invPwSum)

            __m128 r  = PLERP128(v0cr, v1cr, v2cr);
            __m128 g  = PLERP128(v0cg, v1cg, v2cg);
            __m128 b  = PLERP128(v0cb, v1cb, v2cb);
            __m128 a  = PLERP128(v0ca, v1ca, v2ca);
            __m128 nx = PLERP128(v0nx, v1nx, v2nx);
            __m128 ny = PLERP128(v0ny, v1ny, v2ny);
            __m128 nz = PLERP128(v0nz, v1nz, v2nz);
            __m128 u  = PLERP128(v0u,  v1u,  v2u);
            __m128 v  = PLERP128(v0v,  v1v,  v2v);

#undef PLERP128

            // Depth test
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

            __m128 finalMask = _mm_and_ps(_mm_and_ps(depthCmp, inside), tileMask);
            int depthMask    = _mm_movemask_ps(finalMask);
            if (depthMask == 0)
                continue;

            if (state.depthWriteEnable)
                depthBuffer.Write4(uint2(x, y), z, finalMask);

            // Scalar shading loop
            alignas(16) float zArr[4], rArr[4], gArr[4], bArr[4], aArr[4];
            alignas(16) float uArr[4], vArr[4], nxArr[4], nyArr[4], nzArr[4];
            _mm_store_ps(zArr, z);
            _mm_store_ps(rArr, r);   
            _mm_store_ps(gArr,  g);
            _mm_store_ps(bArr, b);   
            _mm_store_ps(aArr,  a);
            _mm_store_ps(uArr, u);   
            _mm_store_ps(vArr,  v);
            _mm_store_ps(nxArr, nx); 
            _mm_store_ps(nyArr, ny);
            _mm_store_ps(nzArr, nz);

            for (int i = 0; i < 4; ++i)
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

        // Advance row accumulators (int64 — exact, no drift)
        f01Row += stepY01;
        f12Row += stepY12;
        f20Row += stepY20;

        // Scalar fallback for remaining pixels (< 4 at the right tile edge).
        // Edge functions are recomputed directly from coordinates — exact, no accumulated error.
        // normSign applies the same CCW normalisation as the SIMD path.
        for (; x <= iMaxX; ++x)
        {
            if (x < (int)tileMin.x || x > (int)tileMax.x)
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

            const float fa = float(sf12) / float(area2Int); // weight for v0
            const float fb = float(sf20) / float(area2Int); // weight for v1
            const float fc = float(sf01) / float(area2Int); // weight for v2

            VertexOutput frag = RasterizerCommon::Trilerp(v0, v1, v2, fa, fb, fc);
            const uint idx    = y * width + x;

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
