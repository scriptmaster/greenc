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
move /y "bin\bootstrap\greenc.exe" "greenc.exe" >nul
if errorlevel 1 goto done
echo Built "%CD%\greenc.exe"
set "bootstrap_result=0"

:done
if not "%bootstrap_result%"=="0" echo Compiler bootstrap failed. Existing greenc.exe was preserved.
if not "%~1"=="--no-pause" pause
exit /b %bootstrap_result%
