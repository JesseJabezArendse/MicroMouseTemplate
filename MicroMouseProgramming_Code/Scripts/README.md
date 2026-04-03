# MicroMouse Interface

A web-based serial interface for real-time monitoring and control of the MicroMouse robot.

## Features

- **Real-time Sensor Monitoring**
  - Time-of-Flight (TOF) distance sensors (left, center, right)
  - IMU (Accelerometer, Gyroscope, Temperature)
  - Battery voltage, current, and health percentage
  - ADC analog readings

- **Motor Control**
  - Independent left/right motor speed control (-255 to 255)
  - Real-time feedback

- **LED Control**
  - Toggle individual LEDs with visual indicators
  - Real-time status display

- **Button Monitoring**
  - Display button press states
  - Real-time updates

- **Data Visualization**
  - Live graphs for TOF sensor readings
  - Battery voltage over time
  - Historical data tracking

- **Browser-Based Interface**
  - Responsive design (desktop and mobile)
  - Dark theme optimized for dark environments
  - Real-time updates at 200ms intervals

## Prerequisites

- Python 3.7+
- pip (Python package manager)

## Installation

### Option 1: Using VS Code Tasks (Recommended)

1. Open the workspace in VS Code
2. Run the task: **Install Python Dependencies**
   - Press `Ctrl+Shift+P` → "Tasks: Run Task" → "Install Python Dependencies"
3. Run the task: **Start MicroMouse Interface**
   - Press `Ctrl+Shift+P` → "Tasks: Run Task" → "Start MicroMouse Interface"
4. The interface will be available at `http://127.0.0.1:5000`

### Option 2: Manual Installation

```bash
# Navigate to Scripts folder
cd Scripts

# Install dependencies
pip install -r requirements.txt

# Run the interface
python micromouse_interface.py
```

## Usage

### Web Interface

1. Open your browser to `http://127.0.0.1:5000` (or `localhost:5000`)
2. Select your serial port from the dropdown
3. Choose the appropriate baud rate (default: 115200)
4. Click **Connect**
5. Monitor sensors and control actuators in real-time

### Serial Communication

The interface uses serial protocol with the following format:

**Motor Command (0x01):**
```
[Command: 0x01] [Left Speed: int16] [Right Speed: int16]
```

**LED Command (0x02):**
```
[Command: 0x02] [LED0: uint8] [LED1: uint8] [LED2: uint8]
```

**Data Packet from MicroMouse:**
```
[TOF Left: uint32] [TOF Center: uint32] [TOF Right: uint32] 
[IMU Accel: 3x float32] [IMU Gyro: 3x float32] [IMU Temp: float32]
[Battery V: float32] [Battery I: float32] [Battery %: float32]
[ADC0-4: 5x uint16]
```

## Configuration

Edit `micromouse_interface.py` to change:

```python
SERIAL_PORT = 'COM3'      # Change to your serial port
SERIAL_BAUD = 115200      # Adjust baud rate if needed
MAX_DATA_POINTS = 100     # Number of historical data points
```

## File Structure

```
Scripts/
├── micromouse_interface.py       # Flask server and serial handling
├── requirements.txt              # Python dependencies
├── README.md                     # This file
├── templates/
│   └── index.html               # Web interface HTML
└── static/
    ├── style.css                # Styling
    └── script.js                # Client-side JavaScript
```

## Troubleshooting

### Connection Issues

1. **Serial port not found:**
   - Use the "Scan Ports" button to refresh the port list
   - Verify the correct COM port in Device Manager
   - Ensure STM32 driver is installed

2. **No data received:**
   - Check serial port baud rate matches your MicroMouse firmware
   - Verify the data format from your MicroMouse matches the parser
   - Check serial cable connection

3. **Browser not opening:**
   - Manually open `http://127.0.0.1:5000` in your browser
   - Check if port 5000 is in use by another application

### Python Issues

1. **ModuleNotFoundError:**
   ```bash
   pip install -r Scripts/requirements.txt
   ```

2. **Port already in use:**
   - Change Flask port in `micromouse_interface.py`
   - Or kill the process using that port

## API Endpoints

- `GET /` - Serve web interface
- `GET /api/status` - Get current sensor and actuator status
- `GET /api/history` - Get historical data for plotting
- `GET /api/ports` - List available serial ports
- `POST /api/connect` - Connect to serial port
- `POST /api/disconnect` - Disconnect from serial port
- `POST /api/motor` - Control motor speeds
- `POST /api/led` - Control LED states

## Performance Notes

- Update frequency: 200ms (5 Hz)
- Max historical data points: 100 (adjustable)
- Suitable for baud rates up to 115200

## Future Enhancements

- [ ] Data logging to CSV/JSON
- [ ] Motor PID tuning interface
- [ ] IMU calibration tools
- [ ] Path replay function
- [ ] Maze mapping visualization
- [ ] Network streaming (UDP/TCP)

## License

MIT License

## Support

For issues or questions, refer to the main MicroMouse documentation.
