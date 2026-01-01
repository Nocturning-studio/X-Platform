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

// =================================================================================================
// DLL EXPORT / IMPORT MACROS
// =================================================================================================
// Standard Windows DLL visibility control macros.
// When building the library (PRESENCE_BUILD_DLL), symbols are exported.
// When linking from game engine - symbols are imported.
// =================================================================================================
#ifdef PRESENCE_BUILD_DLL
#define PRESENCE_API __declspec(dllexport)
#else
#define PRESENCE_API __declspec(dllimport)
#endif

namespace Presence
{
    // =================================================================================================
    // MATERIAL TYPES
    // =================================================================================================
    // Acoustic properties of surfaces. Affect sound absorption,
    // reflectivity, and transmission.
    // =================================================================================================
    enum class MaterialType : int
    {
        Air = 0,      // Air / No obstacle (full transmission).
        Stone = 1,    // Concrete, brick, stone, asphalt (strong echo, low transmission).
        Metal = 2,    // Metal (ringing echo, complete sound blocking).
        Wood = 3,     // Wood, parquet, plywood (warm tone, medium absorption).
        Soft = 4,     // Fabric, carpets, grass, soil, foliage (strong HF absorption).
        Glass = 5,    // Glass, ice (transmits sound but strongly reflects high frequencies).
        Absorber = 6, // Acoustic foam, studio isolation (complete absorption, no echo).

        // Sentinel value for automatic material count calculation.
        // Must always be last!
        Count = 7
    };

    // =================================================================================================
    // MATERIAL PARAMETERS STRUCTURE
    // =================================================================================================
    // Defines acoustic properties of a material.
    // All values range from 0.0 to 1.0.
    // =================================================================================================
    struct MaterialParams
    {
        float transmission;  // [0.0-1.0] Sound transmission through material
        float reflectivity;  // [0.0-1.0] Sound reflection coefficient
        float absorption;    // [0.0-1.0] Sound absorption coefficient
        float rt60_weight;   // [0.0-1.0] Weight in RT60 calculation

        MaterialParams() : transmission(0.0f), reflectivity(0.0f),
            absorption(0.0f), rt60_weight(0.0f) {}

        MaterialParams(float t, float r, float a, float w) :
            transmission(t), reflectivity(r), absorption(a), rt60_weight(w) {}
    };

    // =================================================================================================
    // RAYCAST HIT RESULT
    // =================================================================================================
    // Structure returned by game engine's CastRay implementation.
    // Contains physical data about sound ray collision with geometry.
    // =================================================================================================
    struct RayHit
    {
        bool isHit;           // true if ray intersected an obstacle
        float distance;       // Distance from ray origin to hit point (in meters)
        float3 normal;        // Surface normal at hit point (for reflection calculations)
        int materialID;       // Material identifier at hit point

        RayHit() : isHit(false), distance(0.0f), materialID(0) {}

        RayHit(bool hit, float dist, const float3& norm, int matID) :
            isHit(hit), distance(dist), normal(norm), materialID(matID) {}
    };

    // =================================================================================================
    // GEOMETRY PROVIDER INTERFACE
    // =================================================================================================
    // Abstract layer between Presence SDK and game's physics engine (X-Ray, PhysX, etc.).
    // Library user must implement this interface.
    // =================================================================================================
    class IGeometryProvider
    {
    public:
        virtual ~IGeometryProvider() = default;

        /**
         * @brief Performs ray tracing in game world
         * @param start Ray origin point (usually camera or source position)
         * @param dir Normalized direction vector
         * @param maxDist Maximum ray distance to check
         * @return RayHit with information about nearest obstacle
         */
        virtual RayHit CastRay(const float3& start, const float3& dir, float maxDist) = 0;
    };

    // =================================================================================================
    // OCCLUSION CALCULATOR INTERFACE
    // =================================================================================================
    // Interface for integration into sound subsystem (e.g., CSoundRender_Core).
    // Allows querying sound volume considering obstacles.
    // =================================================================================================
    class PRESENCE_API ISoundOcclusionCalculator
    {
    public:
        virtual ~ISoundOcclusionCalculator() = default;

        /**
         * @brief Calculates sound source audibility coefficient
         * @param listenerPos Listener position
         * @param sourcePos Sound source position
         * @return float [0.0 ... 1.0] where 0.0 = complete silence, 1.0 = direct line of sight
         */
        virtual float CalculateOcclusion(const float3& listenerPos, const float3& sourcePos) = 0;
    };

    // =================================================================================================
    // REVERBERATION PARAMETERS (EAX / OPENAL ENVIRONMENT)
    // =================================================================================================
    // Final structure with acoustic parameters calculated by the system.
    // Fully compatible with EAX 2.0 / EAX 5.0 / OpenAL EF X standards.
    // Volume values are in Millibels (mB): 0 mB = maximum volume, -10000 mB = silence.
    // =================================================================================================
    struct EAXResult
    {
        // --- Core parameters (Room & Decay) ---
        int32_t lRoom;               // [-10000 ... 0] Overall reverb volume (Master Volume)
        int32_t lRoomHF;             // [-10000 ... 0] High frequency attenuation (Tone)
        float   flRoomRolloffFactor; // [0.0 ... 10.0] Distance-based effect rolloff factor
        float   flDecayTime;         // [0.1 ... 20.0] Reverberation decay time (RT60) in seconds
        float   flDecayHFRatio;      // [0.1 ... 2.0] HF to LF decay time ratio

        // --- Early reflections ---
        int32_t lReflections;        // [-10000 ... 1000] First reflection volume
        float   flReflectionsDelay;  // [0.0 ... 0.3] First reflection delay in seconds

        // --- Late reverb ---
        int32_t lReverb;             // [-10000 ... 2000] Late reverb tail volume
        float   flReverbDelay;       // [0.0 ... 0.1] Late reverb delay relative to early reflections

        // --- Environment properties ---
        float   flEnvironmentSize;      // [1.0 ... 100.0] Perceived room size in meters
        float   flEnvironmentDiffusion; // [0.0 ... 1.0] Echo density (1.0 = dense, 0.0 = grainy)
        float   flAirAbsorptionHF;      // [-100 ... 0] Air absorption (affected by fog/humidity)

        // --- Internal flags ---
        bool    isValid;                // Data readiness flag (false if calculation not complete)

        // --- Debug information (for console output) ---
        float   debugEnclosedness;      // [0.0 ... 1.0] 1.0 = fully enclosed space
        float   debugOpenness;          // [0.0 ... 1.0] 1.0 = open field

        EAXResult() { Reset(); }

        /**
         * @brief Resets to neutral state (sound as in vacuum/quiet room)
         */
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
            isValid = false;
            debugEnclosedness = 0.0f;
            debugOpenness = 1.0f;
        }
    };

    // =================================================================================================
    // SYSTEM SETTINGS STRUCTURE
    // =================================================================================================
    struct Settings
    {
        bool useMultithreading = true;  // Use background thread for calculations (recommended true)
        float maxRayDistance = 200.0f;  // Maximum ray length in meters (Culling distance)
        int maxBounces = 3;             // Recursion depth (number of sound bounces). Optimal 2-3.
        float updateInterval = 0.033f;  // Physics update interval in seconds (~30 FPS)

        Settings() = default;

        Settings(bool multithread, float maxDist, int bounces, float interval) :
            useMultithreading(multithread), maxRayDistance(maxDist),
            maxBounces(bounces), updateInterval(interval) {}
    };
}
