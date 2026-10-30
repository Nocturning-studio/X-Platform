// xrRender.cpp : Defines the entry point for the DLL application.
//
#include "pch.h"
#include "xrRender_console.h"

#ifndef ENGINE_STATIC
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		::Render = &RenderImplementation;
		xrRender_initconsole();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#else
void xrRender_Init()
{
	static bool initialized = false;
	if (initialized)
		return;
	initialized = true;

	::Render = &RenderImplementation;
	xrRender_initconsole();
}

void xrRender_Shutdown()
{
	::Render = nullptr;
}
#endif
