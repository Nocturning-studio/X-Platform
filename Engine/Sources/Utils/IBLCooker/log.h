////////////////////////////////////////////////////////////////////////////////
// Created: 17.11.2026 20:46:13
// Author: NS_Deathman
// File: log.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <string>
////////////////////////////////////////////////////////////////////////////////
inline void Log(const wchar_t* fmt, ...)
{
    wchar_t buf[2048];
    va_list ap; va_start(ap, fmt);
    vswprintf_s(buf, fmt, ap);
    va_end(ap);

    OutputDebugStringW(buf);
    OutputDebugStringW(L"\n");

    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring logPath(exePath);
    logPath = logPath.substr(0, logPath.find_last_of(L"\\/") + 1) + L"IBLCooker.log";

    FILE* f = nullptr;
    if (_wfopen_s(&f, logPath.c_str(), L"a, ccs=UTF-8") == 0 && f)
    {
        fputws(buf, f);
        fputws(L"\n", f);
        fclose(f);
    }
}
////////////////////////////////////////////////////////////////////////////////
