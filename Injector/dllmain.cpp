// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

#include "config.h"
#include "detour.h"
#include "log.h"

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		LogInit();
		ConfigInit();
		DetourInit();
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		DetourClose();
		LogClose();
	}
	return TRUE;
}

