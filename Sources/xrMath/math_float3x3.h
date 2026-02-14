#pragma once
#include "math_float3.h"
#include "math_float4x4.h" // для преобразования из 4x4

#include <cmath>
#include <limits>
#include <type_traits>
#include <algorithm> // для std::swap
#include <cstring>	 // для std::memcpy (вместо CopyMemory)

XRAY_BEGIN

template <class T> struct Quaternion;

template <class T> struct Matrix3x3
{
	using Self = Matrix3x3<T>;
	using Vector = Vector3<T>;

	union {
		struct
		{
			T _11, _12, _13;
			T _21, _22, _23;
			T _31, _32, _33;
		};
		struct
		{
			Vector i; // первая строка (или столбец? В зависимости от соглашения)
			Vector j; // вторая строка
			Vector k; // третья строка
		};
		T m[3][3];
	};

	Self& set(const Quaternion<T>& Q);

	// Установка в единичную матрицу
	Self& identity()
	{
		_11 = 1;
		_12 = 0;
		_13 = 0;
		_21 = 0;
		_22 = 1;
		_23 = 0;
		_31 = 0;
		_32 = 0;
		_33 = 1;
		return *this;
	}

	// Транспонирование (строки становятся столбцами)
	Self& transpose()
	{
		std::swap(_12, _21);
		std::swap(_13, _31);
		std::swap(_23, _32);
		return *this;
	}

	// Умножение матриц: Result = This * Other
	Self mul(const Self& A, const Self& B)
	{
		Self R;
		R.m[0][0] = A.m[0][0] * B.m[0][0] + A.m[0][1] * B.m[1][0] + A.m[0][2] * B.m[2][0];
		R.m[0][1] = A.m[0][0] * B.m[0][1] + A.m[0][1] * B.m[1][1] + A.m[0][2] * B.m[2][1];
		R.m[0][2] = A.m[0][0] * B.m[0][2] + A.m[0][1] * B.m[1][2] + A.m[0][2] * B.m[2][2];

		R.m[1][0] = A.m[1][0] * B.m[0][0] + A.m[1][1] * B.m[1][0] + A.m[1][2] * B.m[2][0];
		R.m[1][1] = A.m[1][0] * B.m[0][1] + A.m[1][1] * B.m[1][1] + A.m[1][2] * B.m[2][1];
		R.m[1][2] = A.m[1][0] * B.m[0][2] + A.m[1][1] * B.m[1][2] + A.m[1][2] * B.m[2][2];

		R.m[2][0] = A.m[2][0] * B.m[0][0] + A.m[2][1] * B.m[1][0] + A.m[2][2] * B.m[2][0];
		R.m[2][1] = A.m[2][0] * B.m[0][1] + A.m[2][1] * B.m[1][1] + A.m[2][2] * B.m[2][1];
		R.m[2][2] = A.m[2][0] * B.m[0][2] + A.m[2][1] * B.m[1][2] + A.m[2][2] * B.m[2][2];
		*this = R;
		return *this;
	}

	// Трансформация вектора: V_out = V_in * Matrix
	void transform(Vector& v) const
	{
		Vector res;
		res.x = v.x * _11 + v.y * _21 + v.z * _31;
		res.y = v.x * _12 + v.y * _22 + v.z * _32;
		res.z = v.x * _13 + v.y * _23 + v.z * _33;
		v = res;
	}

	// --- Копирование ---
	Self& set(const Self& a)
	{
		std::memcpy(m, a.m, 9 * sizeof(T));
		return *this;
	}

	// --- Установка из матрицы 4x4 (берётся верхний левый блок 3x3) ---
	Self& set(const Matrix4x4<T>& a)
	{
		_11 = a._11;
		_12 = a._12;
		_13 = a._13;
		_21 = a._21;
		_22 = a._22;
		_23 = a._23;
		_31 = a._31;
		_32 = a._32;
		_33 = a._33;
		return *this;
	}

	// --- Специальное преобразование из 4x4 (используется в старой реализации как set_rapid) ---
	//   m[0][0] = a.m[0][0]; m[0][1] = a.m[0][1]; m[0][2] = -a.m[0][2];
	//   m[1][0] = a.m[1][0]; m[1][1] = a.m[1][1]; m[1][2] = -a.m[1][2];
	//   m[2][0] = -a.m[2][0]; m[2][1] = -a.m[2][1]; m[2][2] = a.m[2][2];
	Self& set_rapid(const Matrix4x4<T>& a)
	{
		_11 = a._11;
		_12 = a._12;
		_13 = -a._13;
		_21 = a._21;
		_22 = a._22;
		_23 = -a._23;
		_31 = -a._31;
		_32 = -a._32;
		_33 = a._33;
		return *this;
	}

	// --- Транспонирование исходной матрицы в текущую ---
	Self& transpose(const Self& src)
	{
		_11 = src._11;
		_12 = src._21;
		_13 = src._31;
		_21 = src._12;
		_22 = src._22;
		_23 = src._32;
		_31 = src._13;
		_32 = src._23;
		_33 = src._33;
		return *this;
	}

	// --- Сравнение с эпсилон ---
	bool similar(const Self& other, T eps = static_cast<T>(EPS_L)) const
	{
		return std::abs(_11 - other._11) < eps && std::abs(_12 - other._12) < eps && std::abs(_13 - other._13) < eps &&
			   std::abs(_21 - other._21) < eps && std::abs(_22 - other._22) < eps && std::abs(_23 - other._23) < eps &&
			   std::abs(_31 - other._31) < eps && std::abs(_32 - other._32) < eps && std::abs(_33 - other._33) < eps;
	}

	// --- Умножение текущей матрицы на другую справа (this = this * B) ---
	Self& mulB(const Self& B)
	{
		Self A = *this;
		return mul(A, B);
	}
	// --- Умножение текущей матрицы на другую слева (this = A * this) ---
	Self& mulA(const Self& A)
	{
		Self B = *this;
		return mul(A, B);
	}

	// --- Умножение первой матрицы на вторую с предварительным транспонированием первой (MTxM) ---
	Self& mul_transpose_left(const Self& A, const Self& B)
	{
		// Результат = A^T * B
		m[0][0] = A._11 * B._11 + A._21 * B._21 + A._31 * B._31;
		m[0][1] = A._11 * B._12 + A._21 * B._22 + A._31 * B._32;
		m[0][2] = A._11 * B._13 + A._21 * B._23 + A._31 * B._33;

		m[1][0] = A._12 * B._11 + A._22 * B._21 + A._32 * B._31;
		m[1][1] = A._12 * B._12 + A._22 * B._22 + A._32 * B._32;
		m[1][2] = A._12 * B._13 + A._22 * B._23 + A._32 * B._33;

		m[2][0] = A._13 * B._11 + A._23 * B._21 + A._33 * B._31;
		m[2][1] = A._13 * B._12 + A._23 * B._22 + A._33 * B._32;
		m[2][2] = A._13 * B._13 + A._23 * B._23 + A._33 * B._33;
		return *this;
	}

	// --- Умножение первой матрицы на вторую с предварительным транспонированием второй (MxMT) ---
	Self& mul_transpose_right(const Self& A, const Self& B)
	{
		// Результат = A * B^T
		m[0][0] = A._11 * B._11 + A._12 * B._12 + A._13 * B._13;
		m[0][1] = A._11 * B._21 + A._12 * B._22 + A._13 * B._23;
		m[0][2] = A._11 * B._31 + A._12 * B._32 + A._13 * B._33;

		m[1][0] = A._21 * B._11 + A._22 * B._12 + A._23 * B._13;
		m[1][1] = A._21 * B._21 + A._22 * B._22 + A._23 * B._23;
		m[1][2] = A._21 * B._31 + A._22 * B._32 + A._23 * B._33;

		m[2][0] = A._31 * B._11 + A._32 * B._12 + A._33 * B._13;
		m[2][1] = A._31 * B._21 + A._32 * B._22 + A._33 * B._23;
		m[2][2] = A._31 * B._31 + A._32 * B._32 + A._33 * B._33;
		return *this;
	}

	// --- Обращение матрицы 3x3 (без проверки) ---
	Self& invert(const Self& a)
	{
		T det = a._11 * (a._22 * a._33 - a._23 * a._32) - a._12 * (a._21 * a._33 - a._23 * a._31) +
				a._13 * (a._21 * a._32 - a._22 * a._31);
		T invDet = T(1) / det; // предполагаем, что det != 0

		_11 = invDet * (a._22 * a._33 - a._23 * a._32);
		_12 = -invDet * (a._12 * a._33 - a._13 * a._32);
		_13 = invDet * (a._12 * a._23 - a._13 * a._22);

		_21 = -invDet * (a._21 * a._33 - a._23 * a._31);
		_22 = invDet * (a._11 * a._33 - a._13 * a._31);
		_23 = -invDet * (a._11 * a._23 - a._13 * a._21);

		_31 = invDet * (a._21 * a._32 - a._22 * a._31);
		_32 = -invDet * (a._11 * a._32 - a._12 * a._31);
		_33 = invDet * (a._11 * a._22 - a._12 * a._21);
		return *this;
	}

	// --- Обращение с проверкой на вырожденность (возвращает false, если det близок к нулю) ---
	bool invert_b(const Self& a, T eps = std::numeric_limits<T>::epsilon())
	{
		T det = a._11 * (a._22 * a._33 - a._23 * a._32) - a._12 * (a._21 * a._33 - a._23 * a._31) +
				a._13 * (a._21 * a._32 - a._22 * a._31);
		if (std::abs(det) <= eps)
			return false;
		T invDet = T(1) / det;
		_11 = invDet * (a._22 * a._33 - a._23 * a._32);
		_12 = -invDet * (a._12 * a._33 - a._13 * a._32);
		_13 = invDet * (a._12 * a._23 - a._13 * a._22);
		_21 = -invDet * (a._21 * a._33 - a._23 * a._31);
		_22 = invDet * (a._11 * a._33 - a._13 * a._31);
		_23 = -invDet * (a._11 * a._23 - a._13 * a._21);
		_31 = invDet * (a._21 * a._32 - a._22 * a._31);
		_32 = -invDet * (a._11 * a._32 - a._12 * a._31);
		_33 = invDet * (a._11 * a._22 - a._12 * a._21);
		return true;
	}

	Self& invert()
	{
		Self a = *this;
		return invert(a);
	}

	// --- Кососимметричная матрица из вектора (для векторного произведения: [v]× ) ---
	Self& set_skew(const Vector& v)
	{
		_11 = 0;
		_12 = -v.z;
		_13 = v.y;
		_21 = v.z;
		_22 = 0;
		_23 = -v.x;
		_31 = -v.y;
		_32 = v.x;
		_33 = 0;
		return *this;
	}
	static Self make_skew(const Vector& v)
	{
		Self M;
		M.set_skew(v);
		return M;
	}

	// --- Копирование столбца из другой матрицы ---
	Self& set_column(int col, const Self& src, int src_col)
	{
		m[0][col] = src.m[0][src_col];
		m[1][col] = src.m[1][src_col];
		m[2][col] = src.m[2][src_col];
		return *this;
	}
	Self& set_column(int col, const Vector& v)
	{
		m[0][col] = v.x;
		m[1][col] = v.y;
		m[2][col] = v.z;
		return *this;
	}
	Vector get_column(int col) const
	{
		return Vector(m[0][col], m[1][col], m[2][col]);
	}

	// --- Умножение матрицы на вектор (M * v) ---
	void transform(Vector& dest, const Vector& v) const
	{
		dest.x = _11 * v.x + _12 * v.y + _13 * v.z;
		dest.y = _21 * v.x + _22 * v.y + _23 * v.z;
		dest.z = _31 * v.x + _32 * v.y + _33 * v.z;
	}
	//void transform(Vector& v) const
	//{
	//	Vector tmp;
	//	transform(tmp, v);
	//	v = tmp;
	//}

	// --- Умножение транспонированной матрицы на вектор (M^T * v) ---
	void transform_transposed(Vector& dest, const Vector& v) const
	{
		dest.x = _11 * v.x + _21 * v.y + _31 * v.z;
		dest.y = _12 * v.x + _22 * v.y + _32 * v.z;
		dest.z = _13 * v.x + _23 * v.y + _33 * v.z;
	}
	void transform_transposed(Vector& v) const
	{
		Vector tmp;
		transform_transposed(tmp, v);
		v = tmp;
	}

	// --- Умножение матрицы на вектор и добавление вектора (M * v1 + v2) ---
	void transform_add(Vector& dest, const Vector& v1, const Vector& v2) const
	{
		dest.x = _11 * v1.x + _12 * v1.y + _13 * v1.z + v2.x;
		dest.y = _21 * v1.x + _22 * v1.y + _23 * v1.z + v2.y;
		dest.z = _31 * v1.x + _32 * v1.y + _33 * v1.z + v2.z;
	}
	void transform_add(Vector& v, const Vector& add) const
	{
		Vector tmp;
		transform_add(tmp, v, add);
		v = tmp;
	}

	// --- Умножение транспонированной матрицы на вектор и добавление вектора (M^T * v1 + v2) ---
	void transform_transposed_add(Vector& dest, const Vector& v1, const Vector& v2) const
	{
		dest.x = _11 * v1.x + _21 * v1.y + _31 * v1.z + v2.x;
		dest.y = _12 * v1.x + _22 * v1.y + _32 * v1.z + v2.y;
		dest.z = _13 * v1.x + _23 * v1.y + _33 * v1.z + v2.z;
	}
	void transform_transposed_add(Vector& v, const Vector& add) const
	{
		Vector tmp;
		transform_transposed_add(tmp, v, add);
		v = tmp;
	}

	// --- Умножение матрицы на вектор с масштабированием (s * (M * v1) + v2) ---
	void transform_scaled_add(Vector& dest, T s, const Vector& v1, const Vector& v2) const
	{
		dest.x = s * (_11 * v1.x + _12 * v1.y + _13 * v1.z) + v2.x;
		dest.y = s * (_21 * v1.x + _22 * v1.y + _23 * v1.z) + v2.y;
		dest.z = s * (_31 * v1.x + _32 * v1.y + _33 * v1.z) + v2.z;
	}

	// --- Умножение матрицы на вектор (для 2D векторов, результат в 3D) ---
	void transform(Vector& dest, const Vector2<T>& v) const
	{
		dest.x = _11 * v.x + _12 * v.y;
		dest.y = _21 * v.x + _22 * v.y;
		dest.z = _31 * v.x + _32 * v.y;
	}
	void transform(Vector2<T>& dest, const Vector& v) const
	{
		dest.x = _11 * v.x + _21 * v.y + _31 * v.z;
		dest.y = _12 * v.x + _22 * v.y + _32 * v.z;
	}
	void transform(Vector2<T>& v) const
	{
		Vector2<T> tmp;
		transform(tmp, v);
		v = tmp;
	}

	// ========== Алгоритм Якоби для собственных чисел и векторов ==========
  private:
	// Вспомогательный макрос, заменяющий ROT из старой реализации
	static void jacobi_rotate(Self& a, Self& v, int i, int j, int k, int l, T s, T tau)
	{
		T g = a.m[i][j];
		T h = a.m[k][l];
		a.m[i][j] = g - s * (h + g * tau);
		a.m[k][l] = h + s * (g - h * tau);
		// Применяем те же вращения к матрице v
		g = v.m[i][j];
		h = v.m[k][l];
		v.m[i][j] = g - s * (h + g * tau);
		v.m[k][l] = h + s * (g - h * tau);
	}

  public:
	// Вычисление собственных чисел (d) и собственных векторов (в этой матрице)
	// Возвращает число итераций (максимум 50)
	int eigen(Vector& d_out)
	{
		const int max_iter = 50;
		T threshold;
		T theta, tau, t, sm, s, h, g, c;
		int nrot = 0;
		Vector b, z, d;
		Self v; // матрица собственных векторов (накапливает вращения)
		v.identity();

		b.set(_11, _22, _33);
		d.set(_11, _22, _33);
		z.set(0, 0, 0);

		for (int iter = 0; iter < max_iter; ++iter)
		{
			// Сумма внедиагональных элементов
			sm = std::abs(m[0][1]) + std::abs(m[0][2]) + std::abs(m[1][2]);
			if (sm == 0)
			{
				*this = v; // матрица вращений накоплена в v
				d_out = d;
				return iter;
			}

			if (iter < 3)
				threshold = T(0.2) * sm / T(9);
			else
				threshold = T(0);

			// Обработка элемента (0,1)
			{
				g = T(100) * std::abs(m[0][1]);
				if (iter > 3 && std::abs(d.x) + g == std::abs(d.x) && std::abs(d.y) + g == std::abs(d.y))
					m[0][1] = 0;
				else if (std::abs(m[0][1]) > threshold)
				{
					h = d.y - d.x;
					if (std::abs(h) + g == std::abs(h))
						t = m[0][1] / h;
					else
					{
						theta = T(0.5) * h / m[0][1];
						t = T(1) / (std::abs(theta) + std::sqrt(1 + theta * theta));
						if (theta < 0)
							t = -t;
					}
					c = T(1) / std::sqrt(1 + t * t);
					s = t * c;
					tau = s / (1 + c);
					h = t * m[0][1];
					z.x -= h;
					z.y += h;
					d.x -= h;
					d.y += h;
					m[0][1] = 0;
					jacobi_rotate(*this, v, 0, 2, 1, 2, s, tau); // ROT(a,0,2,1,2)
					jacobi_rotate(*this, v, 0, 0, 0, 1, s, tau); // ROT(v,0,0,0,1)
					jacobi_rotate(*this, v, 1, 0, 1, 1, s, tau); // ROT(v,1,0,1,1)
					jacobi_rotate(*this, v, 2, 0, 2, 1, s, tau); // ROT(v,2,0,2,1)
					++nrot;
				}
			}

			// Обработка элемента (0,2)
			{
				g = T(100) * std::abs(m[0][2]);
				if (iter > 3 && std::abs(d.x) + g == std::abs(d.x) && std::abs(d.z) + g == std::abs(d.z))
					m[0][2] = 0;
				else if (std::abs(m[0][2]) > threshold)
				{
					h = d.z - d.x;
					if (std::abs(h) + g == std::abs(h))
						t = m[0][2] / h;
					else
					{
						theta = T(0.5) * h / m[0][2];
						t = T(1) / (std::abs(theta) + std::sqrt(1 + theta * theta));
						if (theta < 0)
							t = -t;
					}
					c = T(1) / std::sqrt(1 + t * t);
					s = t * c;
					tau = s / (1 + c);
					h = t * m[0][2];
					z.x -= h;
					z.z += h;
					d.x -= h;
					d.z += h;
					m[0][2] = 0;
					jacobi_rotate(*this, v, 0, 1, 1, 2, s, tau); // ROT(a,0,1,1,2)
					jacobi_rotate(*this, v, 0, 0, 0, 2, s, tau); // ROT(v,0,0,0,2)
					jacobi_rotate(*this, v, 1, 0, 1, 2, s, tau); // ROT(v,1,0,1,2)
					jacobi_rotate(*this, v, 2, 0, 2, 2, s, tau); // ROT(v,2,0,2,2)
					++nrot;
				}
			}

			// Обработка элемента (1,2)
			{
				g = T(100) * std::abs(m[1][2]);
				if (iter > 3 && std::abs(d.y) + g == std::abs(d.y) && std::abs(d.z) + g == std::abs(d.z))
					m[1][2] = 0;
				else if (std::abs(m[1][2]) > threshold)
				{
					h = d.z - d.y;
					if (std::abs(h) + g == std::abs(h))
						t = m[1][2] / h;
					else
					{
						theta = T(0.5) * h / m[1][2];
						t = T(1) / (std::abs(theta) + std::sqrt(1 + theta * theta));
						if (theta < 0)
							t = -t;
					}
					c = T(1) / std::sqrt(1 + t * t);
					s = t * c;
					tau = s / (1 + c);
					h = t * m[1][2];
					z.y -= h;
					z.z += h;
					d.y -= h;
					d.z += h;
					m[1][2] = 0;
					jacobi_rotate(*this, v, 0, 1, 0, 2, s, tau); // ROT(a,0,1,0,2)
					jacobi_rotate(*this, v, 0, 1, 0, 2, s, tau); // ROT(v,0,1,0,2)
					jacobi_rotate(*this, v, 1, 1, 1, 2, s, tau); // ROT(v,1,1,1,2)
					jacobi_rotate(*this, v, 2, 1, 2, 2, s, tau); // ROT(v,2,1,2,2)
					++nrot;
				}
			}

			// Обновление диагональных элементов
			b += z;
			d = b;
			z.set(0, 0, 0);
		}
		// Если достигнуто максимальное число итераций, копируем v в this
		*this = v;
		d_out = d;
		return max_iter;
	}
};

// Определения конкретных типов
using float3x3 = Matrix3x3<float>;

// --- Внешняя функция проверки на NaN/Inf ---
template <class T> inline bool valid(const Matrix3x3<T>& m)
{
	return valid(m.i) && valid(m.j) && valid(m.k);
}

XRAY_END
