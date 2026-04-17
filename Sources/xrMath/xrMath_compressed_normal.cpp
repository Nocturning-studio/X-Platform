#include "pch.h"
#include "xrMath_compressed_normal.h"
#include <cmath>

// Definition of the lookup table
XRMATH_API float pvUVAdjustment[0x2000];

XRMATH_API void initialize_normal_compression_stats()
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
		pvUVAdjustment[idx] = 1.0f / std::sqrt(y * y + z * z + x * x);
	}
}
