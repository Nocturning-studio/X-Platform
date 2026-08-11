#pragma once

#ifdef XRCORE_EXPORTS
#define XRCORE_API __declspec(dllexport)
#else
#define XRCORE_API __declspec(dllimport)
#endif

#ifndef DEBUG
#ifdef _DEBUG
#define DEBUG
#endif
#ifdef MIXED
#define DEBUG
#endif
#endif

// inline control - redefine to use compiler's heuristics ONLY
// it seems "IC" is misused in many places which cause code-bloat
// ...and VC7.1 really don't miss opportunities for inline :)
#define _inline inline
#define __inline inline
#define IC inline
#define ICF __forceinline // !!! this should be used only in critical places found by PROFILER
#define ICN __declspec(noinline)
