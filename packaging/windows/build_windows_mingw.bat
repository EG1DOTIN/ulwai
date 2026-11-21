@echo off
REM ==============================================================================
REM ULWAI AI Agent - Native Windows MinGW-w64 Build & Packaging Script
REM Builds standalone binaries, bundles Qt DLLs, and creates portable ZIP + Installer.
REM ==============================================================================

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%..\.."
set "BUILD_DIR=%ROOT_DIR%\build-mingw"
set "STAGE_DIR=%ROOT_DIR%\build\windows-stage"
set "DIST_DIR=%ROOT_DIR%\packaging\dist"

if exist "%ROOT_DIR%\VERSION" (
    set /p VERSION=<"%ROOT_DIR%\VERSION"
) else (
    set "VERSION=2.0.0"
)

echo =================================================================
echo  Packaging ULWAI AI Agent v%VERSION% (Windows MinGW-w64)
echo =================================================================

REM 1. Configure and Build
echo [1/4] Configuring and Building with MinGW-w64...
cmake -B "%BUILD_DIR%" -S "%ROOT_DIR%" ^
    -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_GUI=ON ^
    -DBUILD_TESTS=OFF

if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

cmake --build "%BUILD_DIR%" --config Release -j %NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

REM 2. Stage Binaries
echo [2/4] Assembling portable stage folder...
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
mkdir "%STAGE_DIR%"
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"

copy "%BUILD_DIR%\ulw-ai-agent.exe" "%STAGE_DIR%\" >nul
copy "%BUILD_DIR%\ulwai.exe" "%STAGE_DIR%\" >nul
copy "%BUILD_DIR%\ulwai-cli.exe" "%STAGE_DIR%\" >nul
copy "%ROOT_DIR%\README.md" "%STAGE_DIR%\" >nul
copy "%ROOT_DIR%\LICENSE" "%STAGE_DIR%\" >nul

if exist "%ROOT_DIR%\resources" (
    xcopy /e /i /y "%ROOT_DIR%\resources" "%STAGE_DIR%\resources" >nul
)

REM 3. Deploy Qt Dependencies via windeployqt
echo [3/4] Running windeployqt to bundle runtime DLLs...
where windeployqt >nul 2>nul
if %ERRORLEVEL% equ 0 (
    windeployqt --release --no-translations --compiler-runtime "%STAGE_DIR%\ulw-ai-agent.exe"
) else (
    echo [WARNING] windeployqt not found in PATH. Ensure Qt bin directory is in PATH.
)

REM 4. Create Portable ZIP
echo [4/4] Creating portable ZIP archive...
set "ZIP_FILE=%DIST_DIR%\ulwai-v%VERSION%-windows-x64-portable.zip"
if exist "%ZIP_FILE%" del "%ZIP_FILE%"

powershell -Command "Compress-Archive -Path '%STAGE_DIR%\*' -DestinationPath '%ZIP_FILE%'"

echo [SUCCESS] Portable ZIP created: %ZIP_FILE%

REM 5. Optional: Compile NSIS One-Click Installer
where makensis >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo [*] Compiling NSIS Windows Installer...
    makensis "%SCRIPT_DIR%ulwai_installer.nsi"
    echo [SUCCESS] Installer created: %DIST_DIR%\ULWAI-v%VERSION%-windows-setup.exe
) else (
    echo [*] NSIS (makensis) not detected. Skipping setup installer creation.
)

echo =================================================================
echo  Windows MinGW-w64 Packaging Complete!
echo =================================================================
pause
