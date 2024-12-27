@echo off
where cl >nul 2>&1
if %errorlevel% == 0 (
  exit /b
)

set "path1=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set "path2=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if exist "%path1%" (
    call "%path1%"
    bash
    exit /b
) else if exist "%path2%" (
    call "%path2%"
    bash
    exit /b
)

echo Error: Neither vcvars64.bat was found.
exit /b 1
