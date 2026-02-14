#pragma once
#include "xrMathCommon.h"
#include "math_float3.h"

#include <cmath>
#include <limits>
#include <type_traits>

XRAY_BEGIN

template <class T> struct Matrix3x3;
template <class T> struct Matrix4x4;

template <class T> struct Quaternion
{
	using Self = Quaternion<T>;
	using Vector3 = Vector3<T>;

	T x, y, z, w;

	// --- Конструкторы и базовые операции ---

	Self& set(const Matrix3x3<T>& M);
	Self& set(const Matrix4x4<T>& M);

	Self& set(T _x, T _y, T _z, T _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
		return *this;
	}

	Self& set(const Self& q)
	{
		x = q.x;
		y = q.y;
		z = q.z;
		w = q.w;
		return *this;
	}

	// Единичное вращение (нет вращения)
	Self& identity()
	{
		x = 0;
		y = 0;
		z = 0;
		w = 1;
		return *this;
	}

	// Инверсия (обратное вращение)
	Self& inverse()
	{
		x = -x;
		y = -y;
		z = -z;
		return *this;
	}

	// --- Математика ---

	T magnitude() const
	{
		return std::sqrt(x * x + y * y + z * z + w * w);
	}

	Self& normalize()
	{
		T m = magnitude();
		if (m > std::numeric_limits<T>::epsilon())
		{
			T inv = 1.0f / m;
			x *= inv;
			y *= inv;
			z *= inv;
			w *= inv;
		}
		return *this;
	}

	// Умножение кватернионов (q1 * q2)
	// Результат: поворот q2, затем поворот q1
	Self& mul(const Self& A, const Self& B)
	{
		// Формула Грассмана
		T _w = A.w * B.w - A.x * B.x - A.y * B.y - A.z * B.z;
		T _x = A.w * B.x + A.x * B.w + A.y * B.z - A.z * B.y;
		T _y = A.w * B.y - A.x * B.z + A.y * B.w + A.z * B.x;
		T _z = A.w * B.z + A.x * B.y - A.y * B.x + A.z * B.w;

		x = _x;
		y = _y;
		z = _z;
		w = _w;
		return *this;
	}

	// Создание вращения вокруг оси
	Self& rotation(Vector3 axis, T angle)
	{
		T halfAngle = angle * 0.5f;
		T sinHalf = std::sin(halfAngle);

		axis.normalize();

		w = std::cos(halfAngle);
		x = axis.x * sinHalf;
		y = axis.y * sinHalf;
		z = axis.z * sinHalf;
		return *this;
	}

	// SLERP - Сферическая линейная интерполяция
	// t = [0..1]
	Self& slerp(const Self& A, const Self& B, T t)
	{
		T cosTheta = A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;

		// Если cosTheta < 0, кватернионы направлены в разные стороны полусферы.
		// Чтобы пойти по кратчайшему пути, инвертируем один из них.
		T sign = 1.0f;
		if (cosTheta < 0.0f)
		{
			cosTheta = -cosTheta;
			sign = -1.0f;
		}

		T scale0, scale1;

		if ((1.0f - cosTheta) > EPS)
		{
			// Стандартный случай
			T theta = std::acos(cosTheta);
			T sinTheta = std::sin(theta);
			T invSinTheta = 1.0f / sinTheta;

			scale0 = std::sin((1.0f - t) * theta) * invSinTheta;
			scale1 = std::sin(t * theta) * invSinTheta;
		}
		else
		{
			// Кватернионы очень близки, используем линейную интерполяцию (LERP)
			scale0 = 1.0f - t;
			scale1 = t;
		}

		scale1 *= sign;

		x = A.x * scale0 + B.x * scale1;
		y = A.y * scale0 + B.y * scale1;
		z = A.z * scale0 + B.z * scale1;
		w = A.w * scale0 + B.w * scale1;

		return *this;
	}
};

using quaternion = Quaternion<float>;

XRAY_END
