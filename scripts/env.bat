@echo off
where cl >nul 2>&1
if %errorlevel% neq 0 (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
  bash
) 
