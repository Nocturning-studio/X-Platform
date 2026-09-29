@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

REM ============================================================
REM  Установка кастомных шаблонов Visual Studio
REM  Nocturning studio for X-Platform
REM ============================================================

REM --- Корень репозитория (папка, где лежит этот .bat) ---
set "REPO_ROOT=%~dp0"

REM --- Папки-источники относительно .bat-файла ---
set "SRC_ITEM=%REPO_ROOT%Sources\VSTemplates\ItemTemplates"
set "SRC_PROJECT=%REPO_ROOT%Sources\VSTemplates\ProjectTemplates"

REM --- Определяем папку Documents (учитываем OneDrive) ---
if exist "%USERPROFILE%\OneDrive\Documents" (
    set "DOCS=%USERPROFILE%\OneDrive\Documents"
) else (
    set "DOCS=%USERPROFILE%\Documents"
)

echo.
echo ============================================================
echo   Установка шаблонов Visual Studio
echo ============================================================
echo.

REM --- Проверяем наличие исходных папок ---
if not exist "%SRC_ITEM%" if not exist "%SRC_PROJECT%" (
    echo [ОШИБКА] Не найдены папки "ItemTemplates" или "ProjectTemplates"
    echo          рядом с этим .bat-файлом.
    echo.
    pause
    exit /b 1
)

REM --- Ищем все установленные версии Visual Studio ---
set "FOUND=0"

for %%V in (
    "2022"
    "2019"
    "2017"
) do (
    set "VSVER=%%~V"
    set "DST_ROOT=!DOCS!\Visual Studio !VSVER!\Templates"

    if exist "!DOCS!\Visual Studio !VSVER!" (
        set "FOUND=1"
        echo [НАЙДЕНО] Visual Studio !VSVER!
        echo          Папка: !DST_ROOT!
        echo.

        REM --- Копируем шаблоны элементов ---
        if exist "%SRC_ITEM%" (
            if not exist "!DST_ROOT!\ItemTemplates" mkdir "!DST_ROOT!\ItemTemplates"
            xcopy "%SRC_ITEM%\*" "!DST_ROOT!\ItemTemplates\" /E /Y /I >nul
            if !errorlevel! equ 0 (
                echo    [OK] Шаблоны элементов скопированы.
            ) else (
                echo    [ОШИБКА] Не удалось скопировать шаблоны элементов.
            )
        )

        REM --- Копируем шаблоны проектов ---
        if exist "%SRC_PROJECT%" (
            if not exist "!DST_ROOT!\ProjectTemplates" mkdir "!DST_ROOT!\ProjectTemplates"
            xcopy "%SRC_PROJECT%\*" "!DST_ROOT!\ProjectTemplates\" /E /Y /I >nul
            if !errorlevel! equ 0 (
                echo    [OK] Шаблоны проектов скопированы.
            ) else (
                echo    [ОШИБКА] Не удалось скопировать шаблоны проектов.
            )
        )

        echo.
    )
)

if "%FOUND%"=="0" (
    echo [ОШИБКА] Не найдена ни одна установленная версия Visual Studio
    echo          в папке "%DOCS%".
    echo.
    echo Проверьте путь вручную и убедитесь, что VS запускалась хотя бы раз.
    echo.
    pause
    exit /b 1
)

echo ============================================================
echo   Готово! Перезапустите Visual Studio, чтобы шаблоны
echo   появились в окне "Добавить новый элемент / проект".
echo ============================================================
echo.

REM --- Очистка кэша шаблонов (опционально) ---
set /p CLEAR_CACHE="Очистить кэш шаблонов VS (рекомендуется, y/n)? "
if /i "%CLEAR_CACHE%"=="y" (
    echo Очистка кэша...
    del /q "%LOCALAPPDATA%\Microsoft\VisualStudio\*\TemplateCache\*" 2>nul
    echo    [OK] Кэш очищен.
)

echo.
pause
endlocal
