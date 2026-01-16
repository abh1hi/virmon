@echo off
setlocal enabledelayedexpansion

echo [VirMon] Locating MSBuild...

set "MSBUILD="

:: Check specific BuildTools path found on user system
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe" (
    set "MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe"
    goto Found
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    goto Found
)

:: Fallback: vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "!VSWHERE!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
        set "MSBUILD=%%i"
    )
)

if defined MSBUILD goto Found

echo [Error] MSBuild.exe not found! 
echo Please open this script in "Developer Command Prompt for VS 2022".

exit /b 1

:Found
echo [VirMon] Found MSBuild: "!MSBUILD!"
echo [VirMon] Building Driver (Release/x64)...

"!MSBUILD!" "WindowsSender\VirMonDriver.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Build

if %ERRORLEVEL% NEQ 0 (
    echo [Error] Driver Build Failed.
    
    exit /b 1
)

echo [Success] Driver Built.

