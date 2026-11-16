@rem Script to build LuaJIT with MSVC.
@rem Copyright (C) 2005-2017 Mike Pall. See Copyright Notice in luajit.h
@rem
@rem Usage:
@rem   msvcbuild.bat [gc64] [debug] [amalg^|static] ^<src_dir^> ^<lib_dir^> ^<int_dir^> [forced]
@rem
@rem   src_dir  - LuaJIT source tree (contains host\, jit\, lj_*.c, etc.)
@rem   lib_dir  - where lua51.lib / lua51.dll are placed (created if missing)
@rem   int_dir  - scratch for *.obj, minilua.exe, buildvm.exe, generated headers
@rem   forced   - rebuild even if lua51.lib already exists

@if not defined INCLUDE goto :FAIL
@setlocal

@rem ---- parse mode flags ----
@set LJGC64=
@set LJDEBUG=
@set LJMODE=static
@set LJFORCED=

:PARSE
@if "%~1"=="" goto :PARSED
@if /I "%~1"=="gc64"   (set "LJGC64=1"&shift&goto :PARSE)
@if /I "%~1"=="debug"  (set "LJDEBUG=1"&shift&goto :PARSE)
@if /I "%~1"=="amalg"  (set "LJMODE=amalg"&shift&goto :PARSE)
@if /I "%~1"=="static" (set "LJMODE=static"&shift&goto :PARSE)
@if /I "%~1"=="forced" (set "LJFORCED=1"&shift&goto :PARSE)
@goto :PARSED
:PARSED

@if "%~3"=="" goto :USAGE

@set LJSRCDIR=%~f1
@set LJOUTDIR=%~f2
@set LJINTDIR=%~f3
@if /I "%~4"=="forced" set "LJFORCED=1"

@if not "%LJSRCDIR:~-1%"=="\" set LJSRCDIR=%LJSRCDIR%\
@if not "%LJOUTDIR:~-1%"=="\" set LJOUTDIR=%LJOUTDIR%\
@if not "%LJINTDIR:~-1%"=="\" set LJINTDIR=%LJINTDIR%\

@rem "S" = stripped variant, safe to embed inside /I"..." (no trailing backslash!)
@set LJINTDIRS=%LJINTDIR:~0,-1%
@set LJOUTDIRS=%LJOUTDIR:~0,-1%
@rem /Fo needs "...\\" -- a lone trailing \" is eaten by the CRT arg parser.
@set LJINTFO=%LJINTDIRS%\\

@if not defined LJFORCED if exist "%LJOUTDIRS%\lua51.lib" (
    @echo lua51.lib already exists in "%LJOUTDIRS%" -- pass "forced" to rebuild.
    @goto :END
)

@if not exist "%LJINTDIRS%" mkdir "%LJINTDIRS%"
@if errorlevel 1 goto :BAD
@if not exist "%LJOUTDIRS%" mkdir "%LJOUTDIRS%"
@if errorlevel 1 goto :BAD

@set LJCOMPILE=cl /nologo /c /O2 /W3 /MD /D_CRT_SECURE_NO_DEPRECATE /D_CRT_STDIO_INLINE=__declspec(dllexport)__inline
@set LJLINK=link /nologo
@set LJMT=mt /nologo
@set LJLIB=lib /nologo /nodefaultlib
@set DASMDIR=%LJSRCDIR%..\dynasm
@set DASM=%DASMDIR%\dynasm.lua
@set DASC=%LJSRCDIR%vm_x86.dasc
@set LJDLLNAME=lua51.dll
@set LJLIBNAME=lua51.lib
@set ALL_LIB=lib_base.c lib_math.c lib_bit.c lib_string.c lib_table.c lib_io.c lib_os.c lib_package.c lib_debug.c lib_jit.c lib_ffi.c

@cd /D "%LJSRCDIR%"

@rem ---- 1/4: minilua ----
@echo [1/4] minilua
%LJCOMPILE% /Fo"%LJINTFO%" host\minilua.c
@if errorlevel 1 goto :BAD
%LJLINK% /out:"%LJINTDIRS%\minilua.exe" "%LJINTDIRS%\minilua.obj"
@if errorlevel 1 goto :BAD
@if exist "%LJINTDIRS%\minilua.exe.manifest" %LJMT% -manifest "%LJINTDIRS%\minilua.exe.manifest" -outputresource:"%LJINTDIRS%\minilua.exe"

@set DASMFLAGS=-D WIN -D JIT -D FFI -D P64
@set LJARCH=x64
@"%LJINTDIRS%\minilua.exe"
@if errorlevel 8 goto :X64
@set DASMFLAGS=-D WIN -D JIT -D FFI
@set LJARCH=x86
@set LJCOMPILE=%LJCOMPILE% /arch:SSE2
:X64
@if not defined LJGC64 goto :NOGC64
@set DASC=%LJSRCDIR%vm_x64.dasc
@set LJCOMPILE=%LJCOMPILE% /DLUAJIT_ENABLE_GC64
:NOGC64
@"%LJINTDIRS%\minilua.exe" "%DASM%" -LN %DASMFLAGS% -o "%LJINTDIRS%\buildvm_arch.h" "%DASC%"
@if errorlevel 1 goto :BAD

@rem ---- 2/4: buildvm ----
@echo [2/4] buildvm
%LJCOMPILE% /I"." /I"%DASMDIR%" /I"%LJINTDIRS%" /Fo"%LJINTFO%" host\buildvm*.c
@if errorlevel 1 goto :BAD
@pushd "%LJINTDIRS%"
%LJLINK% /out:buildvm.exe buildvm*.obj
@if errorlevel 1 ( @popd & goto :BAD )
@popd
@if exist "%LJINTDIRS%\buildvm.exe.manifest" %LJMT% -manifest "%LJINTDIRS%\buildvm.exe.manifest" -outputresource:"%LJINTDIRS%\buildvm.exe"

@"%LJINTDIRS%\buildvm.exe" -m peobj  -o "%LJINTDIRS%\lj_vm.obj"     %ALL_LIB%
@if errorlevel 1 goto :BAD
@"%LJINTDIRS%\buildvm.exe" -m bcdef  -o "%LJINTDIRS%\lj_bcdef.h"    %ALL_LIB%
@if errorlevel 1 goto :BAD
@"%LJINTDIRS%\buildvm.exe" -m ffdef  -o "%LJINTDIRS%\lj_ffdef.h"    %ALL_LIB%
@if errorlevel 1 goto :BAD
@"%LJINTDIRS%\buildvm.exe" -m libdef -o "%LJINTDIRS%\lj_libdef.h"   %ALL_LIB%
@if errorlevel 1 goto :BAD
@"%LJINTDIRS%\buildvm.exe" -m recdef -o "%LJINTDIRS%\lj_recdef.h"   %ALL_LIB%
@if errorlevel 1 goto :BAD
@"%LJINTDIRS%\buildvm.exe" -m vmdef  -o "jit\vmdef.lua"             %ALL_LIB%
@if errorlevel 1 goto :BAD
@"%LJINTDIRS%\buildvm.exe" -m folddef -o "%LJINTDIRS%\lj_folddef.h" lj_opt_fold.c
@if errorlevel 1 goto :BAD

@rem ---- 3/4: debug flags ----
@if not defined LJDEBUG goto :NODEBUG
@set LJCOMPILE=%LJCOMPILE% /Zi /Fd"%LJINTDIRS%\vc.pdb"
@set LJLINK=%LJLINK% /debug /opt:ref /opt:icf /incremental:no
:NODEBUG

@rem ---- 4/4: main lib ----
@echo [3/4] %LJMODE% library
@if /I "%LJMODE%"=="amalg" goto :AMALG

%LJCOMPILE% /I"." /I"%LJINTDIRS%" /Fo"%LJINTFO%" lj_*.c lib_*.c
@if errorlevel 1 goto :BAD
@pushd "%LJINTDIRS%"
%LJLIB% /OUT:"%LJOUTDIRS%\%LJLIBNAME%" lj_*.obj lib_*.obj
@if errorlevel 1 ( @popd & goto :BAD )
@popd
@goto :BUILDEXE

:AMALG
%LJCOMPILE% /DLUA_BUILD_AS_DLL /I"." /I"%LJINTDIRS%" /Fo"%LJINTFO%" ljamalg.c
@if errorlevel 1 goto :BAD
@pushd "%LJINTDIRS%"
%LJLINK% /DLL /out:"%LJOUTDIRS%\%LJDLLNAME%" ljamalg.obj lj_vm.obj
@if errorlevel 1 ( @popd & goto :BAD )
@popd
@if exist "%LJOUTDIRS%\%LJDLLNAME%.manifest" %LJMT% -manifest "%LJOUTDIRS%\%LJDLLNAME%.manifest" -outputresource:"%LJOUTDIRS%\%LJDLLNAME%;2"

:BUILDEXE
@echo [4/4] luajit.exe
%LJCOMPILE% /I"." /I"%LJINTDIRS%" /Fo"%LJINTFO%" luajit.c
@if errorlevel 1 goto :BAD
@pushd "%LJINTDIRS%"
%LJLINK% /out:luajit.exe luajit.obj "%LJOUTDIRS%\%LJLIBNAME%"
@if errorlevel 1 ( @popd & goto :BAD )
@popd
@if exist "%LJINTDIRS%\luajit.exe.manifest" %LJMT% -manifest "%LJINTDIRS%\luajit.exe.manifest" -outputresource:"%LJINTDIRS%\luajit.exe"

@echo.
@echo === Successfully built LuaJIT for Windows/%LJARCH% ===
@echo   output:  %LJOUTDIRS%\%LJLIBNAME%
@echo   scratch: %LJINTDIRS%
@goto :END

:BAD
@echo.
@echo *******************************************************
@echo *** Build FAILED -- Please check the error messages ***
@echo *******************************************************
@goto :END

:USAGE
@echo Usage: %~nx0 [gc64] [debug] [amalg^|static] ^<src_dir^> ^<lib_dir^> ^<int_dir^> [forced]

:FAIL
@echo You must open a "Visual Studio .NET Command Prompt" to run this script
:END
@endlocal
