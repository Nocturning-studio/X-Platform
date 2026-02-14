/*
====================================================================================================
  Presence Audio SDK Integration for X-Ray Engine
  File: Sound_environment_common.h
====================================================================================================
  Author: NSDeathman
  Description: Общие структуры данных для звукового окружения.
  Определяет формат данных EAX (Environmental Audio Extensions), используемый
  для передачи параметров реверберации из логики движка в звуковой драйвер (OpenAL/DSound).
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

// Подключение базовых типов движка (для float3, u32 и т.д., если нужно)
#include "../xrEngine/xr_collide_defs.h"

// =================================================================================================
// EAX ENVIRONMENT DATA STRUCTURE
// =================================================================================================
// Структура, полностью описывающая акустические свойства помещения в данный кадр.
// Соответствует стандарту EAX 2.0 / OpenAL EFX.
//
// Единицы измерения:
// - Громкость (Level): Millibels (mB). 0 mB = 0 dB (Макс), -10000 mB = -100 dB (Тишина).
// - Время (Time): Секунды.
// - Расстояние (Size): Метры.
// =================================================================================================
struct SEAXEnvironmentData
{
	// ---------------------------------------------------------------------------------------------
	// Master Volume & Tone (Общая громкость и Тон)
	// ---------------------------------------------------------------------------------------------
	LONG lRoom; // [-10000 ... 0] Общий уровень громкости эффекта реверберации.
	LONG lRoomHF; // [-10000 ... 0] Затухание высоких частот (Tone). Влияет на "глухоту" звука.
	float flRoomRolloffFactor; // [0.0 ... 10.0] Коэффициент затухания эффекта с расстоянием. 0 = не затухает.

	// ---------------------------------------------------------------------------------------------
	// Decay (Затухание)
	// ---------------------------------------------------------------------------------------------
	float flDecayTime;	  // [0.1 ... 20.0] Время полной реверберации (RT60) в секундах.
	float flDecayHFRatio; // [0.1 ... 2.0] Отношение времени затухания ВЧ к НЧ.
						  // < 1.0: ВЧ гаснут быстрее (натурально). > 1.0: ВЧ гаснут медленнее (синтетика).

	// ---------------------------------------------------------------------------------------------
	// Early Reflections (Ранние отражения)
	// ---------------------------------------------------------------------------------------------
	LONG lReflections; // [-10000 ... 1000] Громкость первых, различимых эхо-сигналов.
	float flReflectionsDelay; // [0.0 ... 0.3] Задержка перед приходом первого отражения (зависит от размера комнаты).

	// ---------------------------------------------------------------------------------------------
	// Late Reverb (Поздняя реверберация / Хвост)
	// ---------------------------------------------------------------------------------------------
	LONG lReverb; // [-10000 ... 2000] Громкость диффузного "хвоста" реверберации.
	float flReverbDelay; // [0.0 ... 0.1] Задержка начала хвоста относительно ранних отражений.

	// ---------------------------------------------------------------------------------------------
	// Environment Properties (Свойства среды)
	// ---------------------------------------------------------------------------------------------
	float flEnvironmentSize; // [1.0 ... 100.0] Масштаб помещения в метрах.
	float flEnvironmentDiffusion; // [0.0 ... 1.0] Плотность эха. 1.0 = густое (хамам), 0.0 = зернистое (стадион).
	float flAirAbsorptionHF; // [-100 ... 0] Поглощение звука воздухом (зависит от влажности/тумана).

	// ---------------------------------------------------------------------------------------------
	// System Flags & State
	// ---------------------------------------------------------------------------------------------
	DWORD dwFlags; // Флаги EAX (EAXLISTENERFLAGS), определяющие, какие параметры активны.

	u32 dwFrameStamp; // Номер кадра движка, когда данные были рассчитаны (для синхронизации потоков).
	bool bDataValid; // Флаг валидности. True, если данные были успешно рассчитаны и готовы к применению.

	// =============================================================================================
	// Methods
	// =============================================================================================

	SEAXEnvironmentData()
	{
		Reset();
	}

	// Сброс в дефолтное состояние
	void Reset()
	{
		lRoom = -1000;
		lRoomHF = -100;
		flRoomRolloffFactor = 0.0f;
		flDecayTime = 1.49f;
		flDecayHFRatio = 0.83f;
		lReflections = -2602;
		flReflectionsDelay = 0.007f;
		lReverb = 200;
		flReverbDelay = 0.011f;
		flEnvironmentSize = 7.5f;
		flEnvironmentDiffusion = 1.0f;
		flAirAbsorptionHF = -5.0f;
		dwFlags = 0x00000001 | 0x00000002 | 0x00000004 | 0x00000008 | 0x00000010 | 0x00000020;
	}

	// Копирование данных (используется для двойной буферизации между потоками)
	void CopyFrom(const SEAXEnvironmentData& other)
	{
		lRoom = other.lRoom;
		lRoomHF = other.lRoomHF;
		flRoomRolloffFactor = other.flRoomRolloffFactor;
		flDecayTime = other.flDecayTime;
		flDecayHFRatio = other.flDecayHFRatio;
		lReflections = other.lReflections;
		flReflectionsDelay = other.flReflectionsDelay;
		lReverb = other.lReverb;
		flReverbDelay = other.flReverbDelay;
		flEnvironmentSize = other.flEnvironmentSize;
		flEnvironmentDiffusion = other.flEnvironmentDiffusion;
		flAirAbsorptionHF = other.flAirAbsorptionHF;
		dwFlags = other.dwFlags;
		dwFrameStamp = other.dwFrameStamp;
		bDataValid = other.bDataValid;
	}
};
