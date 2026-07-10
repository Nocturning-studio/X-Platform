#pragma once

#ifdef ENGINE_BUILD
#define DLL_API __declspec(dllimport)
#define ENGINE_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllexport)
#define ENGINE_API __declspec(dllimport)
#endif
