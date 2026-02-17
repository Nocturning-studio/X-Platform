#include "stdafx.h"

void __stdcall xrM44_Mul_x86(_matrix<float>* pfD, _matrix<float>* pfM1, _matrix<float>* pfM2)
{
	// Проверка корректности указателей (опционально)
	VERIFY(pfD && pfM1 && pfM2);

	// Используем готовую реализацию умножения матриц
	pfD->mul(*pfM1, *pfM2);
}
