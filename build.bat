@echo off
setlocal
cd /d "%~dp0"
set "bootstrap_result=1"
set "bootstrap_cc=gcc"
if defined CC set "bootstrap_cc=%CC%"
where /q "%bootstrap_cc%"
if not errorlevel 1 goto compile
if defined CC goto missing_compiler
if exist "C:\msys64\ucrt64\bin\gcc.exe" set "bootstrap_cc=C:\msys64\ucrt64\bin\gcc.exe"
if exist "%bootstrap_cc%" goto compile

:missing_compiler
echo Install the MSYS2 UCRT64 GCC toolchain, or set CC to a GCC executable.
goto done

:compile
if not exist "bin\bootstrap" mkdir "bin\bootstrap"
if errorlevel 1 goto done
"%bootstrap_cc%" -Os -std=c17 -DGREENC_MAIN -static-libgcc -ffreestanding -fno-builtin -nostdlib -Icompiler/libs compiler/main.c -o bin/bootstrap/greenc.exe -lshell32 -lkernel32 -luser32 -Wl,-egreenc_start
if errorlevel 1 goto done
> "bin\bootstrap\smoke.c" echo int main^(void^) { return 0; }
bin\bootstrap\greenc.exe bin/bootstrap/smoke.c --hosted-module -o bin/bootstrap/smoke.generated.c
if errorlevel 1 goto done
if not exist "compiler\libs\greencsharp\bin" mkdir "compiler\libs\greencsharp\bin"
if errorlevel 1 goto done
"%bootstrap_cc%" -Os -std=c17 -Wall -Wextra -Werror -DGREENC_CSHARP_BUILD -Icompiler/libs -Icompiler/libs/greencsharp/include -shared compiler/libs/greencsharp/src/*.c -o bin/bootstrap/greencsharp.dll
if errorlevel 1 goto done
> "bin\bootstrap\smoke.cs" echo System.Console.WriteLine^(42^);
set "GREENC_CSHARP_CC=%bootstrap_cc%"
bin\bootstrap\greenc.exe bin/bootstrap/smoke.cs --emit-c -o bin/bootstrap/csharp-smoke.c
if errorlevel 1 goto done
"%bootstrap_cc%" -Os -std=c17 -Wall -Wextra -Werror -Icompiler/libs compiler/test/greencsharp_api.c -o bin/bootstrap/greencsharp-api.exe
if errorlevel 1 goto done
bin\bootstrap\greencsharp-api.exe "%CD%\bin\bootstrap\greencsharp.dll" bin/bootstrap/smoke.cs
if errorlevel 1 goto done
move /y "bin\bootstrap\greencsharp.dll" "compiler\libs\greencsharp\bin\greencsharp.dll" >nul
if errorlevel 1 goto done
move /y "bin\bootstrap\greenc.exe" "greenc.exe" >nul
if errorlevel 1 goto done
echo Built "%CD%\greenc.exe"
set "bootstrap_result=0"

:done
if not "%bootstrap_result%"=="0" echo Compiler bootstrap failed. Existing greenc.exe was preserved.
if not "%~1"=="--no-pause" pause
exit /b %bootstrap_result%
