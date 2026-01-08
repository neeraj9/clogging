@echo off
REM CMake Build and Test Script for clogging (Windows)
REM This script provides a convenient way to build and test the library

setlocal enabledelayedexpansion

REM Defaults
set "BUILD_DIR=build"
set "INSTALL_PREFIX="
set "BUILD_TYPE=Release"
set "BUILD_TESTS=ON"
set "CMAKE_GENERATOR=Visual Studio 17 2022"

REM Parse arguments
if "%1"=="" goto :build

if "%1"=="--help" (
    goto :print_help
)

if "%1"=="--prefix" (
    set "INSTALL_PREFIX=%2"
    shift
    shift
    goto :parse_args
)

if "%1"=="--build-dir" (
    set "BUILD_DIR=%2"
    shift
    shift
    goto :parse_args
)

if "%1"=="--type" (
    set "BUILD_TYPE=%2"
    shift
    shift
    goto :parse_args
)

if "%1"=="--no-tests" (
    set "BUILD_TESTS=OFF"
    shift
    goto :parse_args
)

if "%1"=="--generator" (
    set "CMAKE_GENERATOR=%2"
    shift
    shift
    goto :parse_args
)

if "%1"=="--clean" (
    if exist %BUILD_DIR% (
        echo Cleaning build directory...
        rmdir /s /q %BUILD_DIR%
    )
    shift
    goto :parse_args
)

if "%1"=="--test-only" (
    if not exist %BUILD_DIR% (
        echo Error: Build directory '%BUILD_DIR%' not found
        echo Please run build first
        exit /b 1
    )
    cd %BUILD_DIR%
    ctest --output-on-failure
    cd ..
    exit /b 0
)

goto :build

:parse_args
if not "%1"=="" goto :parse_args_loop
:parse_args_loop
goto :parse_args

:print_help
echo.
echo Usage: %0 [OPTIONS]
echo.
echo Options:
echo     --help              Show this help message
echo     --prefix PREFIX     Installation prefix
echo     --build-dir DIR     Build directory (default: build)
echo     --type TYPE         Build type: Debug or Release (default: Release)
echo     --no-tests          Disable building tests
echo     --generator GEN     CMake generator (default: Visual Studio 17 2022)
echo     --clean             Clean build directory before configuring
echo     --test-only         Only run tests (skip configuration and build)
echo.
echo Examples:
echo     %0
echo     %0 --prefix "C:\Program Files\clogging" --type Debug
echo     %0 --clean
echo     %0 --test-only
echo.
exit /b 0

:build
REM Check if cmake is available
where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: cmake is not installed or not in PATH
    echo Please install cmake 3.10 or later from https://cmake.org/download/
    exit /b 1
)

echo.
echo === clogging CMake Build Script (Windows) ===
echo.

cmake --version | findstr /r "cmake version"

echo.
echo Creating build directory: %BUILD_DIR%
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Configure
echo.
echo Configuring project...
set "CMAKE_ARGS=-DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_TESTS=%BUILD_TESTS% -G "%CMAKE_GENERATOR%""

if not "!INSTALL_PREFIX!"=="" (
    set "CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_INSTALL_PREFIX=%INSTALL_PREFIX%"
)

cmake .. %CMAKE_ARGS%
if errorlevel 1 (
    echo Configuration failed
    exit /b 1
)

echo Configuration successful

REM Build
echo.
echo Building project...
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
    echo Build failed
    exit /b 1
)

echo Build successful

REM Run tests
if "%BUILD_TESTS%"=="ON" (
    echo.
    echo Running tests...
    ctest --output-on-failure -C %BUILD_TYPE%
    if errorlevel 1 (
        echo Some tests failed
        exit /b 1
    )
    echo All tests passed
)

cd ..

echo.
echo === Build Complete ===
echo.
echo Next steps:
echo   1. Install the library:
if not "!INSTALL_PREFIX!"=="" (
    echo      cmake --install %BUILD_DIR% --prefix %INSTALL_PREFIX%
) else (
    echo      cmake --install %BUILD_DIR%
)
echo   2. Use in your project:
echo      find_package(clogging REQUIRED)
echo      target_link_libraries(myapp PRIVATE clogging::clogging)
echo.
echo For more information, see CMAKE_GUIDE.md or CMAKE_INSTALL_GUIDE.md
echo.

exit /b 0
