#pragma once
#include "xrMathCommon.h"
#include <cstdint> // для uint8_t, uint16_t, uint32_t, uint64_t

XRAY_BEGIN

// Шаблонный класс для работы с битовыми флагами.
// Внимание: имена методов `or` и `and` не являются ключевыми словами в MSVC.
// При компиляции в других средах может потребоваться замена на альтернативные названия.
template <class T> class Flags
{
  public:
	typedef T TYPE;
	typedef Flags<T> Self;
	typedef Self& SelfRef;
	typedef const Self& SelfCRef;

	T flags;

	// Конструкторы
	IC Flags() : flags(0)
	{
	}
	IC explicit Flags(T init) : flags(init)
	{
	}

	// Получение значения
	IC T get() const
	{
		return flags;
	}

	// Сброс / установка всех битов
	IC SelfRef zero()
	{
		flags = T(0);
		return *this;
	}
	IC SelfRef one()
	{
		flags = T(~T(0));
		return *this;
	} // все биты в 1

	// Инвертирование
	IC SelfRef invert()
	{
		flags = ~flags;
		return *this;
	}
	IC SelfRef invert(const Self& f)
	{
		flags = ~f.flags;
		return *this;
	}
	IC SelfRef invert(T mask)
	{
		flags ^= mask;
		return *this;
	}

	// Присваивание
	IC SelfRef assign(const Self& f)
	{
		flags = f.flags;
		return *this;
	}
	IC SelfRef assign(T mask)
	{
		flags = mask;
		return *this;
	}

	// Установка отдельных битов по маске
	IC SelfRef set(T mask, bool value)
	{
		if (value)
			flags |= mask;
		else
			flags &= ~mask;
		return *this;
	}

	// Проверки
	IC bool is(T mask) const
	{
		return mask == (flags & mask);
	}
	IC bool is_any(T mask) const
	{
		return (flags & mask) != 0;
	}
	IC bool test(T mask) const
	{
		return (flags & mask) != 0;
	}

	// Побитовое ИЛИ (сохраняем оригинальные имена для совместимости со старым кодом)
	IC SelfRef bit_or (T mask)
	{
		flags |= mask;
		return *this;
	}
	IC SelfRef bit_or(const Self& f, T mask)
	{
		flags = f.flags | mask;
		return *this;
	}

	// Побитовое И
	IC SelfRef bit_and (T mask)
	{
		flags &= mask;
		return *this;
	}
	IC SelfRef bit_and(const Self& f, T mask)
	{
		flags = f.flags & mask;
		return *this;
	}

	// Сравнение
	IC bool equal(const Self& f) const
	{
		return flags == f.flags;
	}
	IC bool equal(const Self& f, T mask) const
	{
		return (flags & mask) == (f.flags & mask);
	}
};

// Определения для конкретных размеров
using Flags8 = Flags<uint8_t>;
using Flags16 = Flags<uint16_t>;
using Flags32 = Flags<uint32_t>;
using Flags64 = Flags<uint64_t>;

// Синонимы из старой реализации
using flags8 = Flags<uint8_t>;
using flags16 = Flags<uint16_t>;
using flags32 = Flags<uint32_t>;
using flags64 = Flags<uint64_t>;

XRAY_END
