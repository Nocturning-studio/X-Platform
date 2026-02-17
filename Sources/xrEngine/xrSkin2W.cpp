#include "stdafx.h"
#pragma hdrstop

#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE2

#include "..\skeletonX.h"
#include "..\skeletoncustom.h"

void __stdcall xrSkin2W_SSE(vertRender* D, vertBoned2W* S, u32 vCount, CBoneInstance* Bones)
{
	for (u32 i = 0; i < vCount; ++i)
	{
		vertBoned2W& src = S[i];
		vertRender& dst = D[i];

		u16 idx0 = src.matrix0;
		u16 idx1 = src.matrix1;

		const float4x4& M0 = Bones[idx0].mRenderTransform;
		__m128 srcP = _mm_set_ps(1.0f, src.P.z, src.P.y, src.P.x);
		__m128 srcN = _mm_set_ps(0.0f, src.N.z, src.N.y, src.N.x);

		__m128 m0_row0 = _mm_loadu_ps(&M0._11);
		__m128 m0_row1 = _mm_loadu_ps(&M0._21);
		__m128 m0_row2 = _mm_loadu_ps(&M0._31);
		__m128 m0_row3 = _mm_loadu_ps(&M0._41);

		__m128 pos0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(srcP, srcP, _MM_SHUFFLE(0, 0, 0, 0)), m0_row0),
											_mm_mul_ps(_mm_shuffle_ps(srcP, srcP, _MM_SHUFFLE(1, 1, 1, 1)), m0_row1)),
								 _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(srcP, srcP, _MM_SHUFFLE(2, 2, 2, 2)), m0_row2),
											_mm_mul_ps(_mm_shuffle_ps(srcP, srcP, _MM_SHUFFLE(3, 3, 3, 3)), m0_row3)));

		__m128 norm0 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(srcN, srcN, _MM_SHUFFLE(0, 0, 0, 0)), m0_row0),
											 _mm_mul_ps(_mm_shuffle_ps(srcN, srcN, _MM_SHUFFLE(1, 1, 1, 1)), m0_row1)),
								  _mm_mul_ps(_mm_shuffle_ps(srcN, srcN, _MM_SHUFFLE(2, 2, 2, 2)), m0_row2));

		__m128 final_pos, final_norm;

		if (idx0 == idx1)
		{
			final_pos = pos0;
			final_norm = norm0;
		}
		else
		{
			const float4x4& M1 = Bones[idx1].mRenderTransform;
			__m128 m1_row0 = _mm_loadu_ps(&M1._11);
			__m128 m1_row1 = _mm_loadu_ps(&M1._21);
			__m128 m1_row2 = _mm_loadu_ps(&M1._31);
			__m128 m1_row3 = _mm_loadu_ps(&M1._41);

			__m128 pos1 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(srcP, srcP, _MM_SHUFFLE(0, 0, 0, 0)), m1_row0),
									  _mm_mul_ps(_mm_shuffle_ps(srcP, srcP, _MM_SHUFFLE(1, 1, 1, 1)), m1_row1)),
						   _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(srcP, srcP, _MM_SHUFFLE(2, 2, 2, 2)), m1_row2),
									  _mm_mul_ps(_mm_shuffle_ps(srcP, srcP, _MM_SHUFFLE(3, 3, 3, 3)), m1_row3)));

			__m128 norm1 =
				_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(srcN, srcN, _MM_SHUFFLE(0, 0, 0, 0)), m1_row0),
									  _mm_mul_ps(_mm_shuffle_ps(srcN, srcN, _MM_SHUFFLE(1, 1, 1, 1)), m1_row1)),
						   _mm_mul_ps(_mm_shuffle_ps(srcN, srcN, _MM_SHUFFLE(2, 2, 2, 2)), m1_row2));

			__m128 weight = _mm_set_ps1(src.w);
			__m128 one_minus_weight = _mm_sub_ps(_mm_set_ps1(1.0f), weight);

			final_pos = _mm_add_ps(_mm_mul_ps(pos0, one_minus_weight), _mm_mul_ps(pos1, weight));
			final_norm = _mm_add_ps(_mm_mul_ps(norm0, one_minus_weight), _mm_mul_ps(norm1, weight));
		}

		_mm_store_ss(&dst.P.x, final_pos);
		_mm_store_ss(&dst.P.y, _mm_shuffle_ps(final_pos, final_pos, _MM_SHUFFLE(1, 1, 1, 1)));
		_mm_store_ss(&dst.P.z, _mm_shuffle_ps(final_pos, final_pos, _MM_SHUFFLE(2, 2, 2, 2)));

		_mm_store_ss(&dst.N.x, final_norm);
		_mm_store_ss(&dst.N.y, _mm_shuffle_ps(final_norm, final_norm, _MM_SHUFFLE(1, 1, 1, 1)));
		_mm_store_ss(&dst.N.z, _mm_shuffle_ps(final_norm, final_norm, _MM_SHUFFLE(2, 2, 2, 2)));

		dst.u = src.u;
		dst.v = src.v;
	}
}
