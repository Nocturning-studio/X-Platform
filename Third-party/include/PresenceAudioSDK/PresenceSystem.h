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
#include "PresenceDefines.h"
#include <memory>

namespace Presence
{
    // =================================================================================================
    // MAIN AUDIO SIMULATION SYSTEM
    // =================================================================================================
    // Главный класс библиотеки. Управляет:
    // 1. Асинхронной трассировкой лучей (Path Tracing) в отдельном потоке.
    // 2. Расчетом параметров реверберации (EAX) на основе геометрии помещения.
    // 3. Расчетом окклюзии (слышимости) источников звука.
    // 
    // Реализован с использованием идиомы PIMPL (Pointer to Implementation), чтобы скрыть
    // зависимости (std::thread, internal structs) от пользователя библиотеки.
    // =================================================================================================
    class PRESENCE_API AudioSystem : public ISoundOcclusionCalculator
    {
    public:
        // =============================================================================================
        // Конструкторы и Деструкторы
        // =============================================================================================

        /** @brief Создает экземпляр системы, но не запускает потоки. Требует вызова Initialize(). */
        AudioSystem();

        /** @brief Автоматически вызывает Shutdown() при уничтожении объекта. */
        ~AudioSystem();

        // =============================================================================================
        // Управление Жизненным Циклом (Lifecycle Management)
        // =============================================================================================

        /**
         * @brief Инициализация системы и запуск рабочих потоков.
         * @param provider Указатель на адаптер геометрии (реализованный в движке игры).
         *                 Система использует его для вызова CastRay. Не должен быть null.
         * @param settings Настройки симуляции (кол-во отскоков, дистанция, многопоточность).
         */
        void Initialize(IGeometryProvider* provider, const Settings& settings = Settings());

        /**
         * @brief Остановка вычислений и освобождение ресурсов.
         * @details Корректно завершает рабочий поток. Рекомендуется вызывать при выгрузке уровня.
         */
        void Shutdown();

        // =============================================================================================
        // Обновление симуляции (Simulation Update)
        // =============================================================================================

        /**
         * @brief Основной метод обновления. Должен вызываться каждый кадр (в OnFrame).
         *
         * @details Метод отправляет новую позицию слушателя в рабочий поток.
         * Сами расчеты трассировки происходят асинхронно, не блокируя игровой цикл.
         * Результаты (EAX) интерполируются (сглаживаются) относительно времени кадра dt.
         *
         * @param position Текущая позиция слушателя (Камеры или Актера).
         * @param dt Дельта времени (время, прошедшее с предыдущего кадра) в секундах.
         * @param envFogDensity Плотность тумана/дождя [0.0 - 1.0]. Влияет на поглощение высоких частот (Air Absorption).
         */
        void Update(const float3& position, float dt, float envFogDensity = 0.0f);

        // =============================================================================================
        // Получение результатов (Data Access)
        // =============================================================================================

        /**
         * @brief Возвращает текущие рассчитанные параметры реверберации.
         * @return Структура EAXResult, готовая к отправке в OpenAL/DirectSound.
         * @note Данные в структуре уже сглажены (интерполированы) для предотвращения резких скачков звука.
         */
        EAXResult GetEAXResult() const;

        /**
         * @brief Реализация интерфейса ISoundOcclusionCalculator.
         *
         * Рассчитывает коэффициент громкости для конкретного источника звука.
         * Учитывает препятствия (Transmission) и огибание звука (Diffraction/Reflection).
         *
         * @param listenerPos Позиция слушателя.
         * @param sourcePos Позиция источника звука.
         * @return Коэффициент громкости [0.0 - тишина, 1.0 - полная громкость].
         * @note Этот метод может вызываться синхронно из звукового потока движка.
         */
        virtual float CalculateOcclusion(const float3& listenerPos, const float3& sourcePos) override;

        void SetMaterialProperties(int materialID, const MaterialParams& params);

        int CreateCustomMaterial(const MaterialParams& params);

    private:
        // Скрытая реализация (PIMPL)
        class Impl;
        Impl* m_Impl;
    };
}
