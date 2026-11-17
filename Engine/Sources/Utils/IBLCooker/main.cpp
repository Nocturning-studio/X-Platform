////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <string>
#include <shellapi.h>
#include <filesystem>
#include "IBLCooker.h"
#include "log.h"
////////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "shell32.lib")
////////////////////////////////////////////////////////////////////////////////
namespace fs = std::filesystem;
////////////////////////////////////////////////////////////////////////////////
static void PrintUsage()
{
    Log(
        L"Usage: IBLCooker.exe [options]\n"
        "\n"
        "  (no args)         Process every .dds in <exe_dir>/cubemaps/\n"
        "  -all              Same as no args.\n"
        "  -src <file.dds>   Process a single cubemap.\n"
        "  -h | --help       Show this message.\n"
        "\n"
        "Output (per input cubemap, in the same directory):\n"
        "  <name>_ibl.dds        Combined color (mip0) + prefiltered specular (mips 1..N)\n"
        "  <name>_irradiance.dds Irradiance cubemap (32x32)\n");
}

static fs::path GetExeDir()
{
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    return fs::path(exePath).parent_path();
}

int main()
{
    Log(L"[IBLCooker] === Start ===");

    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        FILE* f; freopen_s(&f, "CONOUT$", "w", stdout); freopen_s(&f, "CONOUT$", "w", stderr);
    }

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::wstring srcPath;
    bool processAll = (argc <= 1);

    for (int i = 1; argv && i < argc; ++i)
    {
        std::wstring a = argv[i];
        if (a == L"-h" || a == L"--help") { PrintUsage(); LocalFree(argv); return 0; }
        else if (a == L"-all")            processAll = true;
        else if (a == L"-src" && i + 1 < argc) srcPath = argv[++i];
        else
        {
            Log((L"[IBLCooker] Unknown argument: " + a + L"\n").c_str());
            PrintUsage();
            if (argv) LocalFree(argv);
            return 1;
        }
    }
    if (argv) LocalFree(argv);

    Log(L"[IBLCooker] src='%s' processAll=%d", srcPath.c_str(), (int)processAll);

    IBLCooker baker;
    if (!baker.Init())
    {
        Log(L"[IBLCooker] Init failed.\n");
        return 1;
    }

    bool ok;
    if (!srcPath.empty() && !processAll)
    {
        ok = baker.ProcessFile(srcPath);
    }
    else
    {
        fs::path cubemapsDir = GetExeDir() / L"cubemaps";
        ok = baker.ProcessDirectory(cubemapsDir.wstring());
    }
    Log(L"[IBLCooker] === Done, ok=%d ===", (int)ok);
    baker.Shutdown();
    return ok ? 0 : 1;
}
////////////////////////////////////////////////////////////////////////////////
