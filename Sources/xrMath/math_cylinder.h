#pragma once

#include "xrMathCommon.h"
#include "math_float3.h"
#include "math_utils.h" // для clamp, similar и т.п.
#include <cmath>
#include <limits>
#include <cassert> // для assert (вместо VERIFY2)

XRAY_BEGIN

// -----------------------------------------------------------------------------
// Цилиндр, заданный центром, осью (направлением), высотой и радиусом
// -----------------------------------------------------------------------------
template <class T> class Cylinder
{
  public:
	using Self = Cylinder<T>;
	using Vector3 = Vector3<T>;

	// Результат пересечения луча с цилиндром (аналогично сфере)
	enum ERP_Result
	{
		rpNone = 0,			// нет пересечения
		rpOriginInside = 1, // начало луча внутри цилиндра
		rpOriginOutside = 2 // начало луча снаружи (пересечение есть)
	};

  public:
	Vector3 m_center;	 // центр цилиндра
	Vector3 m_direction; // направление оси (единичное?)
	T m_height;			 // полная высота
	T m_radius;			 // радиус

	// Сброс в "нулевое" состояние (центр в 0, ось нулевая, высота и радиус 0)
	Self& invalidate()
	{
		m_center.set(0, 0, 0);
		m_direction.set(0, 0, 0);
		m_height = 0;
		m_radius = 0;
		return *this;
	}

	// Проверка на корректность (для внешней функции valid)
	bool valid() const
	{
		return ::xray::valid(m_center) && ::xray::valid(m_direction) && std::isfinite(m_height) &&
			   std::isfinite(m_radius) && m_height >= T(0) && m_radius >= T(0);
	}

	// -------------------------------------------------------------------------
	// Пересечение луча с цилиндром (возвращает количество точек пересечения
	// и заполняет массив afT значениями параметра t вдоль луча).
	// Луч: start + t * dir, t >= 0.
	// -------------------------------------------------------------------------
	int intersect(const Vector3& start, const Vector3& dir, T afT[2]) const
	{
		const T fEpsilon = T(1e-12);

		// Строим ортонормированный базис (kW = ось цилиндра, kU, kV)
		Vector3 kU, kV, kW = m_direction;
		Vector3::generate_orthonormal_basis(kW, kU, kV); // kW остаётся без изменений, kU, kV строятся

		// Проекция направления луча на оси цилиндра
		Vector3 kD(kU.dot(dir), kV.dot(dir), kW.dot(dir));

		// Проверка на нулевой вектор (для отладки)
		if (kD.square_magnitude() <= std::numeric_limits<T>::min())
		{
			// В оригинале здесь был Msg и VERIFY2. Оставим assert для отладки.
			assert(!"Cylinder::intersect: kD is zero (direction parallel to cylinder base?)");
			return 0;
		}

		T fDLength = kD.magnitude();
		T fInvDLength = T(1) / fDLength;
		// Нормализуем kD (теперь это единичное направление в базисе цилиндра)
		kD.mul(fInvDLength);

		// Смещение начала луча относительно центра цилиндра в мировых координатах
		Vector3 kDiff = start - m_center;

		// Проекция смещения на оси цилиндра
		Vector3 kP(kU.dot(kDiff), kV.dot(kDiff), kW.dot(kDiff));

		T fHalfHeight = T(0.5) * m_height;
		T fRadiusSqr = m_radius * m_radius;

		T fInv, fA, fB, fC, fDiscr, fRoot, fT, fT0, fT1, fTmp0, fTmp1;

		// ---- Случай 1: направление почти параллельно оси цилиндра ----
		if (std::abs(kD.z) >= T(1) - fEpsilon)
		{
			// Луч параллелен оси цилиндра
			if (kP.x * kP.x + kP.y * kP.y <= fRadiusSqr)
			{
				// Проекция на плоскость XY попадает в круг – пересекаемся с бесконечным цилиндром.
				// Находим пересечения с плоскостями оснований.
				fTmp0 = fInvDLength / kD.z;
				afT[0] = (+fHalfHeight - kP.z) * fTmp0;
				afT[1] = (-fHalfHeight - kP.z) * fTmp0;
				return 2;
			}
			else
			{
				return 0;
			}
		}

		// ---- Случай 2: луч перпендикулярен оси цилиндра (kD.z ≈ 0) ----
		if (std::abs(kD.z) <= fEpsilon)
		{
			// Луч перпендикулярен оси
			if (std::abs(kP.z) > fHalfHeight)
			{
				// Проекция начала луча на ось лежит вне отрезка [ -h/2, h/2 ] – нет пересечения с основаниями
				return 0;
			}

			// Проверяем пересечение с боковой поверхностью (бесконечный цилиндр)
			fA = kD.x * kD.x + kD.y * kD.y;
			fB = kP.x * kD.x + kP.y * kD.y;
			fC = kP.x * kP.x + kP.y * kP.y - fRadiusSqr;
			fDiscr = fB * fB - fA * fC;
			if (fDiscr < T(0))
			{
				return 0;
			}
			else if (fDiscr > T(0))
			{
				fRoot = std::sqrt(fDiscr);
				fTmp0 = fInvDLength / fA;
				afT[0] = (-fB - fRoot) * fTmp0;
				afT[1] = (-fB + fRoot) * fTmp0;
				return 2;
			}
			else // fDiscr == 0
			{
				afT[0] = -fB * fInvDLength / fA;
				return 1;
			}
		}

		// ---- Общий случай: сначала проверяем пересечения с плоскостями оснований ----
		int iQuantity = 0;
		fInv = T(1) / kD.z;

		// Верхнее основание (z = +halfHeight)
		fT0 = (+fHalfHeight - kP.z) * fInv;
		fTmp0 = kP.x + fT0 * kD.x;
		fTmp1 = kP.y + fT0 * kD.y;
		if (fTmp0 * fTmp0 + fTmp1 * fTmp1 <= fRadiusSqr)
			afT[iQuantity++] = fT0 * fInvDLength;

		// Нижнее основание (z = -halfHeight)
		fT1 = (-fHalfHeight - kP.z) * fInv;
		fTmp0 = kP.x + fT1 * kD.x;
		fTmp1 = kP.y + fT1 * kD.y;
		if (fTmp0 * fTmp0 + fTmp1 * fTmp1 <= fRadiusSqr)
			afT[iQuantity++] = fT1 * fInvDLength;

		if (iQuantity == 2)
		{
			// Луч пересекает оба основания
			return 2;
		}

		// Если есть одно пересечение с основанием, возможно, есть ещё пересечение с боковой поверхностью.
		// Проверяем пересечение с бесконечным цилиндром (боковая поверхность).
		fA = kD.x * kD.x + kD.y * kD.y;
		fB = kP.x * kD.x + kP.y * kD.y;
		fC = kP.x * kP.x + kP.y * kP.y - fRadiusSqr;
		fDiscr = fB * fB - fA * fC;

		if (fDiscr < T(0))
		{
			// Нет пересечения с боковой поверхностью
			return iQuantity; // могло быть одно пересечение с основанием
		}
		else if (fDiscr > T(0))
		{
			fRoot = std::sqrt(fDiscr);
			fInv = T(1) / fA;
			fT = (-fB - fRoot) * fInv; // меньшее t (вход в цилиндр)
			// Проверяем, лежит ли это t в пределах [min(fT0,fT1), max(fT0,fT1)]
			if (fT0 <= fT1)
			{
				if (fT0 <= fT && fT <= fT1)
					afT[iQuantity++] = fT * fInvDLength;
			}
			else
			{
				if (fT1 <= fT && fT <= fT0)
					afT[iQuantity++] = fT * fInvDLength;
			}

			if (iQuantity == 2)
				return 2; // нашли и основание, и вход

			fT = (-fB + fRoot) * fInv; // большее t (выход из цилиндра)
			if (fT0 <= fT1)
			{
				if (fT0 <= fT && fT <= fT1)
					afT[iQuantity++] = fT * fInvDLength;
			}
			else
			{
				if (fT1 <= fT && fT <= fT0)
					afT[iQuantity++] = fT * fInvDLength;
			}
		}
		else // fDiscr == 0
		{
			fT = -fB / fA;
			if (fT0 <= fT1)
			{
				if (fT0 <= fT && fT <= fT1)
					afT[iQuantity++] = fT * fInvDLength;
			}
			else
			{
				if (fT1 <= fT && fT <= fT0)
					afT[iQuantity++] = fT * fInvDLength;
			}
		}

		return iQuantity;
	}

	// Упрощённая версия пересечения, возвращающая результат и ближайшее расстояние.
	ERP_Result intersect(const Vector3& start, const Vector3& dir, T& dist) const
	{
		T afT[2];
		int cnt = intersect(start, dir, afT);
		if (cnt == 0)
			return rpNone;

		bool inside = false;
		bool found = false;
		for (int k = 0; k < cnt; ++k)
		{
			if (afT[k] < T(0))
			{
				if (cnt == 2)
					inside = true; // начало луча внутри цилиндра (одно t отрицательное)
				continue;
			}
			if (afT[k] < dist)
			{
				dist = afT[k];
				found = true;
			}
		}
		return found ? (inside ? rpOriginInside : rpOriginOutside) : rpNone;
	}
};

// Типовые определения
using cylinder = Cylinder<float>;
using fcylinder = Cylinder<float>;
using dcylinder = Cylinder<double>;

// Проверка на корректность (NaN/Inf)
template <class T> inline bool valid(const Cylinder<T>& c)
{
	return c.valid();
}

XRAY_END
