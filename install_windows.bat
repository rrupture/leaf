@echo off
:: ──────────────────────────────────────────────────
:: Leaf Build + Install Script (Windows)
:: Run this from your leaf/ project root folder
:: ──────────────────────────────────────────────────

echo.
echo   Building Leaf...
echo.

:: Compile
g++ -std=c++17 -Wall -Wextra -O2 ^
    src/main.cpp ^
    src/lexer.cpp ^
    -o leaf.exe

if %errorlevel% neq 0 (
    echo.
    echo   BUILD FAILED. Make sure g++ is installed.
    echo   Install MinGW from: https://winlibs.com
    pause
    exit /b 1
)

echo   Build successful: leaf.exe
echo.

:: Install: copy leaf.exe to C:\leaf\ and add to PATH
set INSTALL_DIR=C:\leaf

if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
)

copy /Y leaf.exe "%INSTALL_DIR%\leaf.exe" > nul
echo   Installed to: %INSTALL_DIR%\leaf.exe
echo.

:: Check if already in PATH
echo %PATH% | find /I "%INSTALL_DIR%" > nul
if %errorlevel% equ 0 (
    echo   PATH already contains %INSTALL_DIR% — you're good!
) else (
    echo   Adding %INSTALL_DIR% to your system PATH...
    setx PATH "%PATH%;%INSTALL_DIR%" /M > nul 2>&1
    if %errorlevel% neq 0 (
        echo.
        echo   Could not auto-add to PATH (needs admin).
        echo   Right-click this script and choose "Run as administrator"
        echo   OR manually add %INSTALL_DIR% to your PATH in System Settings.
    ) else (
        echo   PATH updated!
    )
)

echo.
echo   Done! Open a NEW terminal window and try:
echo     leaf -help
echo     leaf -create hello.leaf
echo     leaf -run hello.leaf
echo.
pause
