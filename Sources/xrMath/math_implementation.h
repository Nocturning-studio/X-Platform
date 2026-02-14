#pragma once

// Подключаем все типы, чтобы видеть их внутренности
#include "math_float3x3.h"
#include "math_float4x4.h"
#include "math_quaternion.h"

#include <cmath>
#include <limits>
#include <type_traits>

XRAY_BEGIN

// -----------------------------------------------------------------------------
// Quaternion -> Matrix 3x3
// -----------------------------------------------------------------------------
template <class T> inline Matrix3x3<T>& Matrix3x3<T>::set(const Quaternion<T>& Q)
{
	T xx = Q.x * Q.x, yy = Q.y * Q.y, zz = Q.z * Q.z;
	T xy = Q.x * Q.y, xz = Q.x * Q.z, yz = Q.y * Q.z;
	T wx = Q.w * Q.x, wy = Q.w * Q.y, wz = Q.w * Q.z;

	_11 = 1 - 2 * (yy + zz);
	_12 = 2 * (xy - wz);
	_13 = 2 * (xz + wy);
	_21 = 2 * (xy + wz);
	_22 = 1 - 2 * (xx + zz);
	_23 = 2 * (yz - wx);
	_31 = 2 * (xz - wy);
	_32 = 2 * (yz + wx);
	_33 = 1 - 2 * (xx + yy);

	return *this;
}

// -----------------------------------------------------------------------------
// Quaternion -> Matrix 4x4
// -----------------------------------------------------------------------------
template <class T> inline Matrix4x4<T>& Matrix4x4<T>::set(const Quaternion<T>& Q)
{
	identity(); // Сбрасываем позицию и масштаб в дефолт

	T xx = Q.x * Q.x, yy = Q.y * Q.y, zz = Q.z * Q.z;
	T xy = Q.x * Q.y, xz = Q.x * Q.z, yz = Q.y * Q.z;
	T wx = Q.w * Q.x, wy = Q.w * Q.y, wz = Q.w * Q.z;

	_11 = 1 - 2 * (yy + zz);
	_12 = 2 * (xy - wz);
	_13 = 2 * (xz + wy);
	_21 = 2 * (xy + wz);
	_22 = 1 - 2 * (xx + zz);
	_23 = 2 * (yz - wx);
	_31 = 2 * (xz - wy);
	_32 = 2 * (yz + wx);
	_33 = 1 - 2 * (xx + yy);

	return *this;
}

// -----------------------------------------------------------------------------
// Matrix 3x3 -> Quaternion
// -----------------------------------------------------------------------------
template <class T> inline Quaternion<T>& Quaternion<T>::set(const Matrix3x3<T>& M)
{
	T tr = M._11 + M._22 + M._33; // След матрицы (сумма диагонали)

	if (tr > 0)
	{
		T S = std::sqrt(tr + 1.0f) * 2; // S=4*qw
		w = 0.25f * S;
		x = (M._32 - M._23) / S;
		y = (M._13 - M._31) / S;
		z = (M._21 - M._12) / S;
	}
	else if ((M._11 > M._22) && (M._11 > M._33))
	{
		T S = std::sqrt(1.0f + M._11 - M._22 - M._33) * 2; // S=4*qx
		w = (M._32 - M._23) / S;
		x = 0.25f * S;
		y = (M._12 + M._21) / S;
		z = (M._13 + M._31) / S;
	}
	else if (M._22 > M._33)
	{
		T S = std::sqrt(1.0f + M._22 - M._11 - M._33) * 2; // S=4*qy
		w = (M._13 - M._31) / S;
		x = (M._12 + M._21) / S;
		y = 0.25f * S;
		z = (M._23 + M._32) / S;
	}
	else
	{
		T S = std::sqrt(1.0f + M._33 - M._11 - M._22) * 2; // S=4*qz
		w = (M._21 - M._12) / S;
		x = (M._13 + M._31) / S;
		y = (M._23 + M._32) / S;
		z = 0.25f * S;
	}
	return *this;
}

// -----------------------------------------------------------------------------
// Matrix 4x4 -> Quaternion
// -----------------------------------------------------------------------------
template <class T> inline Quaternion<T>& Quaternion<T>::set(const Matrix4x4<T>& M)
{
	// Используем ту же логику, что и для 3x3, так как вращение в 4x4 находится в верхнем левом углу
	T tr = M._11 + M._22 + M._33;

	if (tr > 0)
	{
		T S = std::sqrt(tr + 1.0f) * 2;
		w = 0.25f * S;
		x = (M._32 - M._23) / S;
		y = (M._13 - M._31) / S;
		z = (M._21 - M._12) / S;
	}
	else if ((M._11 > M._22) && (M._11 > M._33))
	{
		T S = std::sqrt(1.0f + M._11 - M._22 - M._33) * 2;
		w = (M._32 - M._23) / S;
		x = 0.25f * S;
		y = (M._12 + M._21) / S;
		z = (M._13 + M._31) / S;
	}
	else if (M._22 > M._33)
	{
		T S = std::sqrt(1.0f + M._22 - M._11 - M._33) * 2;
		w = (M._13 - M._31) / S;
		x = (M._12 + M._21) / S;
		y = 0.25f * S;
		z = (M._23 + M._32) / S;
	}
	else
	{
		T S = std::sqrt(1.0f + M._33 - M._11 - M._22) * 2;
		w = (M._21 - M._12) / S;
		x = (M._13 + M._31) / S;
		y = (M._23 + M._32) / S;
		z = 0.25f * S;
	}
	return *this;
}

XRAY_END
