#!/usr/bin/env python3
"""
MicroMouse Interface - Web-based Serial Monitor and Controller
Provides real-time visualization and control of MicroMouse sensors and actuators
"""

import serial
import threading
import json
import struct
from flask import Flask, render_template, jsonify, request
from flask_cors import CORS
from datetime import datetime
import logging
from collections import deque
import time

# Configuration
SERIAL_PORT = 'COM3'  # Change this to your serial port
SERIAL_BAUD = 1843200
MAX_DATA_POINTS = 100

# Serial packet constants (packet = 3-byte header 'J_A' + 216 bytes payload = 219 total)
PACKET_HEADER = b'J_A'
PACKET_SIZE = 219

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Flask app setup
app = Flask(__name__)
CORS(app)

# Global state
class MicroMouseData:
    def __init__(self):
        self.serial_connection = None
        self.is_connected = False
        self.lock = threading.Lock()
        
        # Sensor data
        self.tof_left = 0
        self.tof_front_left = 0
        self.tof_centre = 0
        self.tof_front_right = 0
        self.tof_right = 0
        self.tof_mb_back = 0
        self.tof_mb_front = 0
        self.tof_mb_front_left = 0
        self.tof_mb_front_right = 0
        self.imu_accel = [0, 0, 0]
        self.imu_gyro = [0, 0, 0]
        self.imu_temp = 0
        self.adc_values = [0, 0, 0, 0, 0]
        self.battery_voltage = 0
        self.battery_current = 0
        self.battery_pct = 0
        
        # Actuator state
        self.motor_left = 0
        self.motor_right = 0
        self.led_status = [False, False, False]
        self.button_status = [False, False]
        
        # History for plotting
        self.history = {
            'timestamps': deque(maxlen=MAX_DATA_POINTS),
            'tof_left': deque(maxlen=MAX_DATA_POINTS),
            'tof_front_left': deque(maxlen=MAX_DATA_POINTS),
            'tof_centre': deque(maxlen=MAX_DATA_POINTS),
            'tof_front_right': deque(maxlen=MAX_DATA_POINTS),
            'tof_right': deque(maxlen=MAX_DATA_POINTS),
            'tof_mb_back': deque(maxlen=MAX_DATA_POINTS),
            'tof_mb_front': deque(maxlen=MAX_DATA_POINTS),
            'tof_mb_front_left': deque(maxlen=MAX_DATA_POINTS),
            'tof_mb_front_right': deque(maxlen=MAX_DATA_POINTS),
            'battery_voltage': deque(maxlen=MAX_DATA_POINTS),
            'imu_accel_x': deque(maxlen=MAX_DATA_POINTS),
            'imu_gyro_z': deque(maxlen=MAX_DATA_POINTS),
        }
        self.last_update = time.time()

data = MicroMouseData()

def connect_serial(port=SERIAL_PORT, baud=SERIAL_BAUD):
    """Connect to the MicroMouse via serial"""
    try:
        data.serial_connection = serial.Serial(port, baud, timeout=1)
        data.is_connected = True
        logger.info(f"Connected to {port} at {baud} baud")
        return True
    except Exception as e:
        logger.error(f"Failed to connect: {e}")
        data.is_connected = False
        return False

def disconnect_serial():
    """Disconnect from serial"""
    if data.serial_connection:
        data.serial_connection.close()
        data.is_connected = False
        logger.info("Disconnected from serial")

def parse_serial_data(raw_data):
    """Parse 171-byte binary packet from MicroMouse (header 'J_A' + payload)"""
    try:
        if len(raw_data) < PACKET_SIZE or raw_data[0:3] != PACKET_HEADER:
            return
        with data.lock:
            # IMU (offsets 11-38, 7 x float32)
            data.imu_accel[0] = struct.unpack('<f', raw_data[11:15])[0]
            data.imu_accel[1] = struct.unpack('<f', raw_data[15:19])[0]
            data.imu_accel[2] = struct.unpack('<f', raw_data[19:23])[0]
            data.imu_gyro[0]  = struct.unpack('<f', raw_data[23:27])[0]
            data.imu_gyro[1]  = struct.unpack('<f', raw_data[27:31])[0]
            data.imu_gyro[2]  = struct.unpack('<f', raw_data[31:35])[0]
            data.imu_temp     = struct.unpack('<f', raw_data[35:39])[0]

            # ToF sensors (offset 39, 9 x 16 bytes: Distance(4), Ambient(4), Signal(4), Status(4))
            data.tof_left          = struct.unpack('<I', raw_data[39:43])[0]
            data.tof_front_left    = struct.unpack('<I', raw_data[55:59])[0]
            data.tof_centre        = struct.unpack('<I', raw_data[71:75])[0]
            data.tof_front_right   = struct.unpack('<I', raw_data[87:91])[0]
            data.tof_right         = struct.unpack('<I', raw_data[103:107])[0]
            data.tof_mb_back       = struct.unpack('<I', raw_data[119:123])[0]
            data.tof_mb_front      = struct.unpack('<I', raw_data[135:139])[0]
            data.tof_mb_front_left = struct.unpack('<I', raw_data[151:155])[0]
            data.tof_mb_front_right= struct.unpack('<I', raw_data[167:171])[0]

            # ADC (offset 183, 5 x uint16 each sent twice, take first copy)
            data.adc_values[0] = struct.unpack('<H', raw_data[183:185])[0]
            data.adc_values[1] = struct.unpack('<H', raw_data[187:189])[0]
            data.adc_values[2] = struct.unpack('<H', raw_data[191:193])[0]
            data.adc_values[3] = struct.unpack('<H', raw_data[195:197])[0]
            data.adc_values[4] = struct.unpack('<H', raw_data[199:201])[0]

            # Battery (offset 203: Vbattery, Vshunt, Current, Power as int16 in mV/mA; batteryLife as int8)
            data.battery_voltage = struct.unpack('<h', raw_data[203:205])[0] / 1000.0
            data.battery_current = struct.unpack('<h', raw_data[207:209])[0] / 1000.0
            data.battery_pct     = struct.unpack('<b', raw_data[211:212])[0]

            # History
            data.history['timestamps'].append(datetime.now().isoformat())
            data.history['tof_left'].append(data.tof_left)
            data.history['tof_front_left'].append(data.tof_front_left)
            data.history['tof_centre'].append(data.tof_centre)
            data.history['tof_front_right'].append(data.tof_front_right)
            data.history['tof_right'].append(data.tof_right)
            data.history['tof_mb_back'].append(data.tof_mb_back)
            data.history['tof_mb_front'].append(data.tof_mb_front)
            data.history['tof_mb_front_left'].append(data.tof_mb_front_left)
            data.history['tof_mb_front_right'].append(data.tof_mb_front_right)
            data.history['battery_voltage'].append(data.battery_voltage)
            data.history['imu_accel_x'].append(data.imu_accel[0])
            data.history['imu_gyro_z'].append(data.imu_gyro[2])
    except Exception as e:
        logger.warning(f"Error parsing data: {e}")

def read_serial_thread():
    """Background thread to read serial data"""
    buffer = b''
    while data.is_connected:
        try:
            if data.serial_connection and data.serial_connection.in_waiting:
                chunk = data.serial_connection.read(data.serial_connection.in_waiting)
                buffer += chunk
                
                while True:
                    idx = buffer.find(PACKET_HEADER)
                    if idx == -1:
                        buffer = buffer[-2:] if len(buffer) >= 2 else buffer
                        break
                    if idx > 0:
                        buffer = buffer[idx:]
                    if len(buffer) < PACKET_SIZE:
                        break
                    parse_serial_data(buffer[:PACKET_SIZE])
                    buffer = buffer[PACKET_SIZE:]
        except Exception as e:
            logger.error(f"Serial read error: {e}")
            data.is_connected = False
            time.sleep(1)

def send_command(command_type, value):
    """Send a command to MicroMouse"""
    if not data.is_connected or not data.serial_connection:
        return False
    
    try:
        # Format: [command_type, value1, value2, ...]
        if command_type == 'motor':
            # value = {'left': speed, 'right': speed}
            cmd = struct.pack('B', 0x01) + struct.pack('h', value['left']) + struct.pack('h', value['right'])
            data.serial_connection.write(cmd)
            with data.lock:
                data.motor_left = value['left']
                data.motor_right = value['right']
            return True
        elif command_type == 'led':
            # value = [led0_state, led1_state, led2_state]
            cmd = struct.pack('B', 0x02) + struct.pack('BBB', *value)
            data.serial_connection.write(cmd)
            with data.lock:
                data.led_status = value
            return True
        return False
    except Exception as e:
        logger.error(f"Command send error: {e}")
        return False

# Flask Routes
@app.route('/')
def index():
    """Serve the main web interface"""
    return render_template('index.html')

@app.route('/api/status')
def get_status():
    """Get current MicroMouse status"""
    with data.lock:
        return jsonify({
            'connected': data.is_connected,
            'timestamp': datetime.now().isoformat(),
            'sensors': {
                'tof': {
                    'left': data.tof_left,
                    'front_left': data.tof_front_left,
                    'centre': data.tof_centre,
                    'front_right': data.tof_front_right,
                    'right': data.tof_right,
                    'mb_back': data.tof_mb_back,
                    'mb_front': data.tof_mb_front,
                    'mb_front_left': data.tof_mb_front_left,
                    'mb_front_right': data.tof_mb_front_right,
                },
                'imu': {
                    'accel': data.imu_accel,
                    'gyro': data.imu_gyro,
                    'temp': data.imu_temp,
                },
                'adc': data.adc_values,
                'battery': {
                    'voltage': data.battery_voltage,
                    'current': data.battery_current,
                    'percentage': data.battery_pct,
                }
            },
            'actuators': {
                'motors': {
                    'left': data.motor_left,
                    'right': data.motor_right,
                },
                'leds': data.led_status,
                'buttons': data.button_status,
            }
        })

@app.route('/api/history')
def get_history():
    """Get historical data for plotting"""
    with data.lock:
        return jsonify({
            'timestamps': list(data.history['timestamps']),
            'tof_left': list(data.history['tof_left']),
            'tof_front_left': list(data.history['tof_front_left']),
            'tof_centre': list(data.history['tof_centre']),
            'tof_front_right': list(data.history['tof_front_right']),
            'tof_right': list(data.history['tof_right']),
            'tof_mb_back': list(data.history['tof_mb_back']),
            'tof_mb_front': list(data.history['tof_mb_front']),
            'tof_mb_front_left': list(data.history['tof_mb_front_left']),
            'tof_mb_front_right': list(data.history['tof_mb_front_right']),
            'battery_voltage': list(data.history['battery_voltage']),
            'imu_accel_x': list(data.history['imu_accel_x']),
            'imu_gyro_z': list(data.history['imu_gyro_z']),
        })

@app.route('/api/connect', methods=['POST'])
def serial_connect():
    """Connect to serial port"""
    payload = request.json
    port = payload.get('port', SERIAL_PORT)
    baud = payload.get('baud', SERIAL_BAUD)
    
    if connect_serial(port, baud):
        # Start serial read thread
        thread = threading.Thread(target=read_serial_thread, daemon=True)
        thread.start()
        return jsonify({'success': True, 'message': f'Connected to {port}'}), 200
    else:
        return jsonify({'success': False, 'message': 'Failed to connect'}), 400

@app.route('/api/disconnect', methods=['POST'])
def serial_disconnect():
    """Disconnect from serial port"""
    disconnect_serial()
    return jsonify({'success': True}), 200

@app.route('/api/motor', methods=['POST'])
def control_motor():
    """Control motor speeds"""
    payload = request.json
    left_speed = payload.get('left', 0)
    right_speed = payload.get('right', 0)
    
    if send_command('motor', {'left': left_speed, 'right': right_speed}):
        return jsonify({'success': True}), 200
    else:
        return jsonify({'success': False}), 400

@app.route('/api/led', methods=['POST'])
def control_led():
    """Control LED states"""
    payload = request.json
    states = payload.get('states', [False, False, False])
    
    if send_command('led', states):
        return jsonify({'success': True}), 200
    else:
        return jsonify({'success': False}), 400

@app.route('/api/ports', methods=['GET'])
def list_ports():
    """List available serial ports"""
    import serial.tools.list_ports
    ports = [{'port': p.device, 'description': p.description} for p in serial.tools.list_ports.comports()]
    return jsonify(ports)

if __name__ == '__main__':
    try:
        logger.info("Starting MicroMouse Interface Server")
        logger.info(f"Open browser at http://localhost:5000")
        app.run(debug=True, host='127.0.0.1', port=5000)
    except KeyboardInterrupt:
        logger.info("Shutting down...")
        disconnect_serial()
