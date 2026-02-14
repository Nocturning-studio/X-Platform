#pragma once
#include "xrMathCommon.h"
#include "math_float3.h"
#include "math_float3x3.h"
#include "math_float4x4.h"
#include <limits>

XRAY_BEGIN

template <class T> struct OBB
{
    using Self = OBB<T>;
    using Vector3 = Vector3<T>;
    using Matrix3x3 = Matrix3x3<T>;
    using Matrix4x4 = Matrix4x4<T>;

    Matrix3x3 m_rotate;   // ориентация (базис)
    Vector3   m_translate; // центр (трансляция)
    Vector3   m_halfsize;  // половины размеров по каждой оси

    // Сброс в "нулевое" состояние (центр в 0, без поворота, нулевой размер)
    Self& invalidate()
    {
        m_rotate.identity();
        m_translate.set(0, 0, 0);
        m_halfsize.set(0, 0, 0);
        return *this;
    }

    // Преобразование OBB в матрицу 4x4 (только поворот и трансляция)
    void transform_get(Matrix4x4& D) const
    {
        D.i = m_rotate.i;
        D._14_pad = 0;
        D.j = m_rotate.j;
        D._24_pad = 0;
        D.k = m_rotate.k;
        D._34_pad = 0;
        D.c = m_translate;
        D._44_pad = 1;
    }

    // Установка OBB из матрицы 4x4 (берутся i,j,k как оси, c как трансляция)
    Self& transform_set(const Matrix4x4& S)
    {
        m_rotate.i = S.i;
        m_rotate.j = S.j;
        m_rotate.k = S.k;
        m_translate = S.c;
        return *this;
    }

    // Полная матрица преобразования (поворот * масштаб * трансляция)
    void transform_full(Matrix4x4& D) const
    {
        Matrix4x4 R, S;
        transform_get(R);
        S.scale(m_halfsize);          // масштабная матрица
        D.mul_43(R, S);               // R * S
    }

    // Преобразование OBB из исходного состояния с помощью матрицы M
    Self& transform(const Self& src, const Matrix4x4& M)
    {
        Matrix4x4 srcR, destR;
        src.transform_get(srcR);
        destR.mul_43(M, srcR);
        transform_set(destR);
        m_halfsize = src.m_halfsize;   // размер не меняется (предполагается, что M не содержит масштаба)
        return *this;
    }

    // Проверка пересечения луча с OBB (луч задан начальной точкой start и направлением dir)
    // dist на входе — максимальное расстояние поиска, на выходе — расстояние до точки пересечения (ближайшей)
    bool intersect(const Vector3& start, const Vector3& dir, T& dist) const
    {
        // Переводим луч в локальные координаты OBB
        Vector3 diff = start - m_translate;
        Vector3 origin(
            diff.dot(m_rotate.i),
            diff.dot(m_rotate.j),
            diff.dot(m_rotate.k)
        );
        Vector3 direction(
            dir.dot(m_rotate.i),
            dir.dot(m_rotate.j),
            dir.dot(m_rotate.k)
        );

        T t0 = 0, t1 = std::numeric_limits<T>::max();
        if (intersect(origin, direction, m_halfsize, t0, t1))
        {
            bool picked = false;
            if (t0 > 0)
            {
                if (t0 < dist)
                {
                    dist = t0;
                    picked = true;
                }
                if (t1 < dist)
                {
                    dist = t1;
                    picked = true;
                }
            }
            else
            {
                if (t1 < dist)
                {
                    dist = t1;
                    picked = true;
                }
            }
            return picked;
        }
        return false;
    }

private:
    // Вспомогательная функция для отсечения отрезка плоскостью
    static bool clip(T denom, T numer, T& t0, T& t1)
    {
        if (denom > 0)
        {
            if (numer > denom * t1)
                return false;
            if (numer > denom * t0)
                t0 = numer / denom;
            return true;
        }
        else if (denom < 0)
        {
            if (numer > denom * t0)
                return false;
            if (numer > denom * t1)
                t1 = numer / denom;
            return true;
        }
        else
        {
            return numer <= 0;
        }
    }

    // Основная функция пересечения луча с AABB в локальных координатах (центр в начале координат)
    static bool intersect(const Vector3& origin, const Vector3& dir, const Vector3& halfSize, T& t0, T& t1)
    {
        T saveT0 = t0, saveT1 = t1;
        bool notClipped =
            clip( dir.x, -origin.x - halfSize.x, t0, t1) &&
            clip(-dir.x,  origin.x - halfSize.x, t0, t1) &&
            clip( dir.y, -origin.y - halfSize.y, t0, t1) &&
            clip(-dir.y,  origin.y - halfSize.y, t0, t1) &&
            clip( dir.z, -origin.z - halfSize.z, t0, t1) &&
            clip(-dir.z,  origin.z - halfSize.z, t0, t1);

        return notClipped && (t0 != saveT0 || t1 != saveT1);
    }
};

// Конкретные типы
using OBBf = OBB<float>;
using OBBd = OBB<double>;

// Проверка на корректность (NaN/Inf)
template <class T> inline bool valid(const OBB<T>& obb)
{
    return valid(obb.m_rotate) && valid(obb.m_translate) && valid(obb.m_halfsize);
}

XRAY_END
