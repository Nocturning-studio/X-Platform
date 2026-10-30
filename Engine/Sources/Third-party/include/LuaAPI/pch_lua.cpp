// pch_lua.cpp : source file that includes just the standard includes
// pch_lua.obj will contain the pre-compiled type information

#include "pch_lua.h"

#pragma comment(lib, "lua51.lib")

namespace boost
{
	__declspec(dllexport) void throw_exception(std::exception const&) { abort(); }
} // namespace boost
