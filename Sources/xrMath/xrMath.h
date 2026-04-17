#pragma once

#include "xrMath_common.h"
#include "xrMath_types.h"
#include "xrMath_constants.h"
#include "xrMath_bitwise.h"
#include "xrMath_utils.h"

template <class T> struct _quaternion;

#pragma pack(push)
#pragma pack(1)

#include "xrMath_random.h"

#include "xrMath_color.h"
#include "xrMath_vector3d.h"
#include "xrMath_vector2.h"
#include "xrMath_vector4.h"
#include "xrMath_matrix.h"
#include "xrMath_matrix33.h"
#include "xrMath_quaternion.h"
#include "xrMath_rect.h"
#include "xrMath_fbox.h"
#include "xrMath_fbox2.h"
#include "xrMath_obb.h"
#include "xrMath_sphere.h"
#include "xrMath_cylinder.h"
#include "xrMath_random.h"
#include "xrMath_compressed_normal.h"
#include "xrMath_plane.h"
#include "xrMath_plane2.h"
#include "xrMath_flags.h"
#include "xrMath_splines.h"
#include "xrMath_implementation.h"

#pragma pack(pop)

extern XRMATH_API float4x4 Fidentity;
extern XRMATH_API CRandom Random;
