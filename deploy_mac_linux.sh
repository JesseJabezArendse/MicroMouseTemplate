#!/bin/bash
# =========================================================================
# UCT Micromouse - Mac & Linux Standalone Deployer
# =========================================================================

echo "=== UCT Micromouse Mac/Linux Deployer ==="

# Navigate to the directory where this script is located so it can be run from anywhere
cd "$(dirname "$0")" || exit 1

# 1. Compile using Jesse's legacy STM32CubeMX Makefile
echo "[1/3] Compiling C code via Make..."
cd MicroMouseProgramming_Code || { echo "Error: MicroMouseProgramming_Code folder not found!"; exit 1; }

make -j4
if [ $? -ne 0 ]; then
    echo "Build failed! Ensure arm-none-eabi-gcc is installed."
    exit 1
fi

# Find the generated .bin file (CubeMX usually puts it in build/)
BIN_FILE=$(find build -name "*.bin" | head -n 1)
if [ -z "$BIN_FILE" ]; then
    echo "Error: Compiled .bin file not found in build/ directory."
    exit 1
fi

# 2. Flash the binary safely
echo "[2/3] Flashing $BIN_FILE..."
if command -v st-flash &> /dev/null; then
    echo "st-flash utility found. Programming via ST-Link programmer..."
    st-flash write "$BIN_FILE" 0x08000000
    if [ $? -eq 0 ]; then
        echo "Success! The mouse will reboot momentarily."
        exit 0
    else
        echo "Warning: st-flash failed. Falling back to mass storage method..."
    fi
fi

# Fallback: Find the ST-Link Mass Storage Drive
echo "Searching for ST-Link Mass Storage Drive..."
if [ "$(uname)" == "Darwin" ]; then
    STLINK=$(ls -d /Volumes/*STLINK* /Volumes/NOD* 2>/dev/null | head -n 1)
else
    STLINK=$(ls -d /media/*/*STLINK* /media/*/NOD* /run/media/*/*STLINK* /run/media/*/NOD* 2>/dev/null | head -n 1)
fi

if [ -z "$STLINK" ]; then
    echo "Error: ST-Link programmer/drive not found. Is the mouse plugged in?"
    exit 1
fi

# Using 'cat' bypasses the macOS filesystem metadata manager entirely!
cat "$BIN_FILE" > "$STLINK/firmware.bin"
echo "Success! The mouse will reboot momentarily."