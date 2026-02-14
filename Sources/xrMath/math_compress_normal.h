#pragma once

#include "xrMathCommon.h"
#include "math_float3.h"

XRAY_BEGIN

// —жатие единичного вектора в 16 бит (дл€ нормалей и т.п.)
XRMATH_API u16 compress_normal(const float3& vec);
XRMATH_API void decompress_normal(float3& vec, u16 code);
XRMATH_API void init_normal_compression_table(); // вызываетс€ один раз при старте

XRAY_END
