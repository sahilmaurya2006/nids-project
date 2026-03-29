@echo off
REM Build script for NIDS - Windows
REM Supports both MinGW and MSVC

setlocal enabledelayedexpansion

if "%1"=="" (
    echo.
    echo NIDS Build Script for Windows
    echo =============================
    echo.
    echo Usage: build.bat [mingw^|msvc] [debug^|release]
    echo.
    echo Examples:
    echo   build.bat mingw      (Build with MinGW, release mode)
    echo   build.bat msvc       (Build with MSVC, release mode)
    echo   build.bat mingw debug (Build with MinGW, debug mode)
    echo.
    goto :EOF
)

set COMPILER=%1
set MODE=%2

if "%MODE%"=="" (
    set MODE=release
)

echo.
echo Building NIDS...
echo Compiler: %COMPILER%
echo Mode: %MODE%
echo.

if /i "%COMPILER%"=="mingw" (
    echo Compiling with MinGW...
    make COMPILER=mingw
    if !ERRORLEVEL! EQU 0 (
        echo.
        echo ========================================
        echo Build successful!
        echo Executable: nids.exe
        echo.
        echo To run:
        echo   nids.exe
        echo ========================================
    ) else (
        echo Build failed with MinGW
        exit /b 1
    )
) else if /i "%COMPILER%"=="msvc" (
    echo Compiling with MSVC...
    echo Note: Run this from "Developer Command Prompt for Visual Studio"
    echo.
    make COMPILER=msvc
    if !ERRORLEVEL! EQU 0 (
        echo.
        echo ========================================
        echo Build successful!
        echo Executable: nids.exe
        echo.
        echo To run:
        echo   nids.exe
        echo ========================================
    ) else (
        echo Build failed with MSVC
        exit /b 1
    )
) else (
    echo Invalid compiler: %COMPILER%
    echo Use 'mingw' or 'msvc'
    exit /b 1
)

endlocal
