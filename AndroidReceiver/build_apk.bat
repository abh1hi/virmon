@echo off
setlocal

set ANDROID_HOME=C:\Users\abhin\AppData\Local\Android\Sdk

echo Building Android APK...
echo.

REM Find gradle in Android Studio installation
set GRADLE_PATH=

for /d %%i in ("%LOCALAPPDATA%\Android\Sdk\cmdline-tools\*") do (
    if exist "%%i\bin\gradle.bat" (
        set GRADLE_PATH=%%i\bin\gradle.bat
        goto :found
    )
)

REM Try alternative Android Studio location
for /d %%i in ("C:\Program Files\Android\Android Studio\gradle\*") do (
    if exist "%%i\bin\gradle.bat" (
        set GRADLE_PATH=%%i\bin\gradle.bat
        goto :found
    )
)

:found
if "%GRADLE_PATH%"=="" (
    echo ERROR: Gradle not found. Please install Android Studio or Gradle.
    echo Trying to download and use gradlew wrapper...
    
    REM Download gradle wrapper if available
    curl -L https://services.gradle.org/distributions/gradle-8.0-bin.zip -o gradle.zip
    if errorlevel 1 (
        echo Failed to download Gradle wrapper
        pause
        exit /b 1
    )
    
    REM Use PowerShell to extract
    powershell -Command "Expand-Archive -Path gradle.zip -DestinationPath . -Force"
    set GRADLE_PATH=gradle-8.0\bin\gradle.bat
)

echo Using Gradle at: %GRADLE_PATH%
echo.

REM Build the APK
"%GRADLE_PATH%" assembleRelease

if errorlevel 1 (
    echo.
    echo Build failed. Check the error messages above.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful!
echo ========================================
echo.
echo APK location:
dir /b /s app\build\outputs\apk\release\*.apk

pause
