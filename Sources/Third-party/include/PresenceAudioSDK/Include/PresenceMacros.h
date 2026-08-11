/*
====================================================================================================
  Presence Audio SDK
  High-Performance Real-time Audio Path Tracing & EAX Simulation Library
====================================================================================================

  Copyright (c) 2026 Presence Collaboratory, NSDeathman & Gemini 3 & DeepSeek

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

#define PRESENCE_BEGIN namespace Presence {
#define PRESENCE_END }

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

// =================================================================================================
// MEMORY ALIGNMENT MACRO
// =================================================================================================
// For efficient SSE operation, data must be aligned to 16-byte boundary.
// If memory address is not multiple of 16, _mm_load_ps will cause crash (Access Violation).
// =================================================================================================
#if defined(_MSC_VER)
#define PRESENCE_ALIGN(x) __declspec(align(x))
#else
#define PRESENCE_ALIGN(x) __attribute__((aligned(x)))
#endif
