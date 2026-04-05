// MicroMouse Interface JavaScript
let isConnected = false;
let updateInterval;

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    setupEventListeners();
    scanSerialPorts();
    startStatusUpdates();
});

function setupEventListeners() {
    // Connection buttons
    document.getElementById('connect-btn').addEventListener('click', connectSerial);
    document.getElementById('disconnect-btn').addEventListener('click', disconnectSerial);
    document.getElementById('scan-ports-btn').addEventListener('click', scanSerialPorts);

    // Motor controls
    document.getElementById('motor-left').addEventListener('input', (e) => {
        document.getElementById('motor-left-val').textContent = e.target.value;
        sendMotorCommand();
    });
    document.getElementById('motor-right').addEventListener('input', (e) => {
        document.getElementById('motor-right-val').textContent = e.target.value;
        sendMotorCommand();
    });

    // LED controls
    ['led-0', 'led-1', 'led-2'].forEach(ledId => {
        document.getElementById(ledId).addEventListener('change', sendLedCommand);
    });
}

function scanSerialPorts() {
    fetch('/api/ports')
        .then(response => response.json())
        .then(ports => {
            const select = document.getElementById('port-select');
            select.innerHTML = '';
            ports.forEach(port => {
                const option = document.createElement('option');
                option.value = port.port;
                option.textContent = `${port.port} - ${port.description}`;
                select.appendChild(option);
            });
        })
        .catch(err => console.error('Error scanning ports:', err));
}

function connectSerial() {
    const port = document.getElementById('port-select').value;
    const baud = parseInt(document.getElementById('baud-select').value);

    fetch('/api/connect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ port, baud })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            isConnected = true;
            updateConnectionStatus(true);
            console.log('Connected successfully');
        } else {
            alert('Connection failed: ' + data.message);
        }
    })
    .catch(err => {
        console.error('Connection error:', err);
        alert('Connection error: ' + err.message);
    });
}

function disconnectSerial() {
    fetch('/api/disconnect', { method: 'POST' })
        .then(() => {
            isConnected = false;
            updateConnectionStatus(false);
        })
        .catch(err => console.error('Disconnect error:', err));
}

function updateConnectionStatus(connected) {
    const indicator = document.getElementById('status-indicator');
    const text = document.getElementById('status-text');
    const connectBtn = document.getElementById('connect-btn');
    const disconnectBtn = document.getElementById('disconnect-btn');

    if (connected) {
        indicator.className = 'status-indicator status-connected';
        text.textContent = 'Connected';
        connectBtn.disabled = true;
        disconnectBtn.disabled = false;
    } else {
        indicator.className = 'status-indicator status-disconnected';
        text.textContent = 'Disconnected';
        connectBtn.disabled = false;
        disconnectBtn.disabled = true;
    }
}

function startStatusUpdates() {
    updateInterval = setInterval(updateStatus, 200);
}

function updateStatus() {
    if (!isConnected) return;

    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            updateSensors(data.sensors);
            updateActuators(data.actuators);
            updateCharts(data);
        })
        .catch(err => console.error('Status update error:', err));
}

function updateSensors(sensors) {
    // TOF sensors
    document.getElementById('tof-left').textContent = sensors.tof.left.toFixed(0);
    document.getElementById('tof-front-left').textContent = sensors.tof.front_left.toFixed(0);
    document.getElementById('tof-centre').textContent = sensors.tof.centre.toFixed(0);
    document.getElementById('tof-front-right').textContent = sensors.tof.front_right.toFixed(0);
    document.getElementById('tof-right').textContent = sensors.tof.right.toFixed(0);
    document.getElementById('tof-back').textContent = sensors.tof.back.toFixed(0);

    // IMU
    document.getElementById('imu-ax').textContent = sensors.imu.accel[0].toFixed(2);
    document.getElementById('imu-ay').textContent = sensors.imu.accel[1].toFixed(2);
    document.getElementById('imu-az').textContent = sensors.imu.accel[2].toFixed(2);
    document.getElementById('imu-gx').textContent = sensors.imu.gyro[0].toFixed(2);
    document.getElementById('imu-gy').textContent = sensors.imu.gyro[1].toFixed(2);
    document.getElementById('imu-gz').textContent = sensors.imu.gyro[2].toFixed(2);
    document.getElementById('imu-temp').textContent = sensors.imu.temp.toFixed(1);

    // Battery
    document.getElementById('batt-voltage').textContent = sensors.battery.voltage.toFixed(2);
    document.getElementById('batt-current').textContent = sensors.battery.current.toFixed(3);
    document.getElementById('batt-pct').textContent = sensors.battery.percentage.toFixed(1);

    // ADC
    sensors.adc.forEach((value, i) => {
        document.getElementById(`adc-${i}`).textContent = value;
    });
}

function updateActuators(actuators) {
    // Motors — only update the encoder readbacks, not the sliders
    // (sliders are user-controlled; writing them back from the server resets user input)
    document.getElementById('enc-rate-left').textContent = actuators.motors.encoder_rate_left;
    document.getElementById('enc-rate-right').textContent = actuators.motors.encoder_rate_right;

    // LEDs
    actuators.leds.forEach((state, i) => {
        const checkbox = document.getElementById(`led-${i}`);
        checkbox.checked = state;
        updateLedIndicator(i, state);
    });

    // Buttons
    actuators.buttons.forEach((state, i) => {
        const btnElement = document.getElementById(`btn-${i}`);
        const stateText = state ? 'Pressed' : 'Released';
        btnElement.textContent = `Button ${i}: ${stateText}`;
        if (state) {
            btnElement.classList.add('pressed');
        } else {
            btnElement.classList.remove('pressed');
        }
    });
}

function updateLedIndicator(index, state) {
    const indicator = document.getElementById(`led-${index}-indicator`);
    if (state) {
        indicator.style.background = '#2ecc71';
        indicator.style.boxShadow = '0 0 12px rgba(46, 204, 113, 0.8)';
    } else {
        indicator.style.background = '#e74c3c';
        indicator.style.boxShadow = '0 0 8px rgba(231, 76, 60, 0.5)';
    }
}

function sendMotorCommand() {
    const left = parseInt(document.getElementById('motor-left').value);
    const right = parseInt(document.getElementById('motor-right').value);

    fetch('/api/motor', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ left, right })
    })
    .catch(err => console.error('Motor command error:', err));
}

function sendLedCommand() {
    const states = [
        document.getElementById('led-0').checked,
        document.getElementById('led-1').checked,
        document.getElementById('led-2').checked
    ];

    fetch('/api/led', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ states })
    })
    .catch(err => console.error('LED command error:', err));
}

let tofChartLayout = {
    title: 'Time-of-Flight Sensor Values',
    plot_bgcolor: '#1a1a1a',
    paper_bgcolor: '#2d2d2d',
    font: { color: '#ecf0f1', family: 'Segoe UI' },
    xaxis: { title: 'Time', zeroline: false },
    yaxis: { title: 'Distance (mm)', zeroline: false }
};

let batteryChartLayout = {
    title: 'Battery Voltage Over Time',
    plot_bgcolor: '#1a1a1a',
    paper_bgcolor: '#2d2d2d',
    font: { color: '#ecf0f1', family: 'Segoe UI' },
    xaxis: { title: 'Time', zeroline: false },
    yaxis: { title: 'Voltage (V)', zeroline: false }
};

let encoderChartLayout = {
    title: 'Motor Encoder Speed (RPM)',
    plot_bgcolor: '#1a1a1a',
    paper_bgcolor: '#2d2d2d',
    font: { color: '#ecf0f1', family: 'Segoe UI' },
    xaxis: { title: 'Time', zeroline: false },
    yaxis: { title: 'Speed (RPM)', zeroline: false }
};

function updateCharts(data) {
    fetch('/api/history')
        .then(response => response.json())
        .then(history => {
            if (history.tof_left && history.tof_left.length > 0) {
                const tofData = [
                    {
                        x: history.timestamps,
                        y: history.tof_left,
                        name: 'Left',
                        mode: 'lines',
                        line: { color: '#e74c3c' }
                    },
                    {
                        x: history.timestamps,
                        y: history.tof_front_left,
                        name: 'Front Left',
                        mode: 'lines',
                        line: { color: '#e67e22' }
                    },
                    {
                        x: history.timestamps,
                        y: history.tof_centre,
                        name: 'Centre',
                        mode: 'lines',
                        line: { color: '#3498db' }
                    },
                    {
                        x: history.timestamps,
                        y: history.tof_front_right,
                        name: 'Front Right',
                        mode: 'lines',
                        line: { color: '#9b59b6' }
                    },
                    {
                        x: history.timestamps,
                        y: history.tof_right,
                        name: 'Right',
                        mode: 'lines',
                        line: { color: '#2ecc71' }
                    },
                    {
                        x: history.timestamps,
                        y: history.tof_mb_back,
                        name: 'Back',
                        mode: 'lines',
                        line: { color: '#95a5a6' }
                    }
                ];
                Plotly.react('tof-chart', tofData, tofChartLayout, { responsive: true });
            }

            if (history.battery_voltage && history.battery_voltage.length > 0) {
                const batteryData = [{
                    x: history.timestamps,
                    y: history.battery_voltage,
                    name: 'Voltage',
                    mode: 'lines',
                    fill: 'tozeroy',
                    line: { color: '#f39c12' }
                }];
                Plotly.react('battery-chart', batteryData, batteryChartLayout, { responsive: true });
            }

            if (history.encoder_rate_left && history.encoder_rate_left.length > 0) {
                const encoderData = [
                    {
                        x: history.timestamps,
                        y: history.encoder_rate_left,
                        name: 'Left RPM',
                        mode: 'lines',
                        line: { color: '#3498db' }
                    },
                    {
                        x: history.timestamps,
                        y: history.encoder_rate_right,
                        name: 'Right RPM',
                        mode: 'lines',
                        line: { color: '#e74c3c' }
                    }
                ];
                Plotly.react('encoder-chart', encoderData, encoderChartLayout, { responsive: true });
            }
        })
        .catch(err => console.error('History update error:', err));
}

// Auto-updater for connection status when page loads
window.addEventListener('load', () => {
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            if (data.connected) {
                isConnected = true;
                updateConnectionStatus(true);
            }
        })
        .catch(() => { /* Server not running yet */ });
});
