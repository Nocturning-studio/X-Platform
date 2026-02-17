#include "stdafx.h"
#pragma hdrstop

#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE2 (для _mm_set_ps)

#include "..\skeletonX.h"
#include "..\skeletoncustom.h"

void __stdcall xrSkin1W_SSE(vertRender* D, vertBoned1W* S, u32 vCount, CBoneInstance* Bones)
{
	for (u32 i = 0; i < vCount; ++i)
	{
		const vertBoned1W& src = S[i];
		vertRender& dst = D[i];
		const float4x4& M = Bones[src.matrix].mRenderTransform;

		// --- Преобразование позиции (с учётом трансляции) ---
		// Вектор позиции: (x, y, z, 1.0f)
		__m128 pos = _mm_set_ps(1.0f, src.P.z, src.P.y, src.P.x);

		// Загружаем строки матрицы (предполагается row-major порядок)
		__m128 row0 = _mm_loadu_ps(&M._11); // (_11, _12, _13, _14)
		__m128 row1 = _mm_loadu_ps(&M._21); // (_21, _22, _23, _24)
		__m128 row2 = _mm_loadu_ps(&M._31); // (_31, _32, _33, _34)
		__m128 row3 = _mm_loadu_ps(&M._41); // (_41, _42, _43, _44)

		// Вычисляем: pos * M = x*row0 + y*row1 + z*row2 + w*row3 (w=1)
		__m128 res = _mm_mul_ps(_mm_shuffle_ps(pos, pos, _MM_SHUFFLE(0, 0, 0, 0)), row0);			// x*row0
		res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(pos, pos, _MM_SHUFFLE(1, 1, 1, 1)), row1)); // + y*row1
		res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(pos, pos, _MM_SHUFFLE(2, 2, 2, 2)), row2)); // + z*row2
		res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(pos, pos, _MM_SHUFFLE(3, 3, 3, 3)), row3)); // + 1*row3

		// Сохраняем x,y,z результата
		_mm_store_ss(&dst.P.x, res);
		_mm_store_ss(&dst.P.y, _mm_shuffle_ps(res, res, _MM_SHUFFLE(1, 1, 1, 1)));
		_mm_store_ss(&dst.P.z, _mm_shuffle_ps(res, res, _MM_SHUFFLE(2, 2, 2, 2)));

		// --- Преобразование нормали (без трансляции) ---
		// Вектор нормали: (x, y, z, 0.0f)
		__m128 norm = _mm_set_ps(0.0f, src.N.z, src.N.y, src.N.x);

		__m128 norm_res = _mm_mul_ps(_mm_shuffle_ps(norm, norm, _MM_SHUFFLE(0, 0, 0, 0)), row0); // x*row0
		norm_res =
			_mm_add_ps(norm_res, _mm_mul_ps(_mm_shuffle_ps(norm, norm, _MM_SHUFFLE(1, 1, 1, 1)), row1)); // + y*row1
		norm_res =
			_mm_add_ps(norm_res, _mm_mul_ps(_mm_shuffle_ps(norm, norm, _MM_SHUFFLE(2, 2, 2, 2)), row2)); // + z*row2
		// w=0, поэтому row3 не добавляем

		// Сохраняем x,y,z нормали
		_mm_store_ss(&dst.N.x, norm_res);
		_mm_store_ss(&dst.N.y, _mm_shuffle_ps(norm_res, norm_res, _MM_SHUFFLE(1, 1, 1, 1)));
		_mm_store_ss(&dst.N.z, _mm_shuffle_ps(norm_res, norm_res, _MM_SHUFFLE(2, 2, 2, 2)));

		// Копируем текстурные координаты (скалярно)
		dst.u = src.u;
		dst.v = src.v;
	}
}
