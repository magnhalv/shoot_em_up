@echo off
setlocal EnableDelayedExpansion

set ROOT=%~dp0..
set FOUND_ERRORS=0

for %%F in ("%ROOT%\*.log") do (
    findstr /i /r "error C[0-9][0-9][0-9][0-9] error LNK[0-9][0-9][0-9][0-9]" "%%F" > NUL 2>&1
    if !errorlevel! == 0 (
        echo [%%~nxF]
        findstr /i /r "error C[0-9][0-9][0-9][0-9] error LNK[0-9][0-9][0-9][0-9]" "%%F"
        echo.
        set FOUND_ERRORS=1
    )
)

if !FOUND_ERRORS! == 0 (
    echo No errors found in any log files.
) else (
    echo Errors were found.
    exit /b 1
)
