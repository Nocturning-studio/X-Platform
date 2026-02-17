#pragma once

#include "xrMathCommon.h"
#include "math_types.h"
#include "math_constants.h"
#include "_bitwise.h"
#include "math_utils.h"

template <class T> struct _quaternion;

#pragma pack(push)
#pragma pack(1)

#include "_random.h"

#include "_color.h"
#include "_vector3d.h"
#include "_vector2.h"
#include "_vector4.h"
#include "_matrix.h"
#include "_matrix33.h"
#include "_quaternion.h"
#include "_rect.h"
#include "_fbox.h"
#include "_fbox2.h"
#include "_obb.h"
#include "_sphere.h"
#include "_cylinder.h"
#include "_random.h"
#include "_compressed_normal.h"
#include "_plane.h"
#include "_plane2.h"
#include "_flags.h"
#include "math_splines.h"
#include "math_implementation.h"

#pragma pack(pop)

extern XRMATH_API float4x4 Fidentity;
extern XRMATH_API CRandom Random;
