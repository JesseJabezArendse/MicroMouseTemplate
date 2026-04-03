#!/usr/bin/env python3
"""
Setup ARM Toolchain Downloader
Downloads latest arm-none-eabi-gcc from xpack and extracts to Toolchains folder
Checks installed version and only downloads if a newer version is available
Platform agnostic (Windows, macOS, Linux)
"""

import json
import os
import platform
import shutil
import sys
import urllib.request
import zipfile
from pathlib import Path


def get_repo_root(script_root):
    """Get the workspace root from the script location"""
    # scripts/setup-toolchain.py -> MicroMouseProgramming_Code (workspace root)
    return Path(script_root).parent


def get_platform_asset_name(version):
    """Get the correct xpack asset name for the current platform"""
    system = platform.system()
    
    if system == "Windows":
        return f"xpack-arm-none-eabi-gcc-{version}-win32-x64.zip"
    elif system == "Darwin":  # macOS
        arch = platform.machine()
        if arch == "arm64":
            return f"xpack-arm-none-eabi-gcc-{version}-darwin-arm64.tar.gz"
        else:
            return f"xpack-arm-none-eabi-gcc-{version}-darwin-x64.tar.gz"
    elif system == "Linux":
        arch = platform.machine()
        if arch == "x86_64":
            return f"xpack-arm-none-eabi-gcc-{version}-linux-x64.tar.gz"
        elif arch == "aarch64":
            return f"xpack-arm-none-eabi-gcc-{version}-linux-arm64.tar.gz"
        else:
            print(f"ERROR: Unsupported Linux architecture: {arch}")
            sys.exit(1)
    else:
        print(f"ERROR: Unsupported platform: {system}")
        sys.exit(1)


def fetch_latest_release():
    """Fetch latest release info from GitHub"""
    api_url = "https://api.github.com/repos/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/latest"
    headers = {"Accept": "application/vnd.github.v3+json"}
    
    try:
        req = urllib.request.Request(api_url, headers=headers)
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode())
            return data
    except Exception as e:
        print(f"ERROR: Failed to fetch release info: {e}")
        sys.exit(1)


def download_file(url, destination):
    """Download a file with progress indication"""
    try:
        def download_progress(block_num, block_size, total_size):
            downloaded = block_num * block_size
            if total_size > 0:
                percent = min(100, (downloaded * 100) // total_size)
                print(f"  Downloaded: {percent}%", end="\r")
        
        urllib.request.urlretrieve(url, destination, reporthook=download_progress)
        print("  Downloaded: 100% ")
    except Exception as e:
        print(f"ERROR: Failed to download file: {e}")
        sys.exit(1)


def extract_archive(archive_path, extract_to):
    """Extract tar.gz or zip archive"""
    try:
        if archive_path.endswith('.zip'):
            with zipfile.ZipFile(archive_path, 'r') as zip_ref:
                zip_ref.extractall(extract_to)
        elif archive_path.endswith('.tar.gz') or archive_path.endswith('.tgz'):
            import tarfile
            with tarfile.open(archive_path, 'r:gz') as tar_ref:
                tar_ref.extractall(extract_to)
        else:
            print(f"ERROR: Unknown archive format: {archive_path}")
            sys.exit(1)
    except Exception as e:
        print(f"ERROR: Failed to extract archive: {e}")
        sys.exit(1)


def main():
    script_root = Path(__file__).parent
    repo_root = get_repo_root(script_root)
    toolchains_dir = repo_root / "Toolchains"

    print("ARM Toolchain Setup Script")
    print("==========================\n")

    # Create toolchains directory
    toolchains_dir.mkdir(parents=True, exist_ok=True)

    # Fetch latest release
    print("Fetching latest release information from GitHub...")
    release = fetch_latest_release()
    latest_version = release["tag_name"].lstrip("v")
    print(f"Latest version available: {latest_version}")

    # Check if correct version is already installed
    for item in toolchains_dir.iterdir():
        if item.is_dir() and "xpack-arm-none-eabi-gcc" in item.name:
            version_file = item / ".version"
            if version_file.exists():
                installed_version = version_file.read_text().strip()
                if installed_version == latest_version:
                    print(f"✓ Toolchain version {installed_version} is already up to date.")
                    print(f"\nToolchain path:")
                    print(f"{item / 'bin'}")
                    return 0

    print(f"Installing version {latest_version}...")

    # Get platform-specific asset name
    asset_name = get_platform_asset_name(latest_version)
    
    # Find the asset
    asset = None
    for a in release.get("assets", []):
        if a["name"] == asset_name:
            asset = a
            break

    if not asset:
        print(f"ERROR: Could not find {asset_name} in release assets")
        print("Available assets:")
        for a in release.get("assets", []):
            print(f"  - {a['name']}")
        sys.exit(1)

    # Download
    download_url = asset["browser_download_url"]
    archive_path = toolchains_dir / asset_name

    print(f"\nDownloading from: {download_url}")
    print(f"Saving to: {archive_path}\n")
    download_file(download_url, str(archive_path))

    print("Download complete. Extracting archive...")
    extract_archive(str(archive_path), str(toolchains_dir))

    # Find extracted folder
    extracted_folder = None
    for item in toolchains_dir.iterdir():
        if item.is_dir() and "xpack-arm-none-eabi-gcc" in item.name:
            extracted_folder = item
            break

    if not extracted_folder:
        print("ERROR: Could not find extracted folder")
        sys.exit(1)

    # Write version file to mark installation
    version_marker_file = extracted_folder / ".version"
    version_marker_file.write_text(latest_version)

    bin_path = extracted_folder / "bin"
    print(f"Successfully extracted to: {extracted_folder}")
    print(f"Binaries available at: {bin_path}\n")

    # Update VS Code settings with the toolchain path
    import json
    settings_files = [
        Path(__file__).parent.parent / ".vscode" / "settings.json",  # MicroMouseProgramming_Code
        Path(__file__).parent.parent.parent / ".vscode" / "settings.json"  # Root workspace
    ]
    
    for settings_file in settings_files:
        if settings_file.exists():
            try:
                with open(settings_file, 'r') as f:
                    settings = json.load(f)
                
                # Update toolchain paths
                settings["cortex-debug.armToolchainPath"] = str(bin_path)
                if "stm32-for-vscode.armToolchainPath" not in settings:
                    settings["stm32-for-vscode.armToolchainPath"] = str(bin_path)
                else:
                    settings["stm32-for-vscode.armToolchainPath"] = str(bin_path)
                
                with open(settings_file, 'w') as f:
                    json.dump(settings, f, indent=4)
                
                print(f"Updated: {settings_file.relative_to(Path(__file__).parent.parent.parent)}")
            except Exception as e:
                print(f"WARNING: Could not update {settings_file}: {e}")

    # Remove archive
    archive_path.unlink()
    print("Cleaned up archive file.")

    print("\n==========================\n")
    print("Setup complete! Toolchain installed at:")
    print(f"{bin_path}")
    print(f"\nVersion: {latest_version}")
    print(f"\nVS Code settings have been automatically updated.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
