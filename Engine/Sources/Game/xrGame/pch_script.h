////////////////////////////////////////////////////////////////////////////
//	Module 		: pch_script.h
//	Created 	: 23.05.2007
//  Modified 	: 23.05.2007
//	Author		: Dmitriy Iassenev
//	Description : precompiled header for lua and luabind users
////////////////////////////////////////////////////////////////////////////

#ifndef PCH_SCRIPT_H
#define PCH_SCRIPT_H

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING

#include "pch.h"

// #pragma warning(disable:4244)
// #pragma warning(disable:4995)
// #pragma warning(disable:4530)
// #pragma warning(disable:4267)

extern "C"
{
#include <LuaJIT-2.1.0-beta3\src\lua.h>
#include <LuaJIT-2.1.0-beta3\src\lualib.h>
#include <LuaJIT-2.1.0-beta3\src\lauxlib.h>
#include <LuaJIT-2.1.0-beta3\src\luajit.h>
};

#pragma warning(push)
#pragma warning(disable : 4995)
#include <LuaAPI/luabind/luabind.hpp>
#pragma warning(pop)

#include <LuaAPI/luabind/object.hpp>
#include <LuaAPI/luabind/functor.hpp>
#include <LuaAPI/luabind/operator.hpp>
#include <LuaAPI/luabind/adopt_policy.hpp>
#include <LuaAPI/luabind/return_reference_to_policy.hpp>
#include <LuaAPI/luabind/out_value_policy.hpp>
#include <LuaAPI/luabind/iterator_policy.hpp>

#endif // PCH_SCRIPT_H
