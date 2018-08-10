@echo off

set     ZIP_PATH=C:\Program Files\7-Zip\7z.exe
set WIN_SDK_PATH=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\
set      VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio 12.0\

if exist "appName.bat" (
	call appName.bat
) else (
	call ..\\appName.bat
)

set scriptpath=%~d0%~p0
cd %scriptpath%

set PLATFORM=win32
set PLATFORM2=x86
set BUILD_FOLDER=buildWin32

if NOT "%~4"=="-ship" goto buildSetupEnd
	set BUILD_FOLDER=releaseBuild
	if exist "..\%BUILD_FOLDER%" rmdir "..\%BUILD_FOLDER%" /S /Q
:buildSetupEnd

if not exist "..\%BUILD_FOLDER%" mkdir "..\%BUILD_FOLDER%"
pushd "..\%BUILD_FOLDER%"

set INC=
set LINC=
set LINKER_LIBS=

set          INC=%INC% -I"%VS_PATH%VC\include"
set          INC=%INC% -I"%WIN_SDK_PATH%Include"

set                  PATH=%VS_PATH%VC\bin;%PATH%
set LINC=%LINC% -LIBPATH:"%VS_PATH%VC\lib"
set LINC=%LINC% -LIBPATH:"%WIN_SDK_PATH%Lib"

set BUILD_MODE=-Od
if "%~3"=="-release" set BUILD_MODE=-O2

set COMPILER_OPTIONS= -MD %BUILD_MODE% -nologo -Oi -FC -wd4005 -GR- -EHa- -Z7 -fp:fast
set LINKER_OPTIONS= -link -SUBSYSTEM:CONSOLE -OUT:"%APP_NAME:"=%.exe" -incremental:no -opt:ref

cl %COMPILER_OPTIONS% ..\code\main.cpp -DA_NAME=%APP_NAME% %INC% %LINKER_OPTIONS% %LINC% %LINKER_LIBS%

if NOT "%~4"=="-ship" goto packShippingFolderEnd

	cd ..

	if "%~3"=="" goto nodelete
		del ".\%BUILD_FOLDER%\*.pdb"
		del ".\%BUILD_FOLDER%\*.exp"
		del ".\%BUILD_FOLDER%\*.lib"
		del ".\%BUILD_FOLDER%\*.obj"
	:nodelete

	set RELEASE_FOLDER=.\releases\%PLATFORM%\%APP_NAME:"=%
	if exist "%RELEASE_FOLDER%" rmdir "%RELEASE_FOLDER%" /S /Q
	mkdir "%RELEASE_FOLDER%"

	xcopy %BUILD_FOLDER% "%RELEASE_FOLDER%" /E /Q

	rmdir ".\%BUILD_FOLDER%" /S /Q

	"%ZIP_PATH%" a "%RELEASE_FOLDER% %PLATFORM2%.zip" "%RELEASE_FOLDER%"

:packShippingFolderEnd

IF "%~2"=="-run" call "%APP_NAME:"=%.exe"
