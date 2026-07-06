@echo off
REM MicroMouse Interface Quick Start Script for Windows

echo ============================================
echo MicroMouse Interface - Quick Start
echo ============================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python 3.7+ from python.org
    pause
    exit /b 1
)

echo [1/3] Checking Python installation...
python --version
echo.

echo [2/3] Installing Python dependencies...
python -m pip install -r Scripts\requirements.txt
if %errorlevel% neq 0 (
    echo ERROR: Failed to install dependencies
    pause
    exit /b 1
)
echo.

echo [3/3] Starting MicroMouse Interface...
echo.
echo Opening browser to http://127.0.0.1:5000
echo Press Ctrl+C to stop the server
echo.

cd Scripts
python micromouse_interface.py

pause
