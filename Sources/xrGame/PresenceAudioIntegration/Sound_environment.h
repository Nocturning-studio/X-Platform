/*
====================================================================================================
  Presence Audio SDK Integration for X-Ray Engine
  File: Sound_environment.h
====================================================================================================
  Author: NSDeathman & Gemini 3
  Description: Заголовочный файл менеджера звуковой среды.
  Определяет класс CSoundEnvironment, который служит центральным узлом интеграции
  между игровым движком (xrGame), звуковым движком (xrSound) и библиотекой физики (Presence SDK).
====================================================================================================

  Copyright (c) 2025 Nocturning Studio, NSDeathman & Gemini 3

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  1. The above copyright notice and this permission notice shall be included in all
	 copies or substantial portions of the Software.

  2. Any project (commercial, free, open-source, or closed-source) using this Software
	 must include attribution to "Presence Audio SDK by Nocturning Studio" in its
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
  Organization: Nocturning Studio
====================================================================================================
*/

#pragma once

// =================================================================================================
// INCLUDES
// =================================================================================================

// Подключение публичного API библиотеки (Interface Definition)
// Здесь находится определение ISoundOcclusionCalculator и структур данных.
#include <PresenceAudioSDK/PresenceAudioAPI.h>

// Внутренние структуры данных движка X-Ray (SEAXEnvironmentData)
#include "Sound_environment_common.h"

// Предварительное объявление адаптера геометрии (Forward Declaration),
// чтобы не тянуть тяжелые хедеры движка (xr_area.h) сюда.
class XRayGeometryAdapter;

// =================================================================================================
// SOUND ENVIRONMENT MANAGER
// =================================================================================================
// Класс-контроллер для системы Presence Audio.
//
// Ключевые ответственности:
// 1. Управление библиотекой (LoadLibrary/FreeLibrary).
// 2. Связывание геометрии уровня с физическим движком звука.
// 3. Реализация интерфейса ISoundOcclusionCalculator для подмены стандартной
//    логики расчета громкости в xrSound (Dependency Injection).
// =================================================================================================
class CSoundEnvironment : public Presence::ISoundOcclusionCalculator
{
  private:
	// ---------------------------------------------------------------------------------------------
	// System Resources
	// ---------------------------------------------------------------------------------------------

	// Хэндл динамической библиотеки (.dll). Хранится для корректной выгрузки при выходе.
	HMODULE hPresenceAudioSDKLib;

	// Буфер данных EAX для передачи в OpenAL.
	// Используется как промежуточное звено между форматом SDK и форматом X-Ray.
	SEAXEnvironmentData m_CurrentData;

	// Указатель на главный объект системы (находится внутри DLL).
	// Управляет потоками и математикой.
	Presence::AudioSystem* m_pAudioSystem;

	// Указатель на адаптер (находится в xrGame).
	// Предоставляет доступ к треугольникам и материалам уровня.
	XRayGeometryAdapter* m_pGeometryAdapter;

	// Флаг состояния: true, если уровень загружен и система работает.
	bool m_bLoaded;

	// Флаг состояния: true если сама библиотека включена (для проверки выключили ли эффект чтобы очистить данные для EAX)
	bool m_bEnabled;

	bool m_bPaused;

  public:
	// =============================================================================================
	// Initialization / Destruction
	// =============================================================================================

	CSoundEnvironment();
	~CSoundEnvironment();

	// =============================================================================================
	// Main Cycle & Lifecycle
	// =============================================================================================

	/**
	 * @brief Основной метод обновления (Tick).
	 * Вызывается каждый кадр из игрового цикла (IGame_Persistent::OnFrame).
	 * Собирает данные о камере, отправляет их в SDK и применяет результаты.
	 */
	void Update();

	/**
	 * @brief Инициализация сессии уровня.
	 * Вызывается при старте игры или загрузке сохранения.
	 * Здесь создаются потоки трассировки и регистрируется перехватчик в xrSound.
	 */
	void OnLevelLoad();

	/**
	 * @brief Завершение сессии уровня.
	 * Вызывается при выходе в меню или перед загрузкой.
	 * Останавливает потоки и, самое важное, отключает перехватчик из xrSound во избежание крашей.
	 */
	void OnLevelUnload();

	// =============================================================================================
	// Data Marshalling
	// =============================================================================================

	/**
	 * @brief Применение параметров EAX.
	 * Конвертирует структуру Presence::EAXResult в формат X-Ray и отправляет в драйвер.
	 * @param res Результат расчетов из SDK.
	 */
	void ApplyToSoundDriver(const Presence::EAXResult& res);

	// =============================================================================================
	// ISoundOcclusionCalculator Implementation
	// =============================================================================================

	/**
	 * @brief Расчет окклюзии (громкости) для конкретного источника.
	 *
	 * @details Этот метод является "Callback'ом", который вызывает низкоуровневый
	 * звуковой движок (xrSound) для каждого играющего звука.
	 *
	 * Вместо стандартного RayPick движка, запрос перенаправляется в Presence SDK,
	 * где выполняется сложная проверка на дифракцию и проницаемость материалов.
	 *
	 * @param listenerPos Позиция камеры/ушей.
	 * @param sourcePos Позиция эмиттера звука.
	 * @return Коэффициент громкости [0.0..1.0].
	 */
	virtual float CalculateOcclusion(const Presence::float3& listenerPos, const Presence::float3& sourcePos) override;

	void Pause()
	{
		m_bPaused = true;
	}

	void Play()
	{
		m_bPaused = false;
	}
};
