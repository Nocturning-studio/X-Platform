#pragma once

#ifdef ENGINE_BUILD
#define DLL_API __declspec(dllimport)
#else
#define DLL_API __declspec(dllexport)
#endif

#if defined(ENGINE_BUILD)
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif
