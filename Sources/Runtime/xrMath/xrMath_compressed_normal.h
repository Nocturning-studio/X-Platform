// xrMath_compressed_normal.h
// A Unit Vector to 16-bit word conversion algorithm
// based on work of Rafael Baptista (rafael@oroboro.com)
// Accuracy improved by O.D. (punkfloyd@rocketmail.com)

#pragma once

#include "xrMath_types.h"
#include "xrMath_vector3.h"
#include "xrMath_bitwise.h"
#include <cstdint> // std::uint16_t
#include <cmath>   // std::sqrt

// upper 3 bits
constexpr std::uint16_t pvSIGN_MASK = 0xe000;
constexpr std::uint16_t pvXSIGN_MASK = 0x8000;
constexpr std::uint16_t pvYSIGN_MASK = 0x4000;
constexpr std::uint16_t pvZSIGN_MASK = 0x2000;

// middle 6 bits - xbits
constexpr std::uint16_t pvTOP_MASK = 0x1f80;

// lower 7 bits - ybits
constexpr std::uint16_t pvBOTTOM_MASK = 0x007f;

// -----------------------------------------------------------------------------
// Lazy-initialized lookup table for decompression
// -----------------------------------------------------------------------------
namespace xrMath::detail
{
inline const float* get_pvUVAdjustment()
{
	static float table[0x2000]; // zero-initialised first time
	static bool initialized = false;
	if (!initialized)
	{
		for (int idx = 0; idx < 0x2000; ++idx)
		{
			long xbits = idx >> 7;
			long ybits = idx & pvBOTTOM_MASK;

			// map the numbers back to the triangle (0,0)-(0,127)-(127,0)
			if ((xbits + ybits) >= 127)
			{
				xbits = 127 - xbits;
				ybits = 127 - ybits;
			}

			// convert to 3D vectors
			float x = static_cast<float>(xbits);
			float y = static_cast<float>(ybits);
			float z = static_cast<float>(126 - xbits - ybits);

			// calculate the amount of normalization required
			table[idx] = 1.0f / std::sqrt(y * y + z * z + x * x);
		}
		initialized = true;
	}
	return table;
}
} // namespace detail

// -----------------------------------------------------------------------------
// Compress a fvec3 vector into a 16-bit representation.
// -----------------------------------------------------------------------------
inline u16 compress_normal(const fvec3& vec)
{
	// save copy
	fvec3 tmp = vec;

	// input vector does not have to be unit length
	u16 mVec = 0;

	if (negative(tmp.x))
	{
		mVec |= pvXSIGN_MASK;
		set_positive(tmp.x);
	}
	if (negative(tmp.y))
	{
		mVec |= pvYSIGN_MASK;
		set_positive(tmp.y);
	}
	if (negative(tmp.z))
	{
		mVec |= pvZSIGN_MASK;
		set_positive(tmp.z);
	}

	// project the normal onto the plane X0=(1,0,0), Y0=(0,1,0), Z0=(0,0,1)
	float w = 126.0f / (tmp.x + tmp.y + tmp.z);
	int xbits = iFloor(tmp.x * w);
	int ybits = iFloor(tmp.y * w);

	// transform triangle (0,0)-(126,0)-(0,126) into a rectangle
	if (xbits >= 64)
	{
		xbits = 127 - xbits;
		ybits = 127 - ybits;
	}

	// pack all bits together
	mVec |= (xbits << 7);
	mVec |= ybits;

	return mVec;
}

// -----------------------------------------------------------------------------
// Decompress a 16-bit representation back into a fvec3 unit vector.
// -----------------------------------------------------------------------------
inline void decompress_normal(fvec3& vec, u16 mVec)
{
	// get x and y bits
	int xbits = ((mVec & pvTOP_MASK) >> 7);
	int ybits = (mVec & pvBOTTOM_MASK);

	// map back to the triangle (0,0)-(0,126)-(126,0)
	if ((xbits + ybits) >= 127)
	{
		xbits = 127 - xbits;
		ybits = 127 - ybits;
	}

	// fetch precomputed normalisation factor from lazy table
	const float* table = xrMath::detail::get_pvUVAdjustment();
	float uvadj = table[mVec & ~pvSIGN_MASK];

	vec.x = uvadj * float(xbits);
	vec.y = uvadj * float(ybits);
	vec.z = uvadj * float(126 - xbits - ybits);

	// restore signs
	if (mVec & pvXSIGN_MASK)
		set_negative(vec.x);
	if (mVec & pvYSIGN_MASK)
		set_negative(vec.y);
	if (mVec & pvZSIGN_MASK)
		set_negative(vec.z);
}
