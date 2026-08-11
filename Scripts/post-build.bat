@echo off
setlocal enabledelayedexpansion

:: ============================================================
:: post-build.bat – копирование runtime-зависимостей и конфигов
:: Параметры:
::   %1 = платформа (x86, x64, ARM...)
::   %2 = выходная папка бинарников (OutDir)
::   %3 = папка решения (SolutionDir = Engine\Sources\)
:: ============================================================

if "%~1"=="" (
    echo ERROR: Platform parameter is missing.
    exit /b 1
)
if "%~2"=="" (
    echo ERROR: OutDir parameter is missing.
    exit /b 1
)
if "%~3"=="" (
    echo ERROR: SolutionDir parameter is missing.
    exit /b 1
)

set "PLATFORM=%~1"
set "OUTDIR=%~2"
set "SOLUTIONDIR=%~3"

:: Корень игры: SolutionDir\..
for %%i in ("%SOLUTIONDIR%\..") do set "GAMEROOT=%%~fi"

echo Deploying engine dependencies for %PLATFORM%...
echo   OutDir       = %OUTDIR%
echo   SolutionDir  = %SOLUTIONDIR%
echo   GameRoot     = %GAMEROOT%

if not exist "%OUTDIR%" (
    mkdir "%OUTDIR%"
    if errorlevel 1 (
        echo ERROR: Cannot create output directory "%OUTDIR%".
        exit /b 1
    )
)

:: --------------------------------------------------
:: Копируем DLL из Third-Party (лежат в Sources\Third-Party\bin\%PLATFORM%)
:: --------------------------------------------------
set "THIRDPARTY_BIN=%SOLUTIONDIR%\Sources\Third-Party\bin\%PLATFORM%"
if exist "%THIRDPARTY_BIN%\*.dll" (
    echo   Copying DLLs from "%THIRDPARTY_BIN%"...
    xcopy /Y /Q "%THIRDPARTY_BIN%\*.dll" "%OUTDIR%\"
) else (
    echo   WARNING: Third-Party DLL folder not found: "%THIRDPARTY_BIN%"
)

:: --------------------------------------------------
:: Копируем game_filesystem.ltx в корень игры
:: --------------------------------------------------
set "CONFIG_FILE=%SOLUTIONDIR%\Config\game_filesystem.ltx"
if exist "%CONFIG_FILE%" (
    echo   Copying game_filesystem.ltx to "%GAMEROOT%"
    copy /Y "%CONFIG_FILE%" "%GAMEROOT%"
) else (
    echo   WARNING: game_filesystem.ltx not found at "%CONFIG_FILE%"
)

echo Deployment finished.
exit /b 0
