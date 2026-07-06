#!/usr/bin/env python3
"""
MicroMouse Interface Quick Start Script
Helps with initial setup and troubleshooting
"""

import os
import sys
import subprocess
import platform
import webbrowser
import time

def run_command(cmd, shell=False):
    """Run a shell command and return success status"""
    try:
        result = subprocess.run(cmd, shell=shell, capture_output=True, text=True)
        return result.returncode == 0, result.stdout, result.stderr
    except Exception as e:
        return False, "", str(e)

def check_python():
    """Check if Python is installed and get version"""
    try:
        result = subprocess.run([sys.executable, '--version'], capture_output=True, text=True)
        version = result.stdout.strip() + result.stderr.strip()
        return True, version
    except:
        return False, ""

def install_dependencies():
    """Install required Python packages"""
    print("[2/4] Installing Python dependencies...")
    success, stdout, stderr = run_command([sys.executable, '-m', 'pip', 'install', '-r', 'requirements.txt'])
    
    if success:
        print("✓ Dependencies installed successfully")
        return True
    else:
        print("✗ Failed to install dependencies")
        print(stderr)
        return False

def start_server():
    """Start the Flask server"""
    print("[3/4] Starting MicroMouse Interface server...")
    print("      Server running on http://127.0.0.1:5000")
    print()
    
    try:
        subprocess.Popen([sys.executable, 'micromouse_interface.py'])
        return True
    except Exception as e:
        print(f"✗ Failed to start server: {e}")
        return False

def open_browser():
    """Open the web interface in default browser"""
    print("[4/4] Opening web interface...")
    time.sleep(2)  # Wait for server to start
    
    try:
        webbrowser.open('http://127.0.0.1:5000')
        print("✓ Browser opened")
        return True
    except:
        print("⚠ Could not open browser automatically")
        print("  Please open http://127.0.0.1:5000 in your browser")
        return False

def main():
    print("=" * 50)
    print("MicroMouse Interface - Quick Start Setup")
    print("=" * 50)
    print()
    
    # Check Python
    print("[1/4] Checking Python installation...")
    has_python, version = check_python()
    if has_python:
        print(f"✓ {version}")
    else:
        print("✗ Python not found or not in PATH")
        print("  Please install Python 3.7+ from python.org")
        sys.exit(1)
    print()
    
    # Install dependencies
    os.chdir(os.path.dirname(__file__))
    if not install_dependencies():
        sys.exit(1)
    print()
    
    # Start server
    if not start_server():
        sys.exit(1)
    print()
    
    # Open browser
    open_browser()
    print()
    print("=" * 50)
    print("Setup complete! Press Ctrl+C to stop the server")
    print("=" * 50)
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n\nShutting down...")
        sys.exit(0)

if __name__ == '__main__':
    main()
