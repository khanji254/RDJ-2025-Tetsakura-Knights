# Arduino Mega Robot Controller - Millis-Based System

A comprehensive robot controller for Arduino Mega 2560 with **cooperative multitasking** using millis() timing for optimal memory usage and reliability.

## ✨ Key Features
- **Memory Optimized**: Uses only 47.5% of available RAM (3888/8192 bytes)
- **Cooperative Multitasking**: Efficient millis()-based task scheduling
- **ESP8266 Integration**: Web control via UART communication 
- **4-Motor Control**: Individual wheel control with TB6612/L298N drivers
- **Rich Sensor Suite**: MPU6050 DMP, ultrasonics, color sensor, encoders
- **Real-time Communication**: Serial debugging and ESP8266 command interface

## Project Structure

```
├── platformio.ini          # PlatformIO configuration (millis-based, no FreeRTOS)
├── include/
│   ├── config.h            # Hardware pin definitions and constants
│   ├── millis_config.h     # Timing intervals and data structures
│   ├── motor_control.h     # Motor control declarations
│   ├── encoder.h           # Encoder handling declarations
│   ├── odometry.h          # Odometry calculations declarations
│   ├── command_parser.h    # Serial command processing declarations
│   ├── sensor_manager.h    # Sensor management declarations
│   └── mpu_dmp.h          # MPU6050 DMP integration
├── src/
│   ├── main.cpp            # Millis-based cooperative multitasking main loop
│   ├── motor_control.cpp   # Motor control implementation
│   ├── encoder.cpp         # Encoder handling and ISRs
│   ├── odometry.cpp        # Odometry calculations and reporting
│   ├── command_parser.cpp  # Serial command processing
│   ├── sensor_manager.cpp  # Sensor data collection
│   └── mpu_dmp.cpp        # MPU6050 DMP implementation
```

## System Architecture
- **Cooperative Multitasking**: Uses millis()-based timing for task scheduling
- **Memory Efficient**: 47.5% RAM usage (3888/8192 bytes)  
- **Task Timing**: 
  - Communication: 100Hz (10ms intervals)
  - Motor Control: 50Hz (20ms intervals)
  - Sensor Reading: 20Hz (50ms intervals)
  - Status Updates: 1Hz (1000ms intervals)

## Hardware Configuration

### Motors (Pure TB6612 Configuration)
**All 4 Motors use TB6612 Drivers for Better Performance:**

**TB6612 Driver #1 (Front Motors):**
- Motor 1 (Front Left): PWM=3, IN1=22, IN2=23
- Motor 2 (Front Right): PWM=5, IN1=24, IN2=25 *(reversed in code)*

**TB6612 Driver #2 (Rear Motors):**
- Motor 3 (Rear Left): PWM=6, IN1=26, IN2=27
- Motor 4 (Rear Right): PWM=9, IN1=28, IN2=29 *(inverted)*

**Shared Standby Control:**
- STBY=40 (controls both TB6612 drivers - HIGH=enabled)

**Benefits of Pure TB6612:**
- Higher efficiency (95% vs 70% for L298N)
- Better heat dissipation
- Lower voltage drop
- More precise control

### Encoders
- Encoder 1: A=2 (interrupt), B=30
- Encoder 2: A=18 (interrupt), B=31
- Encoder 3: A=19 (interrupt), B=32
- Encoder 4: A=20 (interrupt), B=33

### Communication
- Debug Serial: USB (115200 baud)
- Radio Serial: UART2 pins 16(TX2)/17(RX2) (115200 baud) - ESP8266 connection

## Commands

The robot accepts the following UART commands:

- `SET_V <left> <right>` - Set left/right wheel velocities
- `MALL <m1> <m2> <m3> <m4>` - Set individual motor speeds
- `M1 <speed>`, `M2 <speed>`, `M3 <speed>`, `M4 <speed>` - Set individual motor
- `FWD [speed]` - Drive forward (default speed: 150)
- `BACK [speed]` - Drive backward (default speed: 150)
- `LEFT [speed]` - Turn left (default speed: 150)
- `RIGHT [speed]` - Turn right (default speed: 150)
- `STOP` - Stop all motors
- `ENABLE` - Enable motor drivers
- `DISABLE` - Disable motor drivers
- `REQ_ODOM` - Request odometry data

## Building and Uploading

1. Install PlatformIO
2. Open this project folder in PlatformIO
3. Connect your Arduino Mega
4. Build and upload:
   ```bash
   pio run --target upload
   ```

## Configuration

Modify `include/config.h` to adjust:
- Pin assignments
- Physical constants (wheel radius, gear ratio, encoder CPR)
- Odometry update interval
- Serial port settings

## Modules

### Motor Control (`motor_control.cpp`)
Handles all motor operations including individual motor control, differential drive patterns, and motor driver enable/disable.

### Encoder (`encoder.cpp`)
Manages quadrature encoder interrupts and provides encoder count management with thread-safe reset functionality.

### Odometry (`odometry.cpp`)
Calculates robot position and velocity from encoder data and transmits odometry packets via UART.

### Command Parser (`command_parser.cpp`)
Processes incoming UART commands and executes corresponding robot actions.

## License

This project is open source. Modify as needed for your specific robot configuration.
