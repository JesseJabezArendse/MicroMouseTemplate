# MicroMouse Interface - Setup Guide

## Quick Start

### Easiest Way: VS Code Tasks

1. Open the workspace in VS Code
2. Press `Ctrl+Shift+P` and search for "Tasks: Run Task"
3. Select **"Install Python Dependencies"** and wait for completion
4. Run the task **"Start MicroMouse Interface"**
5. Open browser to `http://127.0.0.1:5000`

### Alternative: Python Script

```bash
# Windows Command Prompt
cd Scripts
python start.py
```

### Alternative: Manual Batch File

Windows users can double-click: `Scripts/start.bat`

## Detailed Setup Instructions

### Prerequisites

- **Python 3.7 or higher** - Download from [python.org](https://www.python.org)
  - Make sure to check "Add Python to PATH" during installation

### Step 1: Verify Python Installation

Open Command Prompt/PowerShell and run:
```bash
python --version
```

You should see: `Python 3.x.x`

### Step 2: Install Dependencies

Navigate to the Scripts folder:
```bash
cd Scripts
pip install -r requirements.txt
```

Or use VS Code task: **Install Python Dependencies**

### Step 3: Start the Server

```bash
python micromouse_interface.py
```

You should see output like:
```
Starting MicroMouse Interface Server
Open browser at http://localhost:5000
Running on http://127.0.0.1:5000
Press CTRL+C to quit
```

### Step 4: Open in Browser

Open your web browser and navigate to: `http://127.0.0.1:5000`

## Using the Interface

### Connecting to MicroMouse

1. Plug in your STM32 MicroMouse via USB
2. In the web interface:
   - Click **"Scan Ports"** to find available serial ports
   - Select the correct COM port (usually COM3-COM5)
   - Select baud rate (default: 115200)
   - Click **"Connect"**

### Monitoring Sensors

Once connected, the interface displays:
- **Time-of-Flight Sensors**: Distance measurements in millimeters
- **IMU Data**: Acceleration, rotation, and temperature
- **Battery Status**: Voltage, current, and health percentage
- **ADC Values**: Analog sensor readings
- **Real-time Graphs**: TOF and battery data over time

### Controlling Actuators

- **Motors**: Use sliders to adjust left/right motor speeds (-255 to 255)
- **LEDs**: Toggle individual LEDs with checkboxes
- **Buttons**: Monitor button press status in real-time

## Troubleshooting

### "Python not found"
- Reinstall Python and check "Add Python to PATH"
- Restart your terminal after installation
- Use full path: `C:\Python39\python.exe --version`

### "ModuleNotFoundError: No module named 'flask'"
- Run: `pip install -r Scripts/requirements.txt`
- Or use the VS Code task: **Install Python Dependencies**

### "Address already in use"
- Another application is using port 5000
- Option 1: Close other applications
- Option 2: Change Flask port in `micromouse_interface.py`:
  ```python
  app.run(port=5001)  # Use different port
  ```

### "COM port not detected"
- Check that STM32 driver is installed
- Try different USB cable
- Verify in Device Manager (COM & LPT Ports)
- Click "Scan Ports" button to refresh list

### "No sensor data received"
- Check serial port baud rate (usually 115200)
- Verify STM32 firmware is flashing data correctly
- Check the data transmission format in your firmware
- Open Device Manager to test the port manually

### Browser won't open automatically
- Manually open `http://127.0.0.1:5000` in your browser
- Try different browsers (Chrome, Firefox, Edge)

### Server crashes or errors
- Check the terminal output for error messages
- Verify all dependencies are installed
- Try restarting the server

## VS Code Tasks Reference

| Task | Action |
|------|--------|
| Install Python Dependencies | Installs Flask, PySerial, and other requirements |
| Start MicroMouse Interface | Starts the Flask server |
| Open MicroMouse Interface | Opens http://127.0.0.1:5000 in browser |

**To run a task:**
1. Press `Ctrl+Shift+P`
2. Type "Tasks: Run Task"
3. Select the desired task

## VS Code Debug Configuration

You can also debug the Python application in VS Code:

1. Set breakpoints in `Scripts/micromouse_interface.py`
2. Press `F5` and select **"Debug MicroMouse Interface"**
3. The debugger will start and stop at breakpoints

## Network Access

By default, the interface is only accessible locally. To access from other computers:

Edit `micromouse_interface.py`:
```python
# Change from:
app.run(host='127.0.0.1', port=5000)

# To:
app.run(host='0.0.0.0', port=5000)
```

**Warning**: This exposes the interface on your local network. Only use if behind a firewall.

## Performance Tips

- Use **Chrome or Edge** for best performance
- Keep browser window in focus for faster updates
- Reduce **MAX_DATA_POINTS** in `micromouse_interface.py` if experiencing lag
- Ensure your computer has adequate resources (2GB+ RAM recommended)

## Common Serial Port Speeds

- 9600 - Standard, slower
- 19200 - Common for embedded systems
- 38400 - Moderate speed
- 57600 - High speed
- 115200 - Very high speed (MicroMouse default)

## File Structure

```
Scripts/
├── micromouse_interface.py       Main Flask application
├── requirements.txt               Python dependencies
├── start.py                       Python startup script
├── start.bat                      Windows batch startup
├── README.md                      Feature documentation
├── SETUP.md                       This file
├── templates/
│   └── index.html                Web interface
└── static/
    ├── style.css                 Styling
    └── script.js                 Frontend logic
```

## Next Steps

1. **Configure Serial Port**: Edit `SERIAL_PORT` variable in `micromouse_interface.py`
2. **Adjust Baud Rate**: Change `SERIAL_BAUD` if your firmware uses different rate
3. **Customize Data Parser**: Modify `parse_serial_data()` function to match your protocol
4. **Add Custom Charts**: Edit the chart configuration in `static/script.js`

## Getting Help

- Check the main README.md in Scripts folder
- Review the Flask server logs in terminal
- Check browser console (F12 → Console tab)
- Verify your MicroMouse firmware is outputting data

## Stopping the Server

Press `Ctrl+C` in the command prompt/terminal window running the server.

---

**Happy coding! 🐭🚀**
