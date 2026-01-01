/*
====================================================================================================
  Presence Audio SDK - Main System Interface
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
    // Primary library class. Manages:
    // 1. Asynchronous ray tracing (Path Tracing) in separate thread
    // 2. Reverberation parameter calculation (EAX) based on room geometry
    // 3. Sound source occlusion (audibility) calculation
    // 
    // Implemented using PIMPL idiom (Pointer to Implementation) to hide
    // dependencies (std::thread, internal structs) from library users.
    // Using raw pointer for Impl to avoid DLL export issues with std::shared_ptr
    // =================================================================================================
    class PRESENCE_API AudioSystem : public ISoundOcclusionCalculator
    {
    public:
        // =============================================================================================
        // VERSION INFORMATION METHODS
        // =============================================================================================

        /**
         * @brief Returns library version number
         * @return Version as float (e.g., 0.2 for version 0.2)
         */
        static float GetVersion();

        /**
         * @brief Returns full version string
         * @return Complete version information in text format
         */
        static const char* GetVersionString();

        // =============================================================================================
        // Constructors and Destructors
        // =============================================================================================

        /**
         * @brief Creates system instance but doesn't start threads. Requires Initialize() call.
         */
        AudioSystem();

        /**
         * @brief Automatically calls Shutdown() on object destruction
         */
        ~AudioSystem();

        /**
         * @brief Move constructor for efficient resource transfer
         */
        AudioSystem(AudioSystem&& other) noexcept;

        /**
         * @brief Move assignment operator
         */
        AudioSystem& operator=(AudioSystem&& other) noexcept;

        // =============================================================================================
        // Lifecycle Management
        // =============================================================================================

        /**
         * @brief Initialize system and start worker threads
         * @param provider Pointer to geometry adapter (implemented in game engine)
         *                 System uses it to call CastRay. Must not be null
         * @param settings Simulation settings (number of bounces, distance, multithreading)
         * @throws std::invalid_argument if provider is null
         */
        void Initialize(IGeometryProvider* provider, const Settings& settings = Settings());

        /**
         * @brief Stop calculations and free resources
         * @details Properly terminates worker thread. Recommended to call when unloading level
         */
        void Shutdown();

        // =============================================================================================
        // Simulation Update
        // =============================================================================================

        /**
         * @brief Main update method. Should be called each frame (in OnFrame)
         * @details Method sends new listener position to worker thread.
         * Tracing calculations occur asynchronously, not blocking game loop.
         * Results (EAX) are interpolated (smoothed) relative to frame time dt.
         * @param position Current listener position (Camera or Actor)
         * @param dt Delta time (time elapsed since previous frame) in seconds
         * @param envFogDensity Fog/rain density [0.0 - 1.0]. Affects high frequency absorption
         */
        void Update(const float3& position, float dt, float envFogDensity = 0.0f);

        // =============================================================================================
        // Data Access
        // =============================================================================================

        /**
         * @brief Returns current calculated reverberation parameters
         * @return EAXResult structure ready for sending to OpenAL/DirectSound
         * @note Data in structure is already smoothed (interpolated) to prevent sharp sound jumps
         */
        EAXResult GetEAXResult() const;

        /**
         * @brief Implementation of ISoundOcclusionCalculator interface
         * @details Calculates volume coefficient for specific sound source.
         * Considers obstacles (Transmission) and sound bending (Diffraction/Reflection)
         * @param listenerPos Listener position
         * @param sourcePos Sound source position
         * @return Volume coefficient [0.0 - silence, 1.0 - full volume]
         * @note This method can be called synchronously from engine's sound thread
         */
        virtual float CalculateOcclusion(const float3& listenerPos, const float3& sourcePos) override;

        // =============================================================================================
        // Material Management
        // =============================================================================================

        /**
         * @brief Set properties for existing material
         * @param materialID Material identifier (0 = Air, 1 = Stone, etc., or custom ID)
         * @param params New acoustic parameters
         * @return true if material exists and parameters were set
         */
        bool SetMaterialProperties(int materialID, const MaterialParams& params);

        /**
         * @brief Create new custom material with specified properties
         * @param params Acoustic parameters for new material
         * @return New material ID (positive integer) or -1 on failure
         */
        int CreateCustomMaterial(const MaterialParams& params);

        // =============================================================================================
        // System Status
        // =============================================================================================

        /**
         * @brief Check if system is initialized and ready
         * @return true if system is operational
         */
        bool IsInitialized() const;

        /**
         * @brief Get current system settings
         * @return Current settings structure
         */
        Settings GetCurrentSettings() const;

    private:
        // =============================================================================================
        // PIMPL Implementation
        // =============================================================================================
        // Forward declaration of implementation class
        class Impl;

        // Raw pointer to implementation to avoid DLL export issues with smart pointers
        Impl* m_Impl;

        // Disable copy semantics
        AudioSystem(const AudioSystem&) = delete;
        AudioSystem& operator=(const AudioSystem&) = delete;
    };
}
