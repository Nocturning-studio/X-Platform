// pch_lua.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// Third generation by Oles.

#ifndef PCH_LUA_H
#define PCH_LUA_H

#pragma once

#include <xrCore/xrCore.h>
#pragma comment(lib, "xrCore.lib")

// #include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #include "xr_LuaAllocator.h"

/*
#include "src\lua.h"
#include "src\lauxlib.h"
#include "src\lualib.h"
*/

#if !defined(DEBUG) || defined(FORCE_NO_EXCEPTIONS)
// release: no error checking, no exceptions
#define LUABIND_NO_EXCEPTIONS
#define BOOST_THROW_EXCEPTION_HPP_INCLUDED

namespace boost
{
	__declspec(dllexport) void throw_exception(std::exception const&);
} // namespace boost
#endif
#define LUABIND_DONT_COPY_STRINGS

#pragma comment(lib, "lua51.lib")

#endif // stdafxH
