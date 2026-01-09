////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: NSDeathman
// Refactored code: Windows application entry point
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "resource.h"
#include "Engine.h"
////////////////////////////////////////////////////////////////////////////////
ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
////////////////////////////////////////////////////////////////////////////////
int stack_overflow_exception_filter(int exception_code)
{
	if (exception_code == EXCEPTION_STACK_OVERFLOW)
		return EXCEPTION_EXECUTE_HANDLER;
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

void OnApplicationExit()
{
	// check for need to execute something external
	if (xr_strlen(g_sLaunchOnExit_app))
	{
		char* _args[3];
		string4096 ModuleFileName = "";
		GetModuleFileName(NULL, ModuleFileName, 4096);

		string4096 ModuleFilePath = "";
		char* ModuleName = NULL;
		GetFullPathName(ModuleFileName, 4096, ModuleFilePath, &ModuleName);
		ModuleName[0] = 0;
		strcat(ModuleFilePath, g_sLaunchOnExit_app);
		_args[0] = g_sLaunchOnExit_app;
		_args[1] = g_sLaunchOnExit_params;
		_args[2] = NULL;

		_spawnv(_P_NOWAIT, _args[0], _args);
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
	__try
	{
		g_sLaunchOnExit_app[0] = NULL;
		g_sLaunchOnExit_params[0] = NULL;

		g_Engine = xr_new<CEngine>();
		g_Engine->Run();
		xr_delete(g_Engine);

		OnApplicationExit();
	}
	__except (stack_overflow_exception_filter(GetExceptionCode()))
	{
		_resetstkoflw();
		FATAL("stack overflow");
	}

	return (0);
}
