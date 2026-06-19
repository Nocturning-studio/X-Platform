#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <stdexcept>
#include <initializer_list>

#include "AfterMathInternal.h"

AFTERMATH_BEGIN

template<typename T, std::size_t N>
class CustomTemplateVector {
public:
    CustomTemplateVector() { data_.fill(T{}); }
    CustomTemplateVector(T value) { data_.fill(value); }
    CustomTemplateVector(std::initializer_list<T> list) {
        if (list.size() != N)
            throw std::invalid_argument("CustomTemplateVector: wrong number of elements in initializer list");
        std::copy(list.begin(), list.end(), data_.begin());
    }

    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end()   const { return data_.end(); }

    CustomTemplateVector operator+() const { return *this; }
    CustomTemplateVector operator-() const {
        static_assert(std::is_signed_v<T>, "Unary minus is not allowed for unsigned types");
        CustomTemplateVector result;
        for (std::size_t i = 0; i < N; ++i)
            result[i] = -data_[i];
        return result;
    }

    CustomTemplateVector& operator++() {
        for (auto& x : data_) ++x;
        return *this;
    }
    CustomTemplateVector operator++(int) {
        CustomTemplateVector tmp = *this;
        ++(*this);
        return tmp;
    }
    CustomTemplateVector& operator--() {
        for (auto& x : data_) --x;
        return *this;
    }
    CustomTemplateVector operator--(int) {
        CustomTemplateVector tmp = *this;
        --(*this);
        return tmp;
    }

    CustomTemplateVector& operator+=(const CustomTemplateVector& rhs) {
        for (std::size_t i = 0; i < N; ++i) data_[i] += rhs[i];
        return *this;
    }
    CustomTemplateVector& operator-=(const CustomTemplateVector& rhs) {
        for (std::size_t i = 0; i < N; ++i) data_[i] -= rhs[i];
        return *this;
    }
    CustomTemplateVector& operator*=(const CustomTemplateVector& rhs) {
        for (std::size_t i = 0; i < N; ++i) data_[i] *= rhs[i];
        return *this;
    }
    CustomTemplateVector& operator/=(const CustomTemplateVector& rhs) {
        for (std::size_t i = 0; i < N; ++i) data_[i] /= rhs[i];
        return *this;
    }
    CustomTemplateVector& operator*=(T scalar) {
        for (auto& x : data_) x *= scalar;
        return *this;
    }
    CustomTemplateVector& operator/=(T scalar) {
        for (auto& x : data_) x /= scalar;
        return *this;
    }

    CustomTemplateVector& operator=(T scalar) {
        for (auto& x : data_) x = scalar;
        return *this;
    }

    bool operator==(const CustomTemplateVector& rhs) const { return data_ == rhs.data_; }
    bool operator!=(const CustomTemplateVector& rhs) const { return data_ != rhs.data_; }

private:
    std::array<T, N> data_;
};

template<typename T, std::size_t N>
CustomTemplateVector<T, N> operator+(const CustomTemplateVector<T, N>& lhs, const CustomTemplateVector<T, N>& rhs) {
    CustomTemplateVector<T, N> result = lhs;
    result += rhs;
    return result;
}

template<typename T, std::size_t N>
CustomTemplateVector<T, N> operator-(const CustomTemplateVector<T, N>& lhs, const CustomTemplateVector<T, N>& rhs) {
    CustomTemplateVector<T, N> result = lhs;
    result -= rhs;
    return result;
}

template<typename T, std::size_t N>
CustomTemplateVector<T, N> operator*(const CustomTemplateVector<T, N>& lhs, const CustomTemplateVector<T, N>& rhs) {
    CustomTemplateVector<T, N> result = lhs;
    result *= rhs;
    return result;
}

template<typename T, std::size_t N>
CustomTemplateVector<T, N> operator/(const CustomTemplateVector<T, N>& lhs, const CustomTemplateVector<T, N>& rhs) {
    CustomTemplateVector<T, N> result = lhs;
    result /= rhs;
    return result;
}

template<typename T, std::size_t N>
CustomTemplateVector<T, N> operator*(const CustomTemplateVector<T, N>& lhs, T scalar) {
    CustomTemplateVector<T, N> result = lhs;
    result *= scalar;
    return result;
}

template<typename T, std::size_t N>
CustomTemplateVector<T, N> operator*(T scalar, const CustomTemplateVector<T, N>& rhs) {
    return rhs * scalar;
}

template<typename T, std::size_t N>
CustomTemplateVector<T, N> operator/(const CustomTemplateVector<T, N>& lhs, T scalar) {
    CustomTemplateVector<T, N> result = lhs;
    result /= scalar;
    return result;
}

AFTERMATH_END