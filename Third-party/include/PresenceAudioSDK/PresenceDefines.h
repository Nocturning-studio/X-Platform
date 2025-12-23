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
#include "PresenceMath.h"
#include <cstdint>

#define PRESENCE_VERSION 0.2f
#define PRESENCE_VERSION_TEXT "Presence Audio ver. 0.2 InDev"

// =================================================================================================
// DLL EXPORT / IMPORT MACROS
// =================================================================================================
// Стандартный блок для управления видимостью символов в Windows DLL.
// При сборке самой библиотеки (PRESENCE_BUILD_DLL) символы экспортируются.
// При подключении к игровому движку - импортируются.
// =================================================================================================
#ifdef PRESENCE_BUILD_DLL
#define PRESENCE_API __declspec(dllexport)
#else
#define PRESENCE_API __declspec(dllimport)
#endif

namespace Presence
{
    // =================================================================================================
    // ТИПЫ МАТЕРИАЛОВ (MATERIAL TYPES)
    // =================================================================================================
    // Акустические свойства поверхностей. Влияют на поглощение звука (absorption),
    // отражающую способность (reflectivity) и проницаемость (transmission).
    // =================================================================================================
    enum class MaterialType : int
    {
        Air = 0,    // Воздух / Отсутствие препятствия (полная проницаемость).
        Stone = 1,      // Бетон, кирпич, камень, асфальт (сильное эхо, низкая проницаемость).
        Metal = 2,      // Металл (звенящее эхо, полная блокировка звука).
        Wood = 3,       // Дерево, паркет, фанера (теплый тон, среднее поглощение).
        Soft = 4,       // Ткань, ковры, трава, земля, листва (сильное поглощение ВЧ).
        Glass = 5,      // Стекло, лед (пропускает звук, но сильно отражает высокие частоты).
        Absorber = 6,   // Акустический поролон, студийная изоляция (полное поглощение, нет эха).

        // Служебное значение для автоматического подсчета количества материалов.
        // Должно всегда быть последним!
        Count = 7
    };

    struct MaterialParams
    {
        float transmission;  // [0.0-1.0] Проницаемость
        float reflectivity;  // [0.0-1.0] Отражение
        float absorption;    // [0.0-1.0] Поглощение
        float rt60_weight;   // [0.0-1.0] Вес в RT60
    };

    // =================================================================================================
    // РЕЗУЛЬТАТ ТРАССИРОВКИ (RAYCAST HIT)
    // =================================================================================================
    // Структура, возвращаемая игровым движком при запросе CastRay.
    // Содержит физические данные о точке столкновения звукового луча с геометрией.
    // =================================================================================================
    struct RayHit
    {
        bool isHit;             // true, если луч пересек препятствие.
        float distance;         // Расстояние от начала луча до точки удара (в метрах).
        float3 normal;          // Нормаль поверхности в точке удара (для расчета отражений).
        int materialID;

        RayHit() : isHit(false), distance(0), materialID(0) {}
    };

    // =================================================================================================
    // ИНТЕРФЕЙС ПРОВАЙДЕРА ГЕОМЕТРИИ (GEOMETRY PROVIDER INTERFACE)
    // =================================================================================================
    // Абстрактный слой между Presence SDK и физическим движком игры (X-Ray, PhysX, etc).
    // Пользователь библиотеки обязан реализовать этот класс.
    // =================================================================================================
    class IGeometryProvider
    {
    public:
        virtual ~IGeometryProvider() = default;

        // Выполняет трассировку луча в игровом мире.
        // @param start: Начальная точка луча (обычно позиция камеры или источника).
        // @param dir: Нормализованный вектор направления.
        // @param maxDist: Максимальная дистанция проверки.
        // @return RayHit с информацией о ближайшем препятствии.
        virtual RayHit CastRay(const float3& start, const float3& dir, float maxDist) = 0;
    };

    // =================================================================================================
    // ИНТЕРФЕЙС РАСЧЕТА ОККЛЮЗИИ (OCCLUSION CALCULATOR INTERFACE)
    // =================================================================================================
    // Интерфейс для интеграции в звуковую подсистему (например, в CSoundRender_Core).
    // Позволяет запрашивать громкость звука с учетом препятствий.
    // =================================================================================================
    class PRESENCE_API ISoundOcclusionCalculator
    {
    public:
        virtual ~ISoundOcclusionCalculator() = default;

        // Рассчитывает коэффициент слышимости источника звука.
        // @param start: Позиция слушателя (Listener).
        // @param end: Позиция источника звука (Source).
        // @return float [0.0 ... 1.0], где 0.0 - полная тишина, 1.0 - прямая видимость.
        virtual float CalculateOcclusion(const float3& start, const float3& end) = 0;
    };

    // =================================================================================================
    // ПАРАМЕТРЫ РЕВЕРБЕРАЦИИ (EAX / OPENAL ENVIRONMENT)
    // =================================================================================================
    // Итоговая структура с акустическими параметрами, рассчитанными системой.
    // Полностью совместима со стандартами EAX 2.0 / EAX 5.0 / OpenAL EF X.
    // Значения громкости указаны в Millibels (mB): 0 mB = макс. громкость, -10000 mB = тишина.
    // =================================================================================================
    struct EAXResult
    {
        // --- Основные параметры (Room & Decay) ---
        int32_t lRoom;               // [-10000 ... 0] Общая громкость реверберации (Master Volume).
        int32_t lRoomHF;             // [-10000 ... 0] Затухание высоких частот (Tone). Чем меньше, тем глуше звук.
        float   flRoomRolloffFactor; // [0.0 ... 10.0] Коэффициент затухания эффекта с расстоянием.
        float   flDecayTime;         // [0.1 ... 20.0] Время затухания (RT60) в секундах. "Хвост" эха.
        float   flDecayHFRatio;      // [0.1 ... 2.0] Отношение времени затухания ВЧ к НЧ. < 1.0 = ВЧ гаснут быстрее.

        // --- Ранние отражения (Early Reflections) ---
        int32_t lReflections;        // [-10000 ... 1000] Громкость первых отражений от стен.
        float   flReflectionsDelay;  // [0.0 ... 0.3] Задержка первых отражений (сек). Зависит от размера комнаты.

        // --- Поздняя реверберация (Late Reverb) ---
        int32_t lReverb;             // [-10000 ... 2000] Громкость "хвоста" реверберации.
        float   flReverbDelay;       // [0.0 ... 0.1] Задержка начала хвоста относительно ранних отражений.

        // --- Параметры среды (Environment) ---
        float   flEnvironmentSize;      // [1.0 ... 100.0] Условный размер помещения в метрах (для масштабирования).
        float   flEnvironmentDiffusion; // [0.0 ... 1.0] Плотность эха. 1.0 = густое, 0.0 = зернистое (как в трубе).
        float   flAirAbsorptionHF;      // [-100 ... 0] Поглощение звука воздухом (зависит от тумана/влажности).

        // --- Служебные поля ---
        bool    isValid;                // Флаг готовности данных (false, если расчет еще не завершен).

        // --- Debug Info (для отладки в консоли) ---
        float   debugEnclosedness;      // [0.0 ... 1.0] 1.0 = полностью закрытое помещение.
        float   debugOpenness;          // [0.0 ... 1.0] 1.0 = открытое поле.

        EAXResult() { Reset(); }

        // Сброс в "нейтральное" состояние (звук как в вакууме/тихой комнате)
        void Reset() {
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
            isValid = false;
            flRoomRolloffFactor = 0.0f;
            debugEnclosedness = 0.0f;
            debugOpenness = 1.0f;
        }
    };

    // =================================================================================================
    // НАСТРОЙКИ СИСТЕМЫ (SYSTEM SETTINGS)
    // =================================================================================================
    struct Settings
    {
        bool useMultithreading = true;  // Использовать фоновый поток для расчетов (рекомендуется true).
        float maxRayDistance = 200.0f;  // Максимальная длина луча в метрах (Culling distance).
        int maxBounces = 3;             // Глубина рекурсии (кол-во отскоков звука). Оптимально 2-3.
        float updateInterval = 0.033f;  // Интервал обновления физики в секундах (~30 FPS).
    };
}
