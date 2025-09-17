# Arduino Mega Robot Controller - PlatformIO Project

This is a modularized Arduino Mega robot controller designed for PlatformIO. The robot features 4 motors with TB6612 drivers, 4 encoders for odometry, and UART communication with an ESP-01 module.

## Project Structure

```
├── platformio.ini          # PlatformIO configuration
├── include/
│   └── config.h            # Hardware pin definitions and constants
├── src/
│   ├── main.cpp            # Main Arduino program
│   ├── motor_control.cpp   # Motor control implementation
│   ├── encoder.cpp         # Encoder handling and ISRs
│   ├── odometry.cpp        # Odometry calculations and reporting
│   └── command_parser.cpp  # Serial command processing
├── motor_control.h         # Motor control header (will be moved)
├── encoder.h              # Encoder header (will be moved)
├── odometry.h             # Odometry header (will be moved)
└── command_parser.h       # Command parser header (will be moved)
```

## Features

- **Motor Control**: 4-motor differential drive with TB6612 motor drivers
- **Encoders**: 4 quadrature encoders with interrupt-driven counting
- **Odometry**: Real-time position and velocity calculation
- **Communication**: UART-based command interface (115200 baud)
- **Modular Design**: Clean separation of concerns across multiple files

## Hardware Configuration

### Motors (Mixed Drivers)
**Front Motors (TB6612 Driver):**
- Motor 1 (Front Left): PWM=3, IN1=22, IN2=23
- Motor 2 (Front Right): PWM=5, IN1=24, IN2=25 *(reversed in code)*
- Standby Pin: 40

**Rear Motors (L298N Driver):**
- Motor 3 (Rear Left - Motor A): ENA=6, IN1=26, IN2=27 → OUT1/OUT2
- Motor 4 (Rear Right - Motor B): ENB=9, IN3=28, IN4=29 → OUT3/OUT4 *(inverted)*

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
