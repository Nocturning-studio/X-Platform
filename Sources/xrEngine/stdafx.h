#ifndef STDAFX_3DA
#define STDAFX_3DA

#pragma once

#define WIN32_LEAN_AND_MEAN

#ifdef _EDITOR
#include "..\editors\ECore\stdafx.h"
#else

#define USE_LOG_IMPL
#include "../xrCore/xrCore.h"

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif

#pragma warning(disable : 4995)
#include <d3d9.h>
#include <DPlay\dplay8.h>
#pragma warning(default : 4995)

// you must define ENGINE_BUILD then building the engine itself
// and not define it if you are about to build DLL
#if !defined(NO_ENGINE_API)
#ifdef ENGINE_BUILD
#define DLL_API __declspec(dllimport)
#define ENGINE_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllexport)
#define ENGINE_API __declspec(dllimport)
#endif
#else
#define ENGINE_API
#define DLL_API
#endif // NO_ENGINE_API

#define ECORE_API

// Our headers
#include "Engine.h"
#include "defines.h"
#ifndef NO_XRLOG
#include "log.h"
#endif
#include "device.h"
#include "fs.h"

#include "xrXRC.h"

#include "../xrSound/sound.h"

extern ENGINE_API CInifile* pGameIni;

#ifndef DEBUG
#define LUABIND_NO_ERROR_CHECKING
#endif

#if !defined(DEBUG) || defined(FORCE_NO_EXCEPTIONS)
// release: no error checking, no exceptions
#define LUABIND_NO_EXCEPTIONS
#define BOOST_THROW_EXCEPTION_HPP_INCLUDED
namespace std
{
	class exception;
}
namespace boost
{
ENGINE_API void throw_exception(const std::exception& A);
};
#endif
#define LUABIND_DONT_COPY_STRINGS

#pragma warning(disable : 4995)
#include <xmmintrin.h>
#pragma warning(default : 4995)

#include <optick/optick.h>

#endif // !M_BORLAND
#endif // !defined STDAFX_3DA
