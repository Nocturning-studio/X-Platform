// xrCPU_Pipe.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "xrCPU_Pipe.h"
#pragma hdrstop

extern xrSkin1W xrSkin1W_x86;
extern xrSkin2W xrSkin2W_x86;
extern xrSkin2W xrSkin2W_SSE;
extern xrM44_Mul xrM44_Mul_x86;
extern xrTransfer xrTransfer_x86;
extern xrMemCopy_8b xrMemCopy_x86;
extern xrMemFill_32b xrMemFill32_MMX;

// Копирование памяти (замена удалённой xrMemCopy_x86)
void __stdcall xrMemCopy_x86(LPVOID dest, const void* src, u32 count)
{
	// Используем стандартную функцию memcpy
	std::memcpy(dest, src, count);
}

// Заполнение памяти 32-битными значениями (замена удалённой xrMemFill32_MMX)
void __stdcall xrMemFill32_MMX(LPVOID ptr, u32 count, u32 value)
{
	// ptr указывает на область памяти, count – количество dword (4-байтных слов)
	u32* p = static_cast<u32*>(ptr);
	for (u32 i = 0; i < count; ++i)
		p[i] = value;

	// Альтернативный вариант с использованием std::fill_n:
	// std::fill_n(static_cast<u32*>(ptr), count, value);
}

void __cdecl xrBind_PSGP(xrDispatchTable* T, DWORD dwFeatures)
{
	T->skin1W = xrSkin1W_x86;
	T->skin2W = xrSkin2W_x86;
	T->m44_mul = xrM44_Mul_x86;
	T->transfer = xrTransfer_x86;
	T->memCopy = xrMemCopy_x86;
	T->memFill = NULL;
	T->memFill32 = xrMemFill32_MMX;
}
