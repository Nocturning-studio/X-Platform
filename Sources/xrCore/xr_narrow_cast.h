#pragma once
#define CHECK_FOR_OVERFLOW

#ifndef CHECK_FOR_OVERFLOW
#define xr_narrow_cast static_cast
#else

template <class T, class F>
constexpr T xr_narrow_cast(F from)
{
	const T casted = static_cast<T>(from);
	R_ASSERT(static_cast<F>(casted) == from);

	if constexpr (std::is_arithmetic<T>::value && std::is_signed<T>::value != std::is_signed<F>::value)
	{
		R_ASSERT(casted < T{} == from < F{});
	}

	return casted;
}

#endif
