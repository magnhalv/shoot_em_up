@echo off
setlocal EnableDelayedExpansion

set CommonCompilerFlags=-Od -MTd -nologo -fp:fast -fp:except- /GR- /EHa- /Z7 /Oi /WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4127 -D_CRT_SECURE_NO_WARNINGS -FC /std:c++20 /arch:AVX2
set CommonCompilerFlags=-DHOMEMADE_DEBUG=1 -DHOMEMADE_SLOW=1 -DHOMEMADE_WIN32=1 %CommonCompilerFlags%
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib ole32.lib
set CommonInclude=/I..\src\ /I..\include\


IF NOT EXIST build mkdir build
pushd build
del *.pdb > NUL 2> NUL

:loop
if "%~1"=="" goto done

if /I "%~1"=="engine" call :engine
if /I "%~1"=="main" call :main
if /I "%~1"=="software_renderer" call :software_renderer
if /I "%~1"=="opengl_renderer" call :opengl_renderer
if /I "%~1"=="tests" call :tests
if /I "%~1"=="asset_builder" call :asset_builder

shift
goto loop

:engine
REM 64-bit build
set CompileCmd=cl %CommonCompilerFlags% %CommonInclude% ..\src\engine\unit.cpp -LD /Feengine_dyn /link -incremental:no -opt:ref
echo %CompileCmd%
call %CompileCmd%
exit /b


:main
set CompileCmd=cl %CommonCompilerFlags% %CommonInclude% ..\src\win32_main.cpp /Feshoot_em_up.exe /link %CommonLinkerFlags%
echo %CompileCmd%
call %CompileCmd%
exit /b

:asset_builder
set CompileCmd=cl %CommonCompilerFlags% %CommonInclude% ..\src\asset_builder_main.cpp /Feasset_builder.exe /link %CommonLinkerFlags%
echo %CompileCmd%
call %CompileCmd%
exit /b


:software_renderer
set CompileCmd=cl %CommonCompilerFlags% %CommonInclude% ..\src\renderers\win32_software_renderer.cpp -LD /Fewin32_software_renderer /link %CommonLinkerFlags%
echo %CompileCmd%
call %CompileCmd%
exit /b


:opengl_renderer
set GladSrcFiles=..\shared_deps\glad\src\gl.c ..\shared_deps\glad\src\wgl.c 
set CompileCmd=cl %CommonCompilerFlags% %CommonInclude% /I..\shared_deps\glad\include\ %GladSrcFiles% ..\..\src\renderers\win32_opengl_renderer.cpp -LD /Feopengl_renderer /link %CommonLinkerFlags%
echo %CompileCmd%
call %CompileCmd%
exit /b

:tests
set CompileCmd=cl %CommonCompilerFlags% -DENGINE_TEST -DDOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS /wd4611 /wd4702 %CommonInclude% ..\tests\test_main.cpp /Fetests.exe /link %CommonLinkerFlags%
echo %CompileCmd%
call %CompileCmd%
exit /b


:done
popd
exit /b
