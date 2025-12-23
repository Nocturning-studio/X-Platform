/*
====================================================================================================
  Presence Audio SDK
  High-Performance Real-time Audio Path Tracing & EAX Simulation Library
====================================================================================================

  Copyright (c) 2025 Presence Collaboratory, NSDeathman & Gemini 3

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  1. The above copyright notice and this permission notice shall be included in all
     copies or substantial portions of the Software.

  2. Any project (commercial, free, open-source, or closed-source) using this Software
     must include attribution to "Presence Audio SDK by Presence Collaboratory" in its
     documentation, credits, or about screen.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

====================================================================================================
  Developed by: NSDeathman (Architecture & Core), Gemini 3 (Optimization & Math)
  Organization: Presence Collaboratory
====================================================================================================
*/
#pragma once

// =================================================================================================
// SIMD / SSE INTRINSICS
// =================================================================================================
// Подключаем заголовки для векторных инструкций процессора.
// SSE (Streaming SIMD Extensions) позволяет выполнять математические операции
// над 4-мя числами float одновременно за один такт процессора.
// =================================================================================================
#include <xmmintrin.h> // SSE  (Базовые операции: add, sub, mul, div)
#include <pmmintrin.h> // SSE3 (Горизонтальное сложение: hadd)
#include <smmintrin.h> // SSE4.1 (Dot product: dp)

// Подавление предупреждений X-Ray Engine о deprecated функциях (sin, cos, sqrt)
#pragma warning(push)
#pragma warning(disable : 4995)
#include <cmath>
#pragma warning(pop)

#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdint>

// =================================================================================================
// MEMORY ALIGNMENT MACRO
// =================================================================================================
// Для эффективной работы SSE данные должны быть выровнены по границе 16 байт.
// Если адрес памяти не кратен 16, инструкция _mm_load_ps вызовет краш (Access Violation).
// =================================================================================================
#if defined(_MSC_VER)
#define PRESENCE_ALIGN(x) __declspec(align(x))
#else
#define PRESENCE_ALIGN(x) __attribute__((aligned(x)))
#endif

namespace Presence
{
    // Небольшая константа для сравнений с плавающей точкой (Epsilon)
    static constexpr float EPSILON = 1e-5f;

    /**
     * @brief 3-dimensional vector class with SSE optimization.
     *
     * Представляет собой 3D вектор (x, y, z), оптимизированный для высокопроизводительных вычислений.
     * Использует 128-битный регистр XMM для хранения данных, что позволяет выполнять
     * арифметические операции (сложение, умножение) в 4 раза быстрее, чем стандартный FPU код.
     *
     * @note Размер структуры: 16 байт (x, y, z, w). Поле w используется для выравнивания (Padding).
     */
    PRESENCE_ALIGN(16) struct float3
    {
    public:
        // ========================================================================
        // Данные (Data Union)
        // Union позволяет обращаться к памяти двояко:
        // 1. Как к отдельным компонентам {x, y, z} для удобства программиста.
        // 2. Как к вектору __m128 для инструкций процессора.
        // ========================================================================
        union {
            struct {
                float x;
                float y;
                float z;
                float _w; // Padding (не используется, но нужен для alignment 16 байт)
            };
            __m128 simd_; // SSE регистр, содержащий {x, y, z, w}
        };

        // ========================================================================
        // Конструкторы (Constructors)
        // ========================================================================

        /** @brief Конструктор по умолчанию. Инициализирует нулями. */
        inline float3() noexcept : simd_(_mm_setzero_ps()) {}

        /** @brief Конструктор из скаляра. Все компоненты равны s. */
        inline explicit float3(float s) noexcept : simd_(_mm_set1_ps(s)) {}

        /**
         * @brief Основной конструктор.
         * @note В SSE данные загружаются в обратном порядке (Little Endian), поэтому _mm_set_ps(w, z, y, x).
         */
        inline float3(float _x, float _y, float _z) noexcept
            : simd_(_mm_set_ps(0.0f, _z, _y, _x)) {}

        /** @brief Конструктор напрямую из SSE регистра (для внутренних операций). */
        inline explicit float3(__m128 s) noexcept : simd_(s) {}

        // ========================================================================
        // Унарные операторы (Unary Operators)
        // ========================================================================

        /**
         * @brief Унарный минус (Инверсия вектора).
         * @return Вектор (-x, -y, -z).
         * @note Реализовано как вычитание из нуля: 0.0f - v. Это быстрее умножения на -1.
         */
        inline float3 operator-() const noexcept {
            return float3(_mm_sub_ps(_mm_setzero_ps(), simd_));
        }

        // ========================================================================
        // Операторы доступа и присваивания (Access & Assignment)
        // ========================================================================

        inline float3& operator=(const float3& rhs) noexcept {
            simd_ = rhs.simd_;
            return *this;
        }

        // Доступ по индексу (0=x, 1=y, 2=z). Небезопасно (нет проверки границ), но быстро.
        inline float& operator[](int index) noexcept { return (&x)[index]; }
        inline const float& operator[](int index) const noexcept { return (&x)[index]; }

        // ========================================================================
        // Арифметика (SSE Optimized Arithmetic)
        // Все операции выполняются параллельно для 4 компонентов за 1 такт.
        // ========================================================================

        inline float3 operator+(const float3& v) const noexcept {
            return float3(_mm_add_ps(simd_, v.simd_));
        }

        inline float3 operator-(const float3& v) const noexcept {
            return float3(_mm_sub_ps(simd_, v.simd_));
        }

        // Покомпонентное умножение (x*x, y*y, z*z)
        inline float3 operator*(const float3& v) const noexcept {
            return float3(_mm_mul_ps(simd_, v.simd_));
        }

        // Покомпонентное деление
        inline float3 operator/(const float3& v) const noexcept {
            return float3(_mm_div_ps(simd_, v.simd_));
        }

        // Умножение на скаляр
        inline float3 operator*(float s) const noexcept {
            return float3(_mm_mul_ps(simd_, _mm_set1_ps(s)));
        }

        // Деление на скаляр (умножение на обратное число 1/s, так быстрее)
        inline float3 operator/(float s) const noexcept {
            return float3(_mm_mul_ps(simd_, _mm_set1_ps(1.0f / s)));
        }

        // Операторы с присваиванием (+=, -=, *=)
        inline float3& operator+=(const float3& v) noexcept {
            simd_ = _mm_add_ps(simd_, v.simd_);
            return *this;
        }

        inline float3& operator-=(const float3& v) noexcept {
            simd_ = _mm_sub_ps(simd_, v.simd_);
            return *this;
        }

        inline float3& operator*=(float s) noexcept {
            simd_ = _mm_mul_ps(simd_, _mm_set1_ps(s));
            return *this;
        }

        // ========================================================================
        // Векторная математика (Vector Math)
        // ========================================================================

        /**
         * @brief Скалярное произведение (Dot Product).
         * @return x*v.x + y*v.y + z*v.z
         * @note Использует горизонтальное сложение для свертки результата.
         */
        inline float dot(const float3& v) const noexcept {
            __m128 mul = _mm_mul_ps(simd_, v.simd_);
            // Горизонтальное сложение: (x+y, z+w, x+y, z+w)
            __m128 shuf = _mm_movehdup_ps(mul);
            __m128 sums = _mm_add_ps(mul, shuf);
            // Финальное сложение: (x+y) + (z+w)
            shuf = _mm_movehl_ps(shuf, sums);
            sums = _mm_add_ss(sums, shuf);
            return _mm_cvtss_f32(sums);
        }

        /**
         * @brief Векторное произведение (Cross Product).
         * @return Вектор, перпендикулярный обоим исходным векторам.
         * @note Реализовано через shuffles (перестановку байт), без переходов в скалярный режим.
         */
        inline float3 cross(const float3& v) const noexcept {
            // Formula: (y1*z2 - z1*y2, z1*x2 - x1*z2, x1*y2 - y1*x2)

            // Permute vectors to align components for multiplication
            __m128 a_yzx = _mm_shuffle_ps(simd_, simd_, _MM_SHUFFLE(3, 0, 2, 1)); // (y, z, x, w)
            __m128 b_yzx = _mm_shuffle_ps(v.simd_, v.simd_, _MM_SHUFFLE(3, 0, 2, 1));

            __m128 a_zxy = _mm_shuffle_ps(simd_, simd_, _MM_SHUFFLE(3, 1, 0, 2)); // (z, x, y, w)
            __m128 b_zxy = _mm_shuffle_ps(v.simd_, v.simd_, _MM_SHUFFLE(3, 1, 0, 2));

            // (y1*z2, z1*x2, ...)
            __m128 mul1 = _mm_mul_ps(a_yzx, b_zxy);
            // (z1*y2, x1*z2, ...)
            __m128 mul2 = _mm_mul_ps(a_zxy, b_yzx);

            // Subtraction gives the final cross product result
            return float3(_mm_sub_ps(mul1, mul2));
        }

        /** @brief Длина вектора (Magnitude). */
        inline float magnitude() const noexcept {
            return std::sqrt(dot(*this));
        }

        /** @brief Квадрат длины (Squared Magnitude). Быстрее, так как нет sqrt. */
        inline float length_sq() const noexcept {
            return dot(*this);
        }

        /**
         * @brief Нормализация вектора (приведение к длине 1.0).
         * @return Единичный вектор того же направления.
         */
        inline float3 normalize() const noexcept {
            // Используем SSE4.1 Dot Product, если доступен (быстрее), иначе ручной dot()
            // Маска 0x7F означает: умножить x,y,z (первые 3 бита 7) и записать результат во все 4 флоата (F)
            __m128 dp = _mm_dp_ps(simd_, simd_, 0x7F);

            float len = std::sqrt(_mm_cvtss_f32(dp));

            if (len > 1e-6f) {
                return *this / len;
            }
            return float3(); // Возвращаем (0,0,0) при делении на ноль
        }

        /** @brief Расстояние до другой точки. */
        inline float distance_to(const float3& v) const noexcept {
            return (*this - v).magnitude();
        }

        /**
         * @brief Multiply-Add (MAD).
         * @details Выполняет операцию: this += v * s.
         * Эффективно используется в трассировке лучей для шага вперед: pos += dir * distance.
         */
        inline void mad(const float3& v, float s) noexcept {
            __m128 s_vec = _mm_set1_ps(s);
            __m128 mul = _mm_mul_ps(v.simd_, s_vec);
            simd_ = _mm_add_ps(simd_, mul);
        }

        /**
         * @brief Отражение вектора от нормали.
         * @param i Вектор падения (Incident).
         * @param n Нормаль поверхности (Normal).
         * @return Отраженный вектор: i - 2 * dot(i, n) * n.
         */
        static float3 reflect(const float3& i, const float3& n) {
            return i - n * (2.0f * i.dot(n));
        }

        // ========================================================================
        // Утилиты / Отладка
        // ========================================================================

        /** @brief Строковое представление вектора "(x, y, z)" для логов. */
        std::string to_string() const {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)", x, y, z);
            return std::string(buffer);
        }
    };

    // =================================================================================================
    // ГЛОБАЛЬНЫЕ ОПЕРАТОРЫ
    // =================================================================================================

    /** @brief Позволяет писать "float * vector" (коммутативность умножения). */
    inline float3 operator*(float s, const float3& v) noexcept {
        return v * s;
    }
}
