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

void DepthBuffer::GenerateHiZ(HiZReduction mode)
{
    switch (mode)
    {
    case HiZReduction::Min: GenerateHiZImpl<HiZReduction::Min>(); break;
    case HiZReduction::Max: GenerateHiZImpl<HiZReduction::Max>(); break;
    }
}

template <DepthBuffer::HiZReduction Mode>
void DepthBuffer::GenerateHiZImpl()
{
    for (uint m = 1; m < MipCount(); ++m)
    {
        const Level& prev = levels[m - 1];
        const Level& curr = levels[m];
        const __m128* prevData = LevelData(m - 1);
        __m128* currData = LevelData(m);

        const uint nw = curr.resolution.x;
        const uint nh = curr.resolution.y;
        const uint prevPadded = prev.widthPadded;
        const uint prevBlocksPerRow = prevPadded / 4;
        const uint currPadded = curr.widthPadded;

        if (nw == 0 || nh == 0) continue;

        auto processRow = [&](uint y)
        {
            const uint py = y * 2;
            const __m128* topRow = prevData + (py * prevPadded) / 4;
            const __m128* botRow = prevData + ((py + 1) * prevPadded) / 4;
            __m128* outRow = currData + (y * currPadded) / 4;

            uint x = 0;

            // SIMD: 4 pixels at once
            for (; x + 4 <= nw; x += 4)
            {
                const uint ib0 = x / 2;
                const uint ib1 = ib0 + 1;
                if (ib1 >= prevBlocksPerRow) break;

                const __m128 A = topRow[ib0];
                const __m128 B = topRow[ib1];
                const __m128 C = botRow[ib0];
                const __m128 D = botRow[ib1];

                const __m128 V0 = _mm_shuffle_ps(A, B, _MM_SHUFFLE(2, 0, 2, 0));
                const __m128 V1 = _mm_shuffle_ps(A, B, _MM_SHUFFLE(3, 1, 3, 1));
                const __m128 V2 = _mm_shuffle_ps(C, D, _MM_SHUFFLE(2, 0, 2, 0));
                const __m128 V3 = _mm_shuffle_ps(C, D, _MM_SHUFFLE(3, 1, 3, 1));

                if constexpr (Mode == HiZReduction::Min)
                    outRow[x / 4] = _mm_min_ps(_mm_min_ps(V0, V1), _mm_min_ps(V2, V3));
                else
                    outRow[x / 4] = _mm_max_ps(_mm_max_ps(V0, V1), _mm_max_ps(V2, V3));
            }

            // Tail (0..3 pixels)
            for (; x < nw; ++x)
            {
                const uint px = x * 2;
                const float d00 = topRow[px / 4].m128_f32[px % 4];
                const float d10 = topRow[(px + 1) / 4].m128_f32[(px + 1) % 4];
                const float d01 = botRow[px / 4].m128_f32[px % 4];
                const float d11 = botRow[(px + 1) / 4].m128_f32[(px + 1) % 4];

                float reduced;
                if constexpr (Mode == HiZReduction::Min)
                    reduced = std::min({ d00, d10, d01, d11 });
                else
                    reduced = std::max({ d00, d10, d01, d11 });

                outRow[x / 4].m128_f32[x % 4] = reduced;
            }
        };

        const bool canParallel = nh >= 32 && (nw * nh) >= 4096;

        if (canParallel)
        {
            PROFILE_SCOPE("DepthBuffer::GenerateHiZ (parallel)");
            std::atomic<uint> nextRow{ 0 };
            auto worker = [&]()
            {
                uint y;
                while ((y = nextRow.fetch_add(1)) < nh)
                    processRow(y);
            };
            ThreadUtils::DispatchWorkers(worker);
        }
        else
        {
            PROFILE_SCOPE("DepthBuffer::GenerateHiZ (serial)");
            for (uint y = 0; y < nh; ++y)
                processRow(y);
        }
    }
}

SOFTX_END
/////////////////////////////////////////////////////////////////
