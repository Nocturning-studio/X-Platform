#pragma once
#include "math_float3.h"
#include "math_float4.h"
#include "math_quaternion.h"

#include <cmath>
#include <limits>
#include <type_traits>
#include <algorithm> // std::swap

XRAY_BEGIN

template <class T> struct Quaternion;

template <class T> struct Matrix4x4
{
	using Self = Matrix4x4<T>;
	using Vector3 = Vector3<T>;
	using Vector4 = Vector4<T>;

	union {
		struct
		{
			T _11, _12, _13, _14;
			T _21, _22, _23, _24;
			T _31, _32, _33, _34;
			T _41, _42, _43, _44;
		};
		struct
		{
			Vector3 i; // правая ось (x‑basis)
			T _14_pad;
			Vector3 j; // верхняя ось (y‑basis)
			T _24_pad;
			Vector3 k; // ось направления (z‑basis)
			T _34_pad;
			Vector3 c; // перемещение (translation)
			T _44_pad;
		};
		T m[4][4];
	};

	Self& set(const Quaternion<T>& Q);

	Self& identity()
	{
		_11 = 1; _12 = 0; _13 = 0; _14 = 0;
		_21 = 0; _22 = 1; _23 = 0; _24 = 0;
		_31 = 0; _32 = 0; _33 = 1; _34 = 0;
		_41 = 0; _42 = 0; _43 = 0; _44 = 1;
		return *this;
	}

	// Установка перемещения (Translation)
	Self& translate(const Vector3& pos)
	{
		identity();
		_41 = pos.x;
		_42 = pos.y;
		_43 = pos.z;
		return *this;
	}
	
	// Установка масштаба (Scale)
	Self& scale(const Vector3& factor)
	{
		identity();
		_11 = factor.x;
		_22 = factor.y;
		_33 = factor.z;
		return *this;
	}

	// Умножение: This = A * B
	Self& mul(const Self& A, const Self& B)
	{
		Self R;
		for (int i = 0; i < 4; i++) // Строка A
		{
			for (int j = 0; j < 4; j++) // Столбец B
			{
				R.m[i][j] = A.m[i][0] * B.m[0][j] + 
							A.m[i][1] * B.m[1][j] + 
							A.m[i][2] * B.m[2][j] + 
							A.m[i][3] * B.m[3][j];
			}
		}
		*this = R;
		return *this;
	}

	// Трансформация точки (учитывает перемещение _41, _42, _43)
	// V_out = V_in * Matrix
	void transform(Vector3& v) const
	{
		// Предполагаем w=1
		T tx = v.x * _11 + v.y * _21 + v.z * _31 + _41;
		T ty = v.x * _12 + v.y * _22 + v.z * _32 + _42;
		T tz = v.x * _13 + v.y * _23 + v.z * _33 + _43;
		// Если это проекционная матрица, здесь нужно деление на w, но для World матриц w=1
		v.x = tx; v.y = ty; v.z = tz;
	}
	
	// Трансформация направления (ИГНОРИРУЕТ перемещение)
	// Используется для нормалей
	void transform_dir(Vector3& v) const
	{
		T tx = v.x * _11 + v.y * _21 + v.z * _31;
		T ty = v.x * _12 + v.y * _22 + v.z * _32;
		T tz = v.x * _13 + v.y * _23 + v.z * _33;
		v.x = tx; v.y = ty; v.z = tz;
	}

	Self& set_rotation_translation(const Quaternion<T>& Q, const Vector3& V)
	{
		set(Q); // устанавливаем поворот
		c = V;	// устанавливаем трансляцию
		return *this;
	}

	// --- Копирование ---
	Self& set(const Self& a)
	{
		i = a.i;
		_14_pad = a._14_pad;
		j = a.j;
		_24_pad = a._24_pad;
		k = a.k;
		_34_pad = a._34_pad;
		c = a.c;
		_44_pad = a._44_pad;
		return *this;
	}

	// --- Сравнение с эпсилон ---
	bool similar(const Self& other, T eps = static_cast<T>(EPS_L)) const
	{
		return std::abs(_11 - other._11) < eps && std::abs(_12 - other._12) < eps && std::abs(_13 - other._13) < eps &&
			   std::abs(_14 - other._14) < eps && std::abs(_21 - other._21) < eps && std::abs(_22 - other._22) < eps &&
			   std::abs(_23 - other._23) < eps && std::abs(_24 - other._24) < eps && std::abs(_31 - other._31) < eps &&
			   std::abs(_32 - other._32) < eps && std::abs(_33 - other._33) < eps && std::abs(_34 - other._34) < eps &&
			   std::abs(_41 - other._41) < eps && std::abs(_42 - other._42) < eps && std::abs(_43 - other._43) < eps &&
			   std::abs(_44 - other._44) < eps;
	}

	// --- Установка по векторам i, j, k, c ---
	Self& set(const Vector3& right, const Vector3& up, const Vector3& dir, const Vector3& pos)
	{
		i = right;
		_14_pad = 0;
		j = up;
		_24_pad = 0;
		k = dir;
		_34_pad = 0;
		c = pos;
		_44_pad = 1;
		return *this;
	}

	// --- Умножение 4x4 без проекционной составляющей (быстрее) ---
	Self& mul_43(const Self& A, const Self& B)
	{
		// Предполагается, что последний столбец = (0,0,0,1)
		m[0][0] = A.m[0][0] * B.m[0][0] + A.m[1][0] * B.m[0][1] + A.m[2][0] * B.m[0][2];
		m[0][1] = A.m[0][1] * B.m[0][0] + A.m[1][1] * B.m[0][1] + A.m[2][1] * B.m[0][2];
		m[0][2] = A.m[0][2] * B.m[0][0] + A.m[1][2] * B.m[0][1] + A.m[2][2] * B.m[0][2];
		m[0][3] = 0;

		m[1][0] = A.m[0][0] * B.m[1][0] + A.m[1][0] * B.m[1][1] + A.m[2][0] * B.m[1][2];
		m[1][1] = A.m[0][1] * B.m[1][0] + A.m[1][1] * B.m[1][1] + A.m[2][1] * B.m[1][2];
		m[1][2] = A.m[0][2] * B.m[1][0] + A.m[1][2] * B.m[1][1] + A.m[2][2] * B.m[1][2];
		m[1][3] = 0;

		m[2][0] = A.m[0][0] * B.m[2][0] + A.m[1][0] * B.m[2][1] + A.m[2][0] * B.m[2][2];
		m[2][1] = A.m[0][1] * B.m[2][0] + A.m[1][1] * B.m[2][1] + A.m[2][1] * B.m[2][2];
		m[2][2] = A.m[0][2] * B.m[2][0] + A.m[1][2] * B.m[2][1] + A.m[2][2] * B.m[2][2];
		m[2][3] = 0;

		m[3][0] = A.m[0][0] * B.m[3][0] + A.m[1][0] * B.m[3][1] + A.m[2][0] * B.m[3][2] + A.m[3][0];
		m[3][1] = A.m[0][1] * B.m[3][0] + A.m[1][1] * B.m[3][1] + A.m[2][1] * B.m[3][2] + A.m[3][1];
		m[3][2] = A.m[0][2] * B.m[3][0] + A.m[1][2] * B.m[3][1] + A.m[2][2] * B.m[3][2] + A.m[3][2];
		m[3][3] = 1;
		return *this;
	}

	// --- Умножение текущей матрицы на другую справа (this = this * B) ---
	Self& mulB_44(const Self& B)
	{
		Self A = *this;
		mul(A, B);
		return *this;
	}
	Self& mulA_44(const Self& A)
	{
		Self B = *this;
		mul(A, B);
		return *this;
	}
	Self& mulB_43(const Self& B)
	{
		Self A = *this;
		mul_43(A, B);
		return *this;
	}
	Self& mulA_43(const Self& A)
	{
		Self B = *this;
		mul_43(A, B);
		return *this;
	}

	// --- Обращение матрицы (4x3) ---
	Self& invert(const Self& a)
	{
		T det = a._11 * (a._22 * a._33 - a._23 * a._32) - a._12 * (a._21 * a._33 - a._23 * a._31) +
				a._13 * (a._21 * a._32 - a._22 * a._31);
		// Проверка на нуль оставлена пользователю (можно добавить assert)
		T invDet = T(1) / det;

		_11 = invDet * (a._22 * a._33 - a._23 * a._32);
		_12 = -invDet * (a._12 * a._33 - a._13 * a._32);
		_13 = invDet * (a._12 * a._23 - a._13 * a._22);
		_14 = 0;

		_21 = -invDet * (a._21 * a._33 - a._23 * a._31);
		_22 = invDet * (a._11 * a._33 - a._13 * a._31);
		_23 = -invDet * (a._11 * a._23 - a._13 * a._21);
		_24 = 0;

		_31 = invDet * (a._21 * a._32 - a._22 * a._31);
		_32 = -invDet * (a._11 * a._32 - a._12 * a._31);
		_33 = invDet * (a._11 * a._22 - a._12 * a._21);
		_34 = 0;

		_41 = -(a._41 * _11 + a._42 * _21 + a._43 * _31);
		_42 = -(a._41 * _12 + a._42 * _22 + a._43 * _32);
		_43 = -(a._41 * _13 + a._42 * _23 + a._43 * _33);
		_44 = 1;
		return *this;
	}

	// --- Обращение с проверкой (возвращает false при вырожденности) ---
	bool invert_b(const Self& a)
	{
		T det = a._11 * (a._22 * a._33 - a._23 * a._32) - a._12 * (a._21 * a._33 - a._23 * a._31) +
				a._13 * (a._21 * a._32 - a._22 * a._31);
		if (std::abs(det) <= std::numeric_limits<T>::epsilon())
			return false;
		T invDet = T(1) / det;
		_11 = invDet * (a._22 * a._33 - a._23 * a._32);
		_12 = -invDet * (a._12 * a._33 - a._13 * a._32);
		_13 = invDet * (a._12 * a._23 - a._13 * a._22);
		_14 = 0;
		_21 = -invDet * (a._21 * a._33 - a._23 * a._31);
		_22 = invDet * (a._11 * a._33 - a._13 * a._31);
		_23 = -invDet * (a._11 * a._23 - a._13 * a._21);
		_24 = 0;
		_31 = invDet * (a._21 * a._32 - a._22 * a._31);
		_32 = -invDet * (a._11 * a._32 - a._12 * a._31);
		_33 = invDet * (a._11 * a._22 - a._12 * a._21);
		_34 = 0;
		_41 = -(a._41 * _11 + a._42 * _21 + a._43 * _31);
		_42 = -(a._41 * _12 + a._42 * _22 + a._43 * _32);
		_43 = -(a._41 * _13 + a._42 * _23 + a._43 * _33);
		_44 = 1;
		return true;
	}

	Self& invert()
	{
		Self a = *this;
		return invert(a);
	}

	// --- Транспонирование ---
	Self& transpose(const Self& src)
	{
		_11 = src._11;
		_12 = src._21;
		_13 = src._31;
		_14 = src._41;
		_21 = src._12;
		_22 = src._22;
		_23 = src._32;
		_24 = src._42;
		_31 = src._13;
		_32 = src._23;
		_33 = src._33;
		_34 = src._43;
		_41 = src._14;
		_42 = src._24;
		_43 = src._34;
		_44 = src._44;
		return *this;
	}
	Self& transpose()
	{
		Self tmp = *this;
		return transpose(tmp);
	}

	// --- Установка только перемещения (без изменения поворота/масштаба) ---
	Self& set_translation(const Vector3& pos)
	{
		c = pos;
		return *this;
	}
	Self& add_translation(const Vector3& delta)
	{
		c += delta;
		return *this;
	}

	// --- Матрицы поворота вокруг базовых осей ---
	Self& rotate_x(T angle)
	{
		T c = std::cos(angle);
		T s = std::sin(angle);
		_11 = 1;
		_12 = 0;
		_13 = 0;
		_14 = 0;
		_21 = 0;
		_22 = c;
		_23 = s;
		_24 = 0;
		_31 = 0;
		_32 = -s;
		_33 = c;
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}
	Self& rotate_y(T angle)
	{
		T c = std::cos(angle);
		T s = std::sin(angle);
		_11 = c;
		_12 = 0;
		_13 = -s;
		_14 = 0;
		_21 = 0;
		_22 = 1;
		_23 = 0;
		_24 = 0;
		_31 = s;
		_32 = 0;
		_33 = c;
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}
	Self& rotate_z(T angle)
	{
		T c = std::cos(angle);
		T s = std::sin(angle);
		_11 = c;
		_12 = s;
		_13 = 0;
		_14 = 0;
		_21 = -s;
		_22 = c;
		_23 = 0;
		_24 = 0;
		_31 = 0;
		_32 = 0;
		_33 = 1;
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}

	// --- Поворот вокруг произвольной оси (ось должна быть единичной) ---
	Self& rotation(const Vector3& axis, T angle)
	{
		T c = std::cos(angle);
		T s = std::sin(angle);
		T t = T(1) - c;

		_11 = t * axis.x * axis.x + c;
		_12 = t * axis.x * axis.y + s * axis.z;
		_13 = t * axis.x * axis.z - s * axis.y;
		_14 = 0;

		_21 = t * axis.x * axis.y - s * axis.z;
		_22 = t * axis.y * axis.y + c;
		_23 = t * axis.y * axis.z + s * axis.x;
		_24 = 0;

		_31 = t * axis.x * axis.z + s * axis.y;
		_32 = t * axis.y * axis.z - s * axis.x;
		_33 = t * axis.z * axis.z + c;
		_34 = 0;

		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}

	// --- Матрица, задающая направление (look rotation) ---
	Self& rotation(const Vector3& dir, const Vector3& up)
	{
		Vector3 right;
		right.cross(up, dir);
		right.normalize();
		Vector3 newUp;
		newUp.cross(dir, right);
		// right, newUp, dir должны быть ортонормированы
		i = right;
		_14_pad = 0;
		j = newUp;
		_24_pad = 0;
		k = dir;
		_34_pad = 0;
		c.set(0, 0, 0);
		_44_pad = 1;
		return *this;
	}

	// --- Перестановка осей (для преобразования между системами координат) ---
	Self& map_xyz()
	{
		identity();
		return *this;
	} // уже единичная
	Self& map_xzy()
	{
		_11 = 1;
		_12 = 0;
		_13 = 0;
		_14 = 0;
		_21 = 0;
		_22 = 0;
		_23 = 1;
		_24 = 0;
		_31 = 0;
		_32 = 1;
		_33 = 0;
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}
	Self& map_yxz()
	{
		_11 = 0;
		_12 = 1;
		_13 = 0;
		_14 = 0;
		_21 = 1;
		_22 = 0;
		_23 = 0;
		_24 = 0;
		_31 = 0;
		_32 = 0;
		_33 = 1;
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}
	Self& map_yzx()
	{
		_11 = 0;
		_12 = 1;
		_13 = 0;
		_14 = 0;
		_21 = 0;
		_22 = 0;
		_23 = 1;
		_24 = 0;
		_31 = 1;
		_32 = 0;
		_33 = 0;
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}
	Self& map_zxy()
	{
		_11 = 0;
		_12 = 0;
		_13 = 1;
		_14 = 0;
		_21 = 1;
		_22 = 0;
		_23 = 0;
		_24 = 0;
		_31 = 0;
		_32 = 1;
		_33 = 0;
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}
	Self& map_zyx()
	{
		_11 = 0;
		_12 = 0;
		_13 = 1;
		_14 = 0;
		_21 = 0;
		_22 = 1;
		_23 = 0;
		_24 = 0;
		_31 = 1;
		_32 = 0;
		_33 = 0;
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = 0;
		_44 = 1;
		return *this;
	}

	// --- Зеркалирование (отражение) ---
	Self& mirror_x()
	{
		identity();
		_11 = -1;
		return *this;
	}
	Self& mirror_x_over()
	{
		_11 = -1;
		return *this;
	}
	Self& mirror_x_add()
	{
		_11 *= -1;
		return *this;
	}
	Self& mirror_y()
	{
		identity();
		_22 = -1;
		return *this;
	}
	Self& mirror_y_over()
	{
		_22 = -1;
		return *this;
	}
	Self& mirror_y_add()
	{
		_22 *= -1;
		return *this;
	}
	Self& mirror_z()
	{
		identity();
		_33 = -1;
		return *this;
	}
	Self& mirror_z_over()
	{
		_33 = -1;
		return *this;
	}
	Self& mirror_z_add()
	{
		_33 *= -1;
		return *this;
	}

	// --- Умножение/деление на скаляр ---
	Self& mul(const Self& A, T scalar)
	{
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				m[i][j] = A.m[i][j] * scalar;
		return *this;
	}
	Self& mul(T scalar)
	{
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				m[i][j] *= scalar;
		return *this;
	}
	Self& div(const Self& A, T scalar)
	{
		T inv = T(1) / scalar;
		return mul(A, inv);
	}
	Self& div(T scalar)
	{
		T inv = T(1) / scalar;
		return mul(inv);
	}

	// --- Проекционные матрицы (только для float, так как используют tanf и т.д.) ---
	Self& build_projection(T fov, T aspect, T nearZ, T farZ)
	{
		static_assert(std::is_same<T, float>::value, "build_projection is only implemented for float");
		T halfTan = std::tan(fov * T(0.5));
		return build_projection_half_atan(halfTan, aspect, nearZ, farZ);
	}
	Self& build_projection_half_atan(T halfFovTan, T aspect, T nearZ, T farZ)
	{
		static_assert(std::is_same<T, float>::value, "build_projection_half_atan is only implemented for float");
		T w = T(1) / halfFovTan;
		T h = aspect * w; // в старой реализации было наоборот: w = aspect * cot, h = cot
		// Уточним: обычно w = cot / aspect? Проверим старую реализацию:
		// В старой: cot = 1/HAT; w = aspect * cot; h = 1 * cot;
		// Значит, w = aspect / HAT, h = 1 / HAT.
		// Здесь halfFovTan = HAT.
		T cot = T(1) / halfFovTan;
		T w_ = aspect * cot;
		T h_ = cot;
		T Q = farZ / (farZ - nearZ);

		_11 = w_;
		_12 = 0;
		_13 = 0;
		_14 = 0;
		_21 = 0;
		_22 = h_;
		_23 = 0;
		_24 = 0;
		_31 = 0;
		_32 = 0;
		_33 = Q;
		_34 = 1;
		_41 = 0;
		_42 = 0;
		_43 = -Q * nearZ;
		_44 = 0;
		return *this;
	}
	Self& build_ortho_projection(T width, T height, T nearZ, T farZ)
	{
		static_assert(std::is_same<T, float>::value, "build_ortho_projection is only implemented for float");
		_11 = T(2) / width;
		_12 = 0;
		_13 = 0;
		_14 = 0;
		_21 = 0;
		_22 = T(2) / height;
		_23 = 0;
		_24 = 0;
		_31 = 0;
		_32 = 0;
		_33 = T(1) / (farZ - nearZ);
		_34 = 0;
		_41 = 0;
		_42 = 0;
		_43 = nearZ / (nearZ - farZ);
		_44 = 1;
		return *this;
	}

	// --- Видовая матрица (look at) ---
	Self& build_camera(const Vector3& from, const Vector3& at, const Vector3& worldUp)
	{
		Vector3 view = at - from;
		view.normalize();
		return build_camera_dir(from, view, worldUp);
	}
	Self& build_camera_dir(const Vector3& from, const Vector3& viewDir, const Vector3& worldUp)
	{
		// Вычисляем правый и верхний векторы
		Vector3 right;
		right.cross(worldUp, viewDir);
		right.normalize();
		Vector3 up;
		up.cross(viewDir, right);
		// up уже нормализован, если входные были ортонормированы

		_11 = right.x;
		_12 = up.x;
		_13 = viewDir.x;
		_14 = 0;
		_21 = right.y;
		_22 = up.y;
		_23 = viewDir.y;
		_24 = 0;
		_31 = right.z;
		_32 = up.z;
		_33 = viewDir.z;
		_34 = 0;

		_41 = -from.dot(right);
		_42 = -from.dot(up);
		_43 = -from.dot(viewDir);
		_44 = 1;
		return *this;
	}

	// --- Интерполяция (inertion) ---
	Self& inertion(const Self& target, T factor)
	{
		T inv = T(1) - factor;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				m[i][j] = m[i][j] * factor + target.m[i][j] * inv;
		return *this;
	}

	// --- Трансформации с разными типами аргументов ---
	void transform(Vector2<T>& dest, const Vector3& v) const
	{ // tiny32
		dest.x = v.x * _11 + v.y * _21 + v.z * _31 + _41;
		dest.y = v.x * _12 + v.y * _22 + v.z * _32 + _42;
	}
	void transform(Vector3& dest, const Vector2<T>& v) const
	{ // tiny23
		dest.x = v.x * _11 + v.y * _21 + _41;
		dest.y = v.x * _12 + v.y * _22 + _42;
		dest.z = v.x * _13 + v.y * _23 + _43;
	}
	void transform_proj(Vector4& dest, const Vector3& v) const
	{
		// Полное преобразование с проективным делением
		T w = v.x * _14 + v.y * _24 + v.z * _34 + _44;
		dest.x = (v.x * _11 + v.y * _21 + v.z * _31 + _41) / w;
		dest.y = (v.x * _12 + v.y * _22 + v.z * _32 + _42) / w;
		dest.z = (v.x * _13 + v.y * _23 + v.z * _33 + _43) / w;
		dest.w = w;
	}
	void transform_proj(Vector3& dest, const Vector3& v) const
	{
		T w = v.x * _14 + v.y * _24 + v.z * _34 + _44;
		T invW = T(1) / w;
		dest.x = (v.x * _11 + v.y * _21 + v.z * _31 + _41) * invW;
		dest.y = (v.x * _12 + v.y * _22 + v.z * _32 + _42) * invW;
		dest.z = (v.x * _13 + v.y * _23 + v.z * _33 + _43) * invW;
	}

	// --- Работа с углами HPB (Heading-Pitch-Bank) / XYZ ---
	Self& set_hpb(T heading, T pitch, T bank)
	{
		T ch = std::cos(heading), sh = std::sin(heading);
		T cp = std::cos(pitch), sp = std::sin(pitch);
		T cb = std::cos(bank), sb = std::sin(bank);

		T cc = ch * cb;
		T cs = ch * sb;
		T sc = sh * cb;
		T ss = sh * sb;

		i.x = cc - sp * ss;
		i.y = -cp * sb;
		i.z = sp * cs + sc;
		j.x = sp * sc + cs;
		j.y = cp * cb;
		j.z = ss - sp * cc;
		k.x = -cp * sh;
		k.y = sp;
		k.z = cp * ch;
		c.set(0, 0, 0);
		_14_pad = _24_pad = _34_pad = 0;
		_44_pad = 1;
		return *this;
	}
	Self& set_xyz(T x, T y, T z)
	{
		return set_hpb(y, x, z);
	}
	Self& set_xyz(const Vector3& xyz)
	{
		return set_hpb(xyz.y, xyz.x, xyz.z);
	}
	Self& set_xyz_inv(T x, T y, T z)
	{
		return set_hpb(-y, -x, -z);
	}
	Self& set_xyz_inv(const Vector3& xyz)
	{
		return set_hpb(-xyz.y, -xyz.x, -xyz.z);
	}

	void get_hpb(T& heading, T& pitch, T& bank) const
	{
		T cy = std::sqrt(j.y * j.y + i.y * i.y);
		const T eps = std::numeric_limits<T>::epsilon();
		if (cy > T(16) * eps)
		{
			heading = -std::atan2(k.x, k.z);
			pitch = -std::atan2(-k.y, cy);
			bank = -std::atan2(i.y, j.y);
		}
		else
		{
			heading = -std::atan2(-i.z, i.x);
			pitch = -std::atan2(-k.y, cy);
			bank = 0;
		}
	}
	void get_hpb(Vector3& hpb) const
	{
		get_hpb(hpb.x, hpb.y, hpb.z);
	}
	void get_xyz(T& x, T& y, T& z) const
	{
		get_hpb(y, x, z);
	}
	void get_xyz(Vector3& xyz) const
	{
		get_xyz(xyz.x, xyz.y, xyz.z);
	}
	void get_xyz_inv(T& x, T& y, T& z) const
	{
		get_hpb(y, x, z);
		x = -x;
		y = -y;
		z = -z;
	}
	void get_xyz_inv(Vector3& xyz) const
	{
		get_xyz(xyz.x, xyz.y, xyz.z);
		xyz.mul(-1);
	}
};

// Определения конкретных типов
using float4x4 = Matrix4x4<float>;

// --- Внешняя функция проверки на NaN/Inf ---
template <class T> inline bool valid(const Matrix4x4<T>& m)
{
	return valid(m.i) && valid(m._14_pad) && valid(m.j) && valid(m._24_pad) && valid(m.k) && valid(m._34_pad) &&
		   valid(m.c) && valid(m._44_pad);
}

XRAY_END
