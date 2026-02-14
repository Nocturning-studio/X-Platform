#pragma once
#include "xrMathCommon.h"
#include <cstdint>
#include <cassert> // для assert (вместо VERIFY)
#include <limits>

XRAY_BEGIN

// Класс генератора псевдослучайных чисел, совместимый со старой реализацией.
// Внимание: в оригинальном коде перед каждым вызовом генератора выполняется
// инкремент состояния (holdrand++), что приводит к быстрому изменению seed.
// Данная реализация сохраняет это поведение для совместимости.
class CRandom
{
  private:
	volatile s32 holdrand; // изменяется из разных потоков? volatile не гарантирует атомарность

  public:
	CRandom() : holdrand(1)
	{
	}
	explicit CRandom(s32 seed) : holdrand(seed)
	{
	}

	// Установка начального значения
	void seed(s32 val)
	{
		holdrand = val;
	}

	// Максимальное целое значение, возвращаемое randI() без аргументов (32767)
	static s32 maxI()
	{
		return 32767;
	}

	// Максимальное вещественное значение (32767.0f)
	static float maxF()
	{
		return 32767.0f;
	}

	// Базовый генератор целого числа в диапазоне [0, 32767]
	s32 randI()
	{
		holdrand++;
		holdrand = holdrand * 214013LL + 2531011LL;
		return (holdrand >> 16) & 0x7fff;
	}

	// Целое число в диапазоне [0, max-1]
	s32 randI(s32 max)
	{
		assert(max != 0);
		holdrand++;
		return randI() % max;
	}

	// Целое число в диапазоне [min, max-1] (max > min)
	s32 randI(s32 min, s32 max)
	{
		holdrand++;
		return min + randI(max - min);
	}

	// Целое число в диапазоне [-range, range]
	s32 randIs(s32 range)
	{
		holdrand++;
		return randI(-range, range + 1); // чтобы включить +range
	}

	// Целое число со смещением: offs + randIs(range)
	s32 randIs(s32 range, s32 offs)
	{
		holdrand++;
		return offs + randIs(range);
	}

	// Вещественное число в диапазоне [0, 1]
	float randF()
	{
		holdrand++;
		return float(randI()) / maxF();
	}

	// Вещественное число в диапазоне [0, max]
	float randF(float max)
	{
		holdrand++;
		return randF() * max;
	}

	// Вещественное число в диапазоне [min, max]
	float randF(float min, float max)
	{
		holdrand++;
		return min + randF(max - min);
	}

	// Вещественное число в диапазоне [-range, range]
	float randFs(float range)
	{
		holdrand++;
		return randF(-range, range);
	}

	// Вещественное число со смещением: offs + randFs(range)
	float randFs(float range, float offs)
	{
		holdrand++;
		return offs + randFs(range);
	}
};

// Глобальный экземпляр (определён в cpp)
extern XRMATH_API CRandom Random;

XRAY_END
