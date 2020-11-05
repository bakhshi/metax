@echo off
setlocal

echo #
echo ############## Starting checking required tools for building #############

rem Checks Visual Studio Community 2015
if defined VisualStudioVersion (
	echo # DEFINED Visual Studio Version: "%VisualStudioVersion%"
) else (
	if exist "C:\Program Files\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat" (
		echo # Setting up VS build environment ...
		call "C:\Program Files\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
	) else (
		if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat" (
			echo # Setting up VS build environment ...
			call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"
		) else (
			echo # FAIL:
			echo # Can not find Visual Studio 2015 version environment variable.
			echo # Need to install Visual Studio Community 2015
			echo # Or to run from Developer Command Prompt for VS2015
			echo # See the current readme file
			echo #
			call :end
			exit /b 0
		)
	)
)

rem Checks Poco lib paths
if defined POCO_BASE (
	echo # DEFINED Poco path: %POCO_BASE%
) else (
	echo # FAIL:
	echo # Can not find POCO_BASE environment variable set it by command:
	echo # Or see the current readme file
	call :end
	exit /b 0
)

if "%1"=="" (
	set ACTION=build
) else (
	set ACTION=%1
	echo # Set config to: %1
)
if "%2"=="" (
	set CONFIG=Release
) else (
	set CONFIG=%2
	echo # Set config to: %2
)
if "%3"=="" (
	if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
		set PLATFORM=x64
	) else (
		set PLATFORM=%PROCESSOR_ARCHITECTURE%
	)
) else (
	set PLATFORM=%3
	echo # Set config to: %3
)

echo #
echo ####################### Setting configs #######################
echo #

:: If need to change msbuild configs replace its values below
set BUILD_TOOL=msbuild
set EXTRASW=/m
set ACTIONSW=/t:
set CONFIGSW=/p:Configuration=
set PLATFORMSW=/p:Platform=%PLATFORM%
set PROJECT_FILE=windows.sln
echo #
echo ####################### Starting %ACTION% process #######################
echo #
echo # Building Full Command is:

echo "%BUILD_TOOL% %PROJECT_FILE% %EXTRASW% %ACTIONSW%%ACTION% %CONFIGSW%%CONFIG% %PLATFORMSW%"
echo #

%BUILD_TOOL% %PROJECT_FILE% %EXTRASW% %ACTIONSW%%ACTION% %CONFIGSW%%CONFIG% %PLATFORMSW%
if ERRORLEVEL 1 exit /b 1
	echo. && echo. && echo.

set peers=peers.conf
if "%ACTION%"=="clean" (
	if exist %CONFIG%\%peers% (
		del %CONFIG%\%peers%
	)
) else (
	rem Creates %CONFIG%\%peers%
	echo {"peers":[]} >%CONFIG%\bin\%peers%
)

rem Custom functions
:end
echo # Exiting script.
endlocal
echo on
exit /b 0
