# ESP8266-01 Robot Web Controller

This PlatformIO project creates a web-based controller for the Arduino Mega robot. The ESP8266-01 sets up a WiFi Access Point and serves a responsive web interface for remote robot control.

## Hardware Connections

### ESP8266-01 to Arduino Mega
- ESP8266 TX → Arduino Mega Pin 17 (RX2)
- ESP8266 RX → Arduino Mega Pin 16 (TX2)  
- ESP8266 VCC → 3.3V (regulated!)
- ESP8266 GND → GND
- ESP8266 CH_PD → 3.3V (enable pin)

**⚠️ Important:** ESP8266-01 operates at 3.3V. Use a voltage divider or level shifter for RX pin if connecting directly to 5V Arduino pins.

## Features

### 🌐 Web Interface
- **Responsive Design**: Works on phones, tablets, and computers
- **Real-time Control**: Instant robot commands via AJAX
- **Movement Controls**: Forward, backward, left, right, stop
- **Speed Control**: Adjustable speed slider (50-255)
- **Manual Commands**: Direct command input for advanced control
- **Status Monitoring**: Connection status and motor state
- **Odometry Display**: Real-time position and velocity data
- **Keyboard Support**: WASD or arrow keys for movement

### 📡 WiFi Access Point
- **SSID**: RobotController
- **Password**: robot123
- **IP Address**: 192.168.4.1
- **Auto-connects**: No internet required

### 🔗 Robot Communication
- **Serial Protocol**: 115200 baud UART
- **Command Format**: Same as original robot commands
- **Response Handling**: Processes OK/ERR responses
- **Connection Monitoring**: Detects robot disconnection
- **Automatic Reconnection**: Attempts to reconnect lost robots

## Web Interface Commands

The web interface sends these commands to the robot:

### Movement Commands
- `FWD <speed>` - Move forward
- `BACK <speed>` - Move backward  
- `LEFT <speed>` - Turn left
- `RIGHT <speed>` - Turn right
- `STOP` - Emergency stop

### Motor Control
- `ENABLE` - Enable motor drivers
- `DISABLE` - Disable motor drivers
- `MALL <m1> <m2> <m3> <m4>` - Individual motor control
- `SET_V <left> <right>` - Differential drive control

### Status Commands
- `REQ_ODOM` - Request odometry data

## Installation & Usage

### 1. Hardware Setup
Connect ESP8266-01 to Arduino Mega as shown above.

### 2. Program ESP8266
```bash
cd ESP8266_WebController
pio run --target upload
```

### 3. Program Arduino Mega
Ensure the main robot controller is programmed with updated UART pins (Serial2).

### 4. Connect to Robot
1. Power on both ESP8266 and Arduino Mega
2. Connect device to WiFi network "RobotController" 
3. Enter password "robot123"
4. Open browser to http://192.168.4.1
5. Start controlling your robot!

## Configuration

Edit `include/esp_config.h` to modify:
- WiFi credentials
- IP addresses  
- Communication timeouts
- Speed limits
- Web server port

## API Endpoints

The ESP8266 provides these HTTP endpoints:

- `GET /` - Main control interface
- `POST /command` - Send robot command
- `GET /status` - Get robot status (JSON)

### Command API
```javascript
// Send movement command
fetch('/command', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: 'cmd=FWD 150'
})
```

### Status API Response
```json
{
    "connected": true,
    "last_response": 12345,
    "motors_enabled": true, 
    "current_speed": 150,
    "odometry": "ODOM 12345 200 5 -3 4 -2 0.125 -0.087 0.625 -0.435",
    "uptime": 67890
}
```

## Troubleshooting

### ESP8266 Won't Start
- Check 3.3V power supply (adequate current)
- Verify CH_PD pin is HIGH
- Try different baud rates for programming

### Can't Connect to WiFi
- Check SSID and password in esp_config.h
- Reset ESP8266 and try again
- Verify ESP8266 is in AP mode

### Robot Not Responding  
- Check UART connections (TX/RX swapped?)
- Verify baud rate matches (115200)
- Use voltage level shifter if needed
- Check Arduino Mega is programmed with Serial2

### Web Interface Issues
- Clear browser cache
- Try different browser
- Check JavaScript console for errors
- Verify ESP8266 IP address

## Customization

### Adding New Commands
1. Add command handling in `robot_comm.cpp`
2. Add web interface button in `generateControlPage()`
3. Add JavaScript handler in `generateJavaScript()`

### Styling Changes
Modify `generateCSS()` in `web_interface.cpp`

### Network Configuration
Edit WiFi settings in `esp_config.h` and update `setupWiFiAP()` if needed.

## Development

### Building
```bash
pio run
```

### Uploading
```bash
pio run --target upload
```

### Monitoring
```bash
pio device monitor
```

## License
Open source - modify as needed for your robot project.
