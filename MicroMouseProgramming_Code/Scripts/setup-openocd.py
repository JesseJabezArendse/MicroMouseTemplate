#!/usr/bin/env python3
"""
Setup OpenOCD Installer
Downloads and installs OpenOCD using the system package manager or prebuilt binaries
Platform agnostic (Windows, macOS, Linux)
Updates VS Code settings.json with the OpenOCD path (same as setup-toolchain.py)
"""

import os
import platform
import shutil
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path
import json

def get_repo_root(script_root):
    return Path(script_root).parent

def find_openocd_bin():
    exe = "openocd.exe" if platform.system() == "Windows" else "openocd"
    toolchains_dir = Path(__file__).parent.parent / "Toolchains"
    openocd_bin = toolchains_dir / "openocd" / "bin" / exe
    if openocd_bin.exists():
        return str(openocd_bin)
    return None

def install_openocd():
    system = platform.system()
    toolchains_dir = Path(__file__).parent.parent / "Toolchains"
    openocd_dir = toolchains_dir / "openocd"
    openocd_bin_dir = openocd_dir / "bin"
    openocd_bin_dir.mkdir(parents=True, exist_ok=True)
    if system == "Windows":
        # Download prebuilt OpenOCD for Windows (xPack)
        version = "0.12.0-4"  # Update as needed
        url = f"https://github.com/xpack-dev-tools/openocd-xpack/releases/download/v{version}/xpack-openocd-{version}-win32-x64.zip"
        archive_path = toolchains_dir / f"openocd-{version}-win32-x64.zip"
        print(f"Downloading OpenOCD from {url}")
        urllib.request.urlretrieve(url, archive_path)
        print("Extracting OpenOCD...")
        with zipfile.ZipFile(archive_path, 'r') as zip_ref:
            zip_ref.extractall(openocd_dir)
        # Find the bin directory inside the extracted folder
        for item in openocd_dir.iterdir():
            if item.is_dir() and "xpack-openocd" in item.name:
                extracted_bin = item / "bin"
                # Copy all files to openocd/bin
                for f in extracted_bin.iterdir():
                    shutil.copy(f, openocd_bin_dir)
        archive_path.unlink()
        print(f"OpenOCD installed at {openocd_bin_dir}")
    elif system == "Linux":
        # Try apt or dnf
        if shutil.which("apt"):
            print("Installing OpenOCD using apt...")
            subprocess.run(["sudo", "apt", "install", "-y", "openocd"], check=True)
        elif shutil.which("dnf"):
            print("Installing OpenOCD using dnf...")
            subprocess.run(["sudo", "dnf", "install", "-y", "openocd"], check=True)
        else:
            print("ERROR: No supported package manager found (apt/dnf)")
            sys.exit(1)
    elif system == "Darwin":
        # macOS
        if shutil.which("brew"):
            print("Installing OpenOCD using Homebrew...")
            subprocess.run(["brew", "install", "open-ocd"], check=True)
        else:
            print("ERROR: Homebrew not found. Please install Homebrew.")
            sys.exit(1)
    else:
        print(f"ERROR: Unsupported platform: {system}")
        sys.exit(1)

def update_settings(openocd_path):
    # Update both settings.json files (project and root)
    settings_files = [
        Path(__file__).parent.parent / ".vscode" / "settings.json",
        Path(__file__).parent.parent.parent / ".vscode" / "settings.json"
    ]
    for settings_file in settings_files:
        if settings_file.exists():
            try:
                with open(settings_file, 'r') as f:
                    settings = json.load(f)
                settings["cortex-debug.openocdPath"] = str(openocd_path)
                settings["stm32-for-vscode.openOCDPath"] = str(openocd_path)
                with open(settings_file, 'w') as f:
                    json.dump(settings, f, indent=4)
                print(f"Updated: {settings_file.relative_to(Path(__file__).parent.parent.parent)}")
            except Exception as e:
                print(f"WARNING: Could not update {settings_file}: {e}")

def main():
    print("OpenOCD Installer Script\n========================\n")
    system = platform.system()
    openocd_bin = find_openocd_bin() if system == "Windows" else None
    if system == "Windows":
        if not openocd_bin:
            print("OpenOCD not found in workspace. Installing locally...")
            install_openocd()
            openocd_bin = find_openocd_bin()
            if not openocd_bin:
                print("ERROR: OpenOCD installation failed in workspace directory.")
                sys.exit(1)
        else:
            print(f"OpenOCD already installed at: {openocd_bin}")
    else:
        # For Linux/macOS, keep old logic
        openocd_bin = find_openocd_bin()
        if openocd_bin:
            print(f"OpenOCD already installed at: {openocd_bin}")
        else:
            print("OpenOCD not found. Installing...")
            install_openocd()
            openocd_bin = find_openocd_bin()
            if not openocd_bin:
                print("ERROR: OpenOCD installation failed or not found in PATH.")
                sys.exit(1)
    print(f"\nOpenOCD binary: {openocd_bin}")
    update_settings(openocd_bin)
    print("\nSetup complete! VS Code settings updated.")

if __name__ == "__main__":
    main()
