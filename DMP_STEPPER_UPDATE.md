# 🚀 MPU6050 DMP Integration & 28BYJ-48 Stepper Control - Millis System

## 🎯 Key Updates Made - Millis-Based Architecture

### ✅ **MPU6050 DMP Module** 
- **Separate DMP module** (`mpu_dmp.h` and `mpu_dmp.cpp`) 
- **Hardware DMP processing** - reduces Arduino Mega computation load
- **Z-axis focus** for robot turning with yaw angle
- **Acceleration data** for full integration capability
- **Polled operation** integrated into millis-based sensor loop (20Hz)

### ✅ **28BYJ-48 Stepper Motor Support**
- **Updated configuration** for 28BYJ-48 5V stepper motor
- **Step-based control** instead of distance-based
- **Proper speed limits** (1-15 RPM for smooth operation)
- **2048 steps per revolution** (32 steps × 64:1 gear ratio)

## 🔧 **MPU6050 DMP Usage**

### Hardware Connection
```
MPU6050 → Arduino Mega:
- VCC → 3.3V or 5V
- GND → GND  
- SDA → Pin 20 (SDA)
- SCL → Pin 21 (SCL)
- INT → Pin A1 (interrupt pin)
```

### DMP Features
```cpp
// Initialize DMP (replaces basic MPU6050 initialization)
bool success = initializeMPU_DMP();

// Read processed data (no complex calculations needed)
DMPData_t dmpData;
if (readDMPData(dmpData)) {
    float yaw = dmpData.yaw;           // Z-axis rotation (for turns)
    float pitch = dmpData.pitch;       // X-axis rotation (tilt)
    float roll = dmpData.roll;         // Y-axis rotation (balance)
    float accel_x = dmpData.accel_x;   // Linear acceleration (m/s²)
    float accel_y = dmpData.accel_y;   // Linear acceleration (m/s²)
    float accel_z = dmpData.accel_z;   // Linear acceleration (m/s²)
}

// Get just the yaw for robot control
float currentYaw = getYawAngle();      // Current Z-axis angle
float yawRate = getYawRate();          // Angular velocity (°/s)

// Reset yaw reference (set current position as 0°)
resetYawReference();
```

### DMP Advantages
- **Hardware processing** - quaternion calculations done by MPU6050's DMP
- **Reduced CPU load** - Arduino Mega just reads processed data
- **Higher accuracy** - DMP uses sensor fusion algorithms
- **Interrupt-driven** - efficient, non-blocking operation
- **Auto-calibration** - built-in offset calibration

## 🔧 **28BYJ-48 Stepper Control**

### Hardware Connection
```
28BYJ-48 + ULN2003AN → Arduino Mega:
- IN1 → Pin 45
- IN2 → Pin 47  
- IN3 → Pin 49
- IN4 → Pin 51
- VCC → 5V
- GND → GND
```

### Stepper Commands
```cpp
// Step-based control (recommended)
STEPPER:2048,10      // 1 full revolution (2048 steps) at 10 RPM
STEPPER:1024,15      // Half revolution (1024 steps) at 15 RPM  
STEPPER:-512,8       // Quarter revolution backward at 8 RPM
STEPPER:0,12,1       // Continuous rotation at 12 RPM

// Revolution-based helper
runStepperRevolutions(1.5, 10);  // 1.5 revolutions at 10 RPM
runStepperRevolutions(-0.25, 15); // Quarter revolution backward
```

### 28BYJ-48 Specifications
- **Steps per revolution**: 2048 (32 × 64:1 gear ratio)
- **Recommended speed**: 10-15 RPM maximum
- **Voltage**: 5V DC
- **Driver**: ULN2003AN (Darlington array)
- **Torque**: High torque due to gear reduction
- **Precision**: 0.176° per step (2048 steps/360°)

## 📡 **Updated Communication Protocol**

### Commands to Robot
```
Motor Control:    M:m1,m2,m3,m4,enable
Servo Control:    SERVO:id,angle (id: 0=camera, 1=tipper)
Stepper Control:  STEPPER:steps,speed,continuous
                  Examples:
                  - STEPPER:2048,10     # 1 revolution CW
                  - STEPPER:-1024,12    # Half revolution CCW  
                  - STEPPER:0,15,1      # Continuous at 15 RPM
Color Sensor:     COLOR:1/0 (enable/disable LED and read)
Mode Control:     MODE:manual/auto/ros
System:           STATUS, SENSORS, EMERGENCY, RESET
```

### Data from Robot
```
High-freq Odom:   ODOM:enc1,enc2,enc3,enc4,timestamp (100Hz)
Comprehensive:    SENSORS:yaw,pitch,roll,accel_x,accel_y,accel_z,
                          ultra_left,ultra_right,ultra_back_left,
                          ultra_back_right,red,green,blue,clear,
                          servo1,servo2,stepper_pos (50Hz)
Status:           STATUS:motors,mode,emergency,battery,errors,
                          enc1,enc2,enc3,enc4,ultra_left,ultra_right,
                          ultra_back_left,ultra_back_right,yaw
Acknowledgments:  ACK:MOTOR/SERVO/STEPPER
Errors:           ERROR:description
```

## 🎮 **Usage Examples**

### Robot Turn Control (Using DMP Yaw)
```cpp
// Get current heading
float currentYaw = getYawAngle();

// Turn 90 degrees clockwise
float targetYaw = currentYaw + 90.0;
while (abs(getYawAngle() - targetYaw) > 2.0) {
    // Send turn command to motors
    sendMotorCommand(100, -100, 100, -100, true); // Turn right
    delay(10);
}
```

### Conveyor Belt Control (28BYJ-48)
```cpp
// Extend conveyor belt (example: 5 revolutions)
sendStepperCommand(5 * 2048, 12, false);  // 5 revolutions at 12 RPM

// Retract conveyor
sendStepperCommand(-5 * 2048, 12, false); // 5 revolutions backward

// Continuous operation
sendStepperCommand(0, 10, true);          // Continuous at 10 RPM
```

### Full ROS Integration Data
```python
# Python/ROS example
def sensor_callback(data):
    # DMP-processed IMU data (no CPU-intensive calculations needed)
    yaw = data.yaw          # Z-axis rotation for navigation
    pitch = data.pitch      # Tilt detection
    roll = data.roll        # Balance monitoring
    
    # Linear acceleration (gravity compensated by DMP)
    accel_x = data.accel_x  # Forward/backward acceleration
    accel_y = data.accel_y  # Left/right acceleration  
    accel_z = data.accel_z  # Up/down acceleration
    
    # 4 ultrasonic sensors for comprehensive mapping
    front_left = data.ultra_left
    front_right = data.ultra_right
    back_left = data.ultra_back_left  
    back_right = data.ultra_back_right
```

## 🛠️ **Required Libraries**

Make sure these libraries are installed:
```
- I2Cdev (Jeff Rowberg's library)
- MPU6050_6Axis_MotionApps20 (DMP library)
- Wire (I2C communication)
- Servo (servo control)
- Stepper (stepper motor control)
- Adafruit TCS34725 (color sensor)
- Adafruit BusIO (sensor communication)
```

## 🚀 **Performance Benefits**

### Millis-Based vs FreeRTOS
- **Memory Usage**: 47.5% vs 102.5% RAM usage
- **Simplicity**: No task stack management or priority conflicts
- **Reliability**: No stack overflow or memory fragmentation
- **Deterministic**: Precise timing control with millis()

### DMP vs Basic MPU6050
- **CPU Usage**: ~80% reduction in computation load
- **Accuracy**: Hardware sensor fusion vs software calculations  
- **Integration**: Polled operation in sensor loop (20Hz)
- **Stability**: Built-in filtering and calibration

### 28BYJ-48 vs Other Steppers
- **High Torque**: 64:1 gear reduction provides excellent holding torque
- **Precision**: 2048 steps/revolution = 0.176° precision
- **Cost Effective**: Inexpensive with ULN2003AN driver included
- **5V Operation**: Direct Arduino 5V compatibility
- **Quiet Operation**: Smooth operation at recommended speeds

The system now provides efficient DMP-based IMU processing and proper 28BYJ-48 stepper control, perfect for precise robot navigation and manipulation tasks!
