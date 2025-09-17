# 🤖 Arduino Mega Robot Controller - ROS Integration Guide

**Updated for Millis-Based System** - This guide covers ROS integration with the cooperative multitasking Arduino system.

## 📋 System Overview

This update transforms the robot controller for **ROS integration** with high-frequency odometry (100Hz), comprehensive mapping with **4 ultrasonic sensors**, **interrupt-driven sensors**, and **distance-based stepper control**.

### 🎯 Key Features for ROS Integration

- **High-Priority Odometry Task**: 100Hz for precise ROS integration
- **4 Ultrasonic Sensors**: Comprehensive mapping during skidding detection
- **Interrupt-Driven Sensors**: MPU6050 and TCS34725 with INT pins
- **Distance-Based Stepper Control**: Millimeter precision instead of step counts
- **Angle-Based Servo Control**: Direct angle specification for camera and tipper

## 🔌 Updated Pinout (Pure TB6612 Configuration)

```
TB6612FNG #1 (Front motors):     TB6612FNG #2 (Rear motors):
- M1 (Front Left): PWM=3, DIR=22,23    - M3 (Rear Left): PWM=6, DIR=26,27
- M2 (Front Right): PWM=5, DIR=24,25   - M4 (Rear Right): PWM=9, DIR=28,29
- STBY=40 (shared standby for both TB6612 drivers)

Encoders (Hardware Interrupts):
- M1: A=2 (INT0), B=30        - M3: A=19 (INT4), B=32
- M2: A=18 (INT5), B=31       - M4: A=14 (PCINT), B=33

Ultrasonic Sensors (4 sensors for mapping):
- Left: TRIG=A6, ECHO=A4      - Back Left: TRIG=36, ECHO=37
- Right: TRIG=A7, ECHO=A5     - Back Right: TRIG=38, ECHO=39

I2C Sensors (SDA=20, SCL=21):
- MPU6050: INT=A1             - TCS34725: INT=A0, LED=A3

Servos:                       Stepper (ULN2003AN):
- Camera: Signal=42           - IN1=45, IN2=47, IN3=49, IN4=51
- Tipper: Signal=44

ESP8266: TX→16, RX←17 (with voltage divider)
```

## 🏗️ Updated System Architecture

### Task Priorities (Higher = More Important)
```
┌─────────────────────────────────────────────────────────────┐
│ Priority 5 (Highest): Sensor Task (50Hz) + Odometry (100Hz) │
├─────────────────────────────────────────────────────────────┤
│ Priority 4: Motor Control Task (50Hz) - Safety critical     │
├─────────────────────────────────────────────────────────────┤
│ Priority 3: Communication Task (20Hz) - ESP8266 + ROS       │
├─────────────────────────────────────────────────────────────┤
│ Priority 2: Servo/Stepper Task (20Hz) - Actuator control    │
├─────────────────────────────────────────────────────────────┤
│ Priority 1: Autonomous + Watchdog Tasks (10Hz, 2Hz)         │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow for ROS Integration
```
Sensors (20Hz) → updateSensors() → Shared Data → handleCommunication() → ESP8266 → ROS
Encoders (40Hz) → updateOdometry() → Serial → ESP8266 → ROS
Commands: ROS → ESP8266 → handleCommunication() → processCommand() → Motor Control
```

## 🚀 Implementation Steps

### Step 1: Update Pin Definitions
✅ **Already completed** - `config.h` updated with exact pinout

### Step 2: Millis-Based System Architecture
✅ **Already completed** - `millis_config.h` provides:
- Cooperative multitasking with precise timing intervals
- Optimized data structures for sensor data
- 47.5% memory usage (vs 102.5% with FreeRTOS)
- Task scheduling without RTOS overhead

### Step 3: Hardware Setup Required
🔧 **Your Action Needed**:

1. **Wire sensors according to pinout above**
2. **Connect interrupt pins**:
   - MPU6050 INT → A1
   - TCS34725 INT → A0
   - TCS34725 LED control → A3

3. **Verify power supplies**:
   - 5V for sensors, servos, encoders
   - 3.3V for ESP8266
   - 11.1V for motors

### Step 4: Update Sensor Manager
The sensor manager now supports:

#### 4 Ultrasonic Sensors for Mapping
```cpp
// Reading functions for comprehensive mapping
uint16_t readUltrasonicLeft();        // A6/A4
uint16_t readUltrasonicRight();       // A7/A5  
uint16_t readUltrasonicBackLeft();    // 36/37
uint16_t readUltrasonicBackRight();   // 38/39
```

#### Interrupt-Driven Sensors
```cpp
// Setup interrupts for efficient sensor reading
attachInterrupt(digitalPinToInterrupt(MPU6050_INT_PIN), mpuDataReadyISR, RISING);
attachInterrupt(digitalPinToInterrupt(TCS34725_INT_PIN), colorSensorReadyISR, FALLING);
```

#### Command-Controlled Color Sensor
```cpp
// Turn on LED and read color (command: "COLOR:1" to enable)
void enableColorSensorLED(bool enable) {
    digitalWrite(TCS34725_LED_PIN, enable ? HIGH : LOW);
    if (enable) {
        readTCS34725(colorData); // Read and return color
    }
}
```

### Step 5: Enhanced Control Commands

#### Motor Control (Same Format)
```
M:m1,m2,m3,m4,enable
Example: M:100,-50,75,0,1
```

#### Servo Control (Angle-Based)
```
SERVO:id,angle
Examples: 
- SERVO:0,90    # Camera servo to 90 degrees
- SERVO:1,45    # Tipper servo to 45 degrees  
```

#### Stepper Control (Distance-Based)
```
STEPPER:distance_mm,speed
Examples:
- STEPPER:50,100     # Move 50mm forward at 100 RPM
- STEPPER:-25,200    # Move 25mm backward at 200 RPM
- STEPPER:0,150,1    # Continuous rotation at 150 RPM
```

#### Color Sensor Control
```
COLOR:1    # Turn on LED and read color
COLOR:0    # Turn off LED
```

#### Mode Control
```
MODE:manual     # Manual control via ESP8266
MODE:auto       # Autonomous behavior mode
MODE:ros        # ROS-controlled mode
```

## 📊 ROS Integration Features

### High-Frequency Odometry (100Hz)
```cpp
// Odometry task publishes encoder data at 100Hz
void vOdometryTask(void *pvParameters) {
    // 10ms intervals for precise ROS integration
    // Sends: "ODOM:enc1,enc2,enc3,enc4,timestamp"
}
```

### Comprehensive Sensor Data (50Hz)
```cpp
// Sensor task publishes all sensor data
void vSensorTask(void *pvParameters) {
    // Reads: 4 ultrasonics, IMU, color sensor
    // 20ms intervals for mapping and navigation
}
```

### Skidding Detection
The 4 ultrasonic sensors help maintain map accuracy during skidding:
- **Front sensors**: Obstacle detection
- **Rear sensors**: Skidding/slip detection
- **Side sensors**: Wall following and lateral movement

## 🛡️ Safety Features

### Emergency Stop
```cpp
// Immediate motor stop with safety checks
void emergencyStop() {
    robotState.emergencyStop = true;
    disableMotors();
    // All motor commands blocked until reset
}
```

### Sensor Health Monitoring
```cpp
// Continuous sensor health checks
bool checkSensorHealth() {
    // MPU6050 communication check
    // Ultrasonic sensor timeout detection
    // Color sensor response validation
}
```

### Motor Timeout Protection
```cpp
// Motors auto-stop if no commands received for 1 second
if (millis() - robotState.lastCommandTime > 1000) {
    disableMotors();
}
```

## 🔧 Testing and Calibration

### 1. Sensor Calibration
```cpp
// MPU6050 calibration (run once)
calibrateMPU6050();  // Takes 1000 samples for offset calculation
```

### 2. Stepper Distance Calibration
```cpp
// Adjust STEPPER_STEPS_PER_MM in config.h
// Test with known distances and measure actual movement
```

### 3. Ultrasonic Sensor Testing
```cpp
// Test all 4 sensors individually
// Verify no interference between sensors
```

## 📡 Communication Protocol Summary

### From ESP8266/ROS to Arduino:
- `M:m1,m2,m3,m4,enable` - Motor control
- `SERVO:id,angle` - Servo positioning  
- `STEPPER:distance,speed` - Distance-based stepper
- `COLOR:1/0` - Color sensor LED control
- `MODE:manual/auto/ros` - Operation mode
- `STATUS` - Request status update
- `SENSORS` - Request sensor data
- `EMERGENCY` - Emergency stop
- `RESET` - Reset robot state

### From Arduino to ESP8266/ROS:
- `ODOM:enc1,enc2,enc3,enc4,time` - High-frequency odometry
- `SENSORS:ax,ay,az,gx,gy,gz,heading,u1,u2,u3,u4,r,g,b,c,...` - Comprehensive sensor data
- `STATUS:motors,mode,emergency,battery,errors,...` - System status
- `ACK:MOTOR/SERVO/STEPPER` - Command acknowledgments
- `ERROR:description` - Error messages

## 🎮 Usage Examples

### ROS Navigation Stack Integration
```python
# Python/ROS example for receiving high-frequency data
def odometry_callback(data):
    # Process 100Hz encoder data for localization
    
def sensor_callback(data):  
    # Process 50Hz sensor data for mapping
    # 4 ultrasonic + IMU + color data
```

### Manual Control via ESP8266
```javascript
// Web interface can send direct commands
sendCommand("M:150,150,150,150,1");  // Move forward
sendCommand("SERVO:0,45");           // Tilt camera down
sendCommand("STEPPER:100,200");      // Extend conveyor 100mm
```

### Autonomous Mode
```cpp
// Robot uses 4 ultrasonics for obstacle avoidance
// Color detection triggers specific actions
// IMU provides heading information for navigation
```

This updated system provides a robust foundation for ROS integration with high-frequency odometry, comprehensive sensor mapping, and precise actuator control. The interrupt-driven sensors and task prioritization ensure real-time performance suitable for advanced robotics applications.
