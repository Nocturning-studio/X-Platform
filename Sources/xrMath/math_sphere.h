#pragma once

#include "xrMathCommon.h"
#include "math_float3.h"
#include "math_utils.h"      // для fsimilar, is_zero и т.п.
#include <cmath>
#include <limits>
#include <list>              // для Miniball

XRAY_BEGIN

// -----------------------------------------------------------------------------
// Сфера (ориентированная? нет, просто с центром и радиусом)
// -----------------------------------------------------------------------------
template <class T>
class Sphere
{
public:
    using Self = Sphere<T>;
    using Vector3 = Vector3<T>;

    // Результат пересечения луча со сферой (используется в intersect)
    enum ERP_Result
    {
        rpNone = 0,           // нет пересечения
        rpOriginInside = 1,   // начало луча внутри сферы
        rpOriginOutside = 2    // начало луча снаружи
    };

public:
    Vector3 P;   // центр
    T R;         // радиус

    // Конструкторы
    Sphere() = default;
    Sphere(const Vector3& center, T radius) : P(center), R(radius) {}

    // Установка
    void set(const Vector3& _P, T _R)
    {
        P = _P;
        R = _R;
    }
    void set(const Sphere& other)
    {
        P = other.P;
        R = other.R;
    }

    // Единичная сфера с центром в начале координат
    void identity()
    {
        P.set(0, 0, 0);
        R = T(1);
    }

    // Проверка на корректность (для внешней функции valid)
    bool valid() const
    {
        return ::xray::valid(P) && std::isfinite(R) && R >= T(0);
    }

    // -------------------------------------------------------------------------
    // Пересечения
    // -------------------------------------------------------------------------

    // Базовое пересечение луча со сферой.
    // Возвращает количество точек пересечения (0,1,2) и заполняет массив afT
    // значениями параметра t (расстояния вдоль луча).
    ERP_Result intersect(const Vector3& start, const Vector3& dir, T max_dist,
                         int& quantity, T afT[2]) const
    {
        // Вычисляем коэффициенты квадратного уравнения a*t^2 + 2*b*t + c = 0
        // где a = |dir|^2 (предполагается, что dir уже нормирован? нет, в оригинале используется range)
        // В оригинале: передаётся range, умножается на D, потом на a. Странно.
        // Воспроизведём точно логику старого кода, но с новыми типами.
        Vector3 diff = start - P;
        T a = max_dist * max_dist;                     // a = range^2
        T b = diff.dot(dir) * max_dist;                 // b = range * (diff·dir)
        T c = diff.square_magnitude() - R * R;          // c = |diff|^2 - R^2

        ERP_Result result = rpNone;
        T discr = b * b - a * c;

        if (discr < T(0))
        {
            quantity = 0;
        }
        else if (discr > T(0))
        {
            T root = std::sqrt(discr);
            T invA = T(1) / a;
            afT[0] = max_dist * (-b - root) * invA;      // t0
            afT[1] = max_dist * (-b + root) * invA;      // t1
            if (afT[0] >= T(0))
            {
                quantity = 2;
                result = rpOriginOutside;
            }
            else if (afT[1] >= T(0))
            {
                quantity = 1;
                afT[0] = afT[1];
                result = rpOriginInside;
            }
            else
            {
                quantity = 0;
            }
        }
        else // discr == 0
        {
            afT[0] = max_dist * (-b / a);
            if (afT[0] >= T(0))
            {
                quantity = 1;
                result = rpOriginOutside;
            }
            else
            {
                quantity = 0;
            }
        }

        return result;
    }

    // Полная версия пересечения: возвращает результат и обновляет dist,
    // если найдено пересечение ближе текущего значения.
    ERP_Result intersect_full(const Vector3& start, const Vector3& dir, T& dist) const
    {
        int quantity;
        T afT[2];
        ERP_Result result = intersect(start, dir, dist, quantity, afT);

        if (result == rpOriginInside ||
            (result == rpOriginOutside && afT[0] < dist))
        {
            if (result == rpOriginInside)
            {
                dist = (afT[0] < dist) ? afT[0] : dist;
            }
            else // rpOriginOutside
            {
                dist = afT[0];
            }
        }
        else
        {
            result = rpNone;
        }

        return result;
    }

    // Упрощённая версия: возвращает результат, dist обновляется при пересечении.
    ERP_Result intersect(const Vector3& start, const Vector3& dir, T& dist) const
    {
        int quantity;
        T afT[2];
        ERP_Result result = intersect(start, dir, dist, quantity, afT);
        if (result != rpNone && quantity > 0 && afT[0] < dist)
        {
            dist = afT[0];
            return result;
        }
        return rpNone;
    }

    // Альтернативная версия (из старого кода intersect2)
    ERP_Result intersect2(const Vector3& start, const Vector3& dir, T& range) const
    {
        Vector3 Q = P - start;
        T R2 = R * R;
        T c2 = Q.square_magnitude();
        T v = Q.dot(dir);
        T d = R2 - (c2 - v * v);

        if (d > T(0))
        {
            T t = v - std::sqrt(d);
            if (t < range)
            {
                range = t;
                return (c2 < R2) ? rpOriginInside : rpOriginOutside;
            }
        }
        return rpNone;
    }

    // Проверка пересечения луча (без вычисления расстояния)
    bool intersect(const Vector3& start, const Vector3& dir) const
    {
        Vector3 Q = P - start;
        T c = Q.magnitude();
        T v = Q.dot(dir);
        T d = R * R - (c * c - v * v);
        return (d > T(0));
    }

    // Пересечение с другой сферой
    bool intersect(const Sphere& other) const
    {
        T sumR = R + other.R;
        return P.distance_to_sqr(other.P) < sumR * sumR;
    }

    // Проверка, содержит ли сфера точку
    bool contains(const Vector3& point) const
    {
        return P.distance_to_sqr(point) <= (R * R + EPS_S);
    }

    // Проверка, содержит ли сфера другую сферу целиком
    bool contains(const Sphere& other) const
    {
        T rDiff = R - other.R;
        if (rDiff < T(0))
            return false;
        return P.distance_to_sqr(other.P) <= rDiff * rDiff;
    }

    // Объём сферы
    T volume() const
    {
        return (T(PI_MUL_4) / T(3)) * (R * R * R);
    }
};

// -----------------------------------------------------------------------------
// Вычисление минимальной описанной сферы для набора точек (только float)
// -----------------------------------------------------------------------------
namespace detail
{
    // Вспомогательные классы для алгоритма Miniball (адаптированы из старого кода)
    // Используют float3 (Vector3<float>)
    class Basis
    {
    private:
        enum { d = 3 };
        int m, s;                 // размер и количество опорных векторов
        float3 q0;
        float z[d + 1];
        float f[d + 1];
        float3 v[d + 1];
        float3 a[d + 1];
        float3 c[d + 1];
        float sqr_r[d + 1];
        float3* current_c;
        float current_sqr_r;

    public:
        Basis();

        // доступ
        const float3* center() const { return current_c; }
        float squared_radius() const { return current_sqr_r; }
        int size() const { return m; }
        int support_size() const { return s; }
        float excess(const float3& p) const;

        // модификация
        void reset();
        bool push(const float3& p);
        void pop();
    };

    Basis::Basis() { reset(); }

    void Basis::reset()
    {
        m = s = 0;
        c[0].set(0, 0, 0);
        current_c = c;
        current_sqr_r = -1;
    }

    float Basis::excess(const float3& p) const
    {
        float e = -current_sqr_r;
        e += p.distance_to_sqr(*current_c);
        return e;
    }

    void Basis::pop()
    {
        --m;
    }

    bool Basis::push(const float3& p)
    {
        const float eps = 1e-16f;

        if (m == 0)
        {
            q0 = p;
            c[0] = q0;
            sqr_r[0] = 0;
        }
        else
        {
            int i;

            // v_m = p - q0
            v[m].sub(p, q0);

            // вычисляем a_{m,i}, i < m
            for (i = 1; i < m; ++i)
            {
                a[m][i] = v[i].dot(v[m]);
                a[m][i] *= (2.f / z[i]);
            }

            // обновляем v_m = p - q0 - сумма a_{m,i}*v_i
            for (i = 1; i < m; ++i)
            {
                v[m].mad(v[m], v[i], -a[m][i]);
            }

            // z_m = 2 * |v_m|^2
            z[m] = 2.f * v[m].square_magnitude();

            // отклоняем, если z_m слишком мало
            if (z[m] < eps * current_sqr_r)
                return false;

            // обновляем центр и квадрат радиуса
            float e = -sqr_r[m - 1];
            e += p.distance_to_sqr(c[m - 1]);

            f[m] = e / z[m];

            c[m] = c[m - 1];
            c[m].mad(v[m], f[m]);

            sqr_r[m] = sqr_r[m - 1] + e * f[m] * 0.5f;
        }

        current_c = c + m;
        current_sqr_r = sqr_r[m];
        s = ++m;
        return true;
    }

    class Miniball
    {
    private:
        using VectorList = std::list<float3>;
        using It = VectorList::iterator;

        VectorList L;
        Basis B;
        It support_end;

        void mtf_mb(It k);
        void pivot_mb(It k);
        void move_to_front(It j);
        float max_excess(It t, It i, It& pivot) const;

    public:
        Miniball() {}

        void check_in(const float3& p) { L.push_back(p); }
        void build();

        float3 center() const { return *B.center(); }
        float squared_radius() const { return B.squared_radius(); }
        int num_points() const { return (int)L.size(); }
        auto points_begin() const { return L.begin(); }
        auto points_end() const { return L.end(); }
        int nr_support_vectors() const { return B.support_size(); }
        auto support_points_begin() const { return L.begin(); }
        auto support_points_end() const { return support_end; }
    };

    void Miniball::build()
    {
        B.reset();
        support_end = L.begin();
        // используем pivot-версию (как в оригинале)
        pivot_mb(L.end());
    }

    void Miniball::pivot_mb(It i)
    {
        It t = ++L.begin();
        mtf_mb(t);
        float max_e, old_sqr_r = 0;
        do
        {
            It pivot = L.begin();
            max_e = max_excess(t, i, pivot);
            if (max_e > 0)
            {
                t = support_end;
                if (t == pivot) ++t;
                old_sqr_r = B.squared_radius();
                B.push(*pivot);
                mtf_mb(support_end);
                B.pop();
                move_to_front(pivot);
            }
        } while (max_e > 0 && B.squared_radius() > old_sqr_r);
    }

    void Miniball::mtf_mb(It i)
    {
        support_end = L.begin();
        if (B.size() == 4) return;

        for (It k = L.begin(); k != i; )
        {
            It j = k++;
            if (B.excess(*j) > 0)
            {
                if (B.push(*j))
                {
                    mtf_mb(j);
                    B.pop();
                    move_to_front(j);
                }
            }
        }
    }

    void Miniball::move_to_front(It j)
    {
        if (support_end == j)
            ++support_end;
        L.splice(L.begin(), L, j);
    }

    float Miniball::max_excess(It t, It i, It& pivot) const
    {
        const float3* pCenter = B.center();
        float sqr_r = B.squared_radius();
        float max_e = 0;
        for (It k = t; k != i; ++k)
        {
            float e = k->distance_to_sqr(*pCenter) - sqr_r;
            if (e > max_e)
            {
                max_e = e;
                pivot = k;
            }
        }
        return max_e;
    }

} // namespace detail

// Функция вычисления минимальной описанной сферы для набора точек (только float)
inline void compute_min_ball(Sphere<float>& dest, const float3* verts, int count)
{
    detail::Miniball mb;
    for (int i = 0; i < count; ++i)
        mb.check_in(verts[i]);
    mb.build();
    dest.P = mb.center();
    dest.R = std::sqrt(mb.squared_radius());
}

// -----------------------------------------------------------------------------
// Типовые определения и внешние функции
// -----------------------------------------------------------------------------
using sphere = Sphere<float>;
using fsphere = Sphere<float>;
using dsphere = Sphere<double>;

// Проверка на корректность (NaN/Inf)
template <class T>
inline bool valid(const Sphere<T>& s)
{
    return s.valid();
}

XRAY_END