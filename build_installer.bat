@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo [VirMon] Step 1: Checking CMake...
where cmake >nul 2>nul
if errorlevel 1 goto NoCmake

echo [VirMon] Step 2: Checking ISCC...
where ISCC >nul 2>nul
if errorlevel 1 echo [Warning] Inno Setup Compiler (ISCC) not found.

echo [VirMon] Step 3: Building Service...
if not exist "WindowsSender" goto NoDir

cd WindowsSender
if not exist "build" mkdir "build"
cd build

echo [VirMon] Running CMake...
cmake .. -A x64
if errorlevel 1 goto CmakeFailed

echo [VirMon] Compiling...
cmake --build . --config Release
if errorlevel 1 goto BuildFailed

cd ..\..

echo [VirMon] Step 4: Verification...
if not exist "WindowsSender\x64\Release\VirMonDriver.sys" echo [Warning] VirMonDriver.sys not found. Manual driver build required.

if not exist "WindowsSender\x64\Release\CaptureService.exe" goto MissingExe

echo [Success] CaptureService built successfully.
echo [VirMon] Step 5: Packaging...

where ISCC >nul 2>nul
if errorlevel 1 goto SkipInno

ISCC "WindowsSender\VirMonSetup.iss"
if errorlevel 1 goto InnoFailed

echo [Success] Installer Generated: WindowsSender\Output\VirMonSetup.exe
goto End

:NoCmake
echo [Error] CMake not found. Please install CMake and restart terminal.
goto End

:NoDir
echo [Error] WindowsSender directory missing!
goto End

:CmakeFailed
echo [Error] CMake Generation Failed.
goto End

:BuildFailed
echo [Error] Compilation Failed.
goto End

:MissingExe
echo [Error] CaptureService.exe missing.
goto End

:SkipInno
echo [Warning] Skipping Installer generation (ISCC not found).
goto End

:InnoFailed
echo [Error] Inno Setup Failed.
goto End

:End
pause
