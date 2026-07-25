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

# Try the ST-Link Mass Storage Drive first (legacy copy method)
echo "Searching for ST-Link Mass Storage Drive..."
if [ "$(uname)" == "Darwin" ]; then
    STLINK=$(ls -d /Volumes/*STLINK* /Volumes/NOD* 2>/dev/null | head -n 1)
else
    STLINK=$(ls -d /media/*/*STLINK* /media/*/NOD* /run/media/*/*STLINK* /run/media/*/NOD* 2>/dev/null | head -n 1)
fi

if [ -n "$STLINK" ] && [ -d "$STLINK" ]; then
    echo "ST-Link Mass Storage Drive found at $STLINK. Copying firmware..."
    cat "$BIN_FILE" > "$STLINK/firmware.bin"
    if [ $? -eq 0 ]; then
        echo "Success! Firmware copied to drive. The mouse will reboot momentarily."
        exit 0
    fi
fi

# Fallback: Revert to using the /dev utility method (st-flash tool)
echo "ST-Link drive not found or write failed. Trying st-flash utility..."
if command -v st-flash &> /dev/null; then
    echo "st-flash utility found. Programming via ST-Link programmer..."
    st-flash write "$BIN_FILE" 0x08000000
    if [ $? -eq 0 ]; then
        echo "Success! The mouse will reboot momentarily."
        exit 0
    fi
fi

echo "Error: Could not flash firmware via mass storage or st-flash. Is the mouse plugged in?"
exit 1