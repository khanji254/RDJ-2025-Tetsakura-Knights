#include "sensor_manager.h"
#include "config.h"
#include "mpu_dmp.h"
#include "motor_control.h"
#include <EEPROM.h>
#include <Adafruit_TCS34725.h>

// ================ GLOBAL SENSOR OBJECTS ================
Servo cameraServo;
Servo tipperServo;
Stepper stepperMotor(STEPPER_STEPS_PER_REV, STEPPER_IN1, STEPPER_IN3, STEPPER_IN2, STEPPER_IN4);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

// Calibration data
MPU6050_Calibration_t mpu_calibration = {0, 0, 0, 0, 0, 0};

// Local variables
float yaw_angle = 0.0;
unsigned long last_mpu_time = 0;

void readMPU_DMP(float &yaw, float &pitch, float &roll, Vector3_t &linearAccel) {
    // Use the proper DMP implementation
    if (dmpReady && isDMPDataReady()) {
        DMPData_t dmpData;
        
        if (readDMPData(dmpData)) {
            // Extract yaw, pitch, roll from DMP
            yaw = dmpData.yaw;
            pitch = dmpData.pitch;
            roll = dmpData.roll;
            
            // Extract gravity-compensated linear acceleration
            linearAccel.x = dmpData.accel_x;
            linearAccel.y = dmpData.accel_y;
            linearAccel.z = dmpData.accel_z;
            
            last_mpu_time = dmpData.timestamp;
        } else {
            // DMP read failed - use last known values or zeros
            yaw = pitch = roll = 0.0;
            linearAccel.x = linearAccel.y = linearAccel.z = 0.0;
        }
    } else {
        // DMP not ready or no data available
        yaw = pitch = roll = 0.0;
        linearAccel.x = linearAccel.y = linearAccel.z = 0.0;
    }
}

// ================ INITIALIZATION FUNCTIONS ================

void initializeSensors() {
    DEBUG_SERIAL.println("Initializing all sensors...");
    
    // Initialize I2C
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C speed
    
    initializeMPU6050();
    initializeUltrasonicSensors();
    initializeTCS34725();
    initializeServos();
    initializeStepper();
    
    DEBUG_SERIAL.println("All sensors initialized successfully!");
}

void initializeMPU6050() {
    DEBUG_SERIAL.print("Initializing MPU6050 with DMP...");
    
    unsigned long startTime = millis();
    const unsigned long INIT_TIMEOUT = 5000; // 5 second timeout
    
    // Initialize DMP module with timeout protection
    bool dmpSuccess = false;
    
    // Try DMP initialization with timeout
    if (millis() - startTime < INIT_TIMEOUT) {
        dmpSuccess = initializeMPU_DMP();
    }
    
    if (dmpSuccess && (millis() - startTime < INIT_TIMEOUT)) {
        DEBUG_SERIAL.println(" DMP OK");
        
        // Setup DMP interrupt for efficient data reading
        setupDMPInterrupt();
        
        // Skip calibration during startup to prevent hanging
        DEBUG_SERIAL.println("MPU6050 initialized - calibration available via INIT command");
        
    } else {
        if (millis() - startTime >= INIT_TIMEOUT) {
            DEBUG_SERIAL.println(" DMP Timeout - continuing without DMP");
        } else {
            DEBUG_SERIAL.println(" DMP Failed - check wiring and connections");
        }
        
        // DMP initialization failed - this is not critical, robot can still operate
        DEBUG_SERIAL.println("Robot will operate without DMP - sensor readings will be basic");
    }
    
    last_mpu_time = millis();
    DEBUG_SERIAL.print("MPU6050 init completed in ");
    DEBUG_SERIAL.print(millis() - startTime);
    DEBUG_SERIAL.println("ms");
}

void initializeUltrasonicSensors() {
    DEBUG_SERIAL.println("Initializing ultrasonic sensors...");
    
    pinMode(ULTRASONIC1_TRIG, OUTPUT);
    pinMode(ULTRASONIC1_ECHO, INPUT);
    pinMode(ULTRASONIC2_TRIG, OUTPUT);
    pinMode(ULTRASONIC2_ECHO, INPUT);
    
    digitalWrite(ULTRASONIC1_TRIG, LOW);
    digitalWrite(ULTRASONIC2_TRIG, LOW);
    
    DEBUG_SERIAL.println("Ultrasonic sensors ready");
}

void initializeTCS34725() {
    DEBUG_SERIAL.print("Initializing TCS34725 RGB sensor...");
    
    unsigned long startTime = millis();
    const unsigned long TCS_TIMEOUT = 2000; // 2 second timeout
    
    bool tcsFound = false;
    
    // Try TCS34725 initialization with timeout
    if (millis() - startTime < TCS_TIMEOUT) {
        tcsFound = tcs.begin();
    }
    
    if (tcsFound && (millis() - startTime < TCS_TIMEOUT)) {
        DEBUG_SERIAL.println(" Found TCS34725!");
        
        // Setup LED pin if used
        if (TCS34725_LED_PIN > 0) {
            pinMode(TCS34725_LED_PIN, OUTPUT);
            digitalWrite(TCS34725_LED_PIN, HIGH); // Turn on LED
        }
    } else {
        if (millis() - startTime >= TCS_TIMEOUT) {
            DEBUG_SERIAL.println(" TCS34725 timeout - continuing without color sensor");
        } else {
            DEBUG_SERIAL.println(" TCS34725 not found - continuing without color sensor");
        }
    }
    
    DEBUG_SERIAL.print("TCS34725 init completed in ");
    DEBUG_SERIAL.print(millis() - startTime);
    DEBUG_SERIAL.println("ms");
}

void initializeServos() {
    DEBUG_SERIAL.println("Initializing servos...");
    
    cameraServo.attach(CAMERA_SERVO_PIN);
    tipperServo.attach(TIPPER_SERVO_PIN);
    
    // Set to neutral positions
    cameraServo.write(90);
    tipperServo.write(0);
    
    delay(500); // Allow servos to reach position
    
    DEBUG_SERIAL.println("Servos initialized");
}

void initializeStepper() {
    DEBUG_SERIAL.println("Initializing stepper motor...");
    
    stepperMotor.setSpeed(10); // 10 RPM default
    
    DEBUG_SERIAL.println("Stepper motor ready");
}

// ================ CALIBRATION FUNCTIONS ================

void calibrateMPU6050() {
    DEBUG_SERIAL.println("Calibrating MPU6050... Keep robot still!");
    
    long gyro_x_sum = 0, gyro_y_sum = 0, gyro_z_sum = 0;
    long accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
    
    for (int i = 0; i < MPU6050_CALIBRATION_SAMPLES; i++) {
        int16_t ax, ay, az, gx, gy, gz;
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        
        gyro_x_sum += gx;
        gyro_y_sum += gy;
        gyro_z_sum += gz;
        accel_x_sum += ax;
        accel_y_sum += ay;
        accel_z_sum += az;
        
        delay(2);
        
        if (i % 100 == 0) {
            DEBUG_SERIAL.print(".");
        }
    }
    
    mpu_calibration.gyro_x_offset = (float)gyro_x_sum / MPU6050_CALIBRATION_SAMPLES;
    mpu_calibration.gyro_y_offset = (float)gyro_y_sum / MPU6050_CALIBRATION_SAMPLES;
    mpu_calibration.gyro_z_offset = (float)gyro_z_sum / MPU6050_CALIBRATION_SAMPLES;
    mpu_calibration.accel_x_offset = (float)accel_x_sum / MPU6050_CALIBRATION_SAMPLES;
    mpu_calibration.accel_y_offset = (float)accel_y_sum / MPU6050_CALIBRATION_SAMPLES;
    mpu_calibration.accel_z_offset = ((float)accel_z_sum / MPU6050_CALIBRATION_SAMPLES) - 16384; // Remove 1g
    
    DEBUG_SERIAL.println("\nMPU6050 calibration complete!");
    saveMPU6050Calibration();
}

void saveMPU6050Calibration() {
    // Save calibration data to EEPROM
    int addr = 0;
    EEPROM.put(addr, mpu_calibration);
    DEBUG_SERIAL.println("MPU6050 calibration saved to EEPROM");
}

void loadMPU6050Calibration() {
    // Load calibration data from EEPROM
    int addr = 0;
    MPU6050_Calibration_t loaded_cal;
    EEPROM.get(addr, loaded_cal);
    
    // Check if calibration data is valid (simple validation)
    if (abs(loaded_cal.gyro_x_offset) < 1000 && abs(loaded_cal.gyro_y_offset) < 1000) {
        mpu_calibration = loaded_cal;
        DEBUG_SERIAL.println("MPU6050 calibration loaded from EEPROM");
    } else {
        DEBUG_SERIAL.println("Invalid calibration data, performing fresh calibration...");
        calibrateMPU6050();
    }
}

// ================ SENSOR READING FUNCTIONS ================

SensorData_t readAllSensors() {
    SensorData_t data;
    
    // Read MPU6050 with DMP
    readMPU_DMP(data.yaw, data.pitch, data.roll, data.linearAccel);
    
    // Read all 4 ultrasonic sensors
    data.ultrasonicLeftDistance = readUltrasonicLeft();
    data.ultrasonicRightDistance = readUltrasonicRight();
    data.ultrasonicBackLeftDistance = readUltrasonicBackLeft();
    data.ultrasonicBackRightDistance = readUltrasonicBackRight();
    
    // Read RGB sensor
    readTCS34725(data.colorSensor);
    
    // Read servo positions
    data.cameraServoPosition = cameraServo.read();
    data.tipperServoPosition = tipperServo.read();
    
    data.timestamp = millis();
    data.dataValidFlags = 0xFF; // All data valid
    
    return data;
}

float readUltrasonic(uint8_t trigPin, uint8_t echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    unsigned long duration = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT);
    
    if (duration == 0) {
        return ULTRASONIC_MAX_DISTANCE; // Timeout - return max distance
    }
    
    float distance = (duration * 0.034) / 2; // Convert to cm
    
    return constrain(distance, 2, ULTRASONIC_MAX_DISTANCE);
}

// Individual ultrasonic sensor reading functions
uint16_t readUltrasonicLeft() {
    return (uint16_t)readUltrasonicGeneric(ULTRASONIC_LEFT_TRIG, ULTRASONIC_LEFT_ECHO);
}

uint16_t readUltrasonicRight() {
    return (uint16_t)readUltrasonicGeneric(ULTRASONIC_RIGHT_TRIG, ULTRASONIC_RIGHT_ECHO);
}

uint16_t readUltrasonicBackLeft() {
    return (uint16_t)readUltrasonicGeneric(ULTRASONIC_BACK_LEFT_TRIG, ULTRASONIC_BACK_LEFT_ECHO);
}

uint16_t readUltrasonicBackRight() {
    return (uint16_t)readUltrasonicGeneric(ULTRASONIC_BACK_RIGHT_TRIG, ULTRASONIC_BACK_RIGHT_ECHO);
}

float readUltrasonicGeneric(uint8_t trigPin, uint8_t echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    unsigned long duration = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT);
    
    if (duration == 0) {
        return ULTRASONIC_MAX_DISTANCE; // Timeout - return max distance
    }
    
    float distance = (duration * 0.034) / 2; // Convert to cm
    
    return constrain(distance, 2, ULTRASONIC_MAX_DISTANCE);
}

void readTCS34725(ColorData_t &colorData) {
    // Read TCS34725 color sensor
    if (tcs.begin()) {
        uint16_t r, g, b, c;
        tcs.getRawData(&r, &g, &b, &c);
        
        colorData.red = r;
        colorData.green = g;
        colorData.blue = b;
        colorData.clear = c;
        colorData.ledEnabled = true;
    } else {
        // Sensor not available - return zero values
        colorData.red = 0;
        colorData.green = 0;
        colorData.blue = 0;
        colorData.clear = 0;
        colorData.ledEnabled = false;
    }
}

// ================ ACTUATOR CONTROL FUNCTIONS ================

void setCameraServoAngle(int angle) {
    angle = constrain(angle, CAMERA_SERVO_MIN, CAMERA_SERVO_MAX);
    cameraServo.write(angle);
}

void setTipperServoAngle(int angle) {
    angle = constrain(angle, TIPPER_SERVO_MIN, TIPPER_SERVO_MAX);
    tipperServo.write(angle);
}

void runStepper(int steps, int speed) {
    stepperMotor.setSpeed(constrain(speed, 1, 20)); // Limit RPM
    stepperMotor.step(steps);
}

void stepperContinuous(int speed, bool clockwise) {
    stepperMotor.setSpeed(constrain(abs(speed), 1, 20));
    int steps = clockwise ? 10 : -10; // Small steps in direction
    stepperMotor.step(steps);
}

void stopStepper() {
    // Turn off all stepper pins to save power
    digitalWrite(STEPPER_IN1, LOW);
    digitalWrite(STEPPER_IN2, LOW);
    digitalWrite(STEPPER_IN3, LOW);
    digitalWrite(STEPPER_IN4, LOW);
}

// ================ UTILITY FUNCTIONS ================

float calculateYaw(float gyroZ, float dt) {
    static float yaw = 0.0;
    yaw += gyroZ * dt;
    
    // Keep yaw in -180 to 180 range
    if (yaw > 180.0) yaw -= 360.0;
    if (yaw < -180.0) yaw += 360.0;
    
    return yaw;
}

bool isColorDetected(uint16_t red, uint16_t green, uint16_t blue, String &colorName) {
    // Simple color detection logic
    uint16_t total = red + green + blue;
    
    if (total < 100) {
        colorName = "Black";
        return true;
    }
    
    float r_ratio = (float)red / total;
    float g_ratio = (float)green / total;
    float b_ratio = (float)blue / total;
    
    if (r_ratio > 0.4 && g_ratio < 0.3 && b_ratio < 0.3) {
        colorName = "Red";
        return true;
    } else if (g_ratio > 0.4 && r_ratio < 0.3 && b_ratio < 0.3) {
        colorName = "Green";
        return true;
    } else if (b_ratio > 0.4 && r_ratio < 0.3 && g_ratio < 0.3) {
        colorName = "Blue";
        return true;
    } else if (r_ratio > 0.3 && g_ratio > 0.3 && b_ratio < 0.2) {
        colorName = "Yellow";
        return true;
    } else if (total > 500) {
        colorName = "White";
        return true;
    }
    
    colorName = "Unknown";
    return false;
}

float filterSensorData(float newValue, float oldValue, float alpha) {
    return alpha * newValue + (1.0 - alpha) * oldValue;
}

// ================ SAFETY FUNCTIONS ================

bool checkSensorHealth() {
    // Check if MPU6050 is responding
    if (!mpu.testConnection()) {
        DEBUG_SERIAL.println("MPU6050 connection lost!");
        return false;
    }
    
    // Add other sensor health checks here
    
    return true;
}

// emergencyStop() is implemented in main.cpp