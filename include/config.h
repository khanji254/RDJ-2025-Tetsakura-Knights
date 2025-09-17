#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ---------------- SERIAL COMMUNICATION CONFIG ----------------
#define DEBUG_SERIAL      Serial     // UART3: pins 14(TX3)/15(RX3) - USB-to-TTL debugging 
#define RADIO_SERIAL      Serial2     // UART2: pins 16(TX2)/17(RX2) - ESP8266 connection
#define DEBUG3_SERIAL     Serial3     // USB serial

// ---------------- MOTOR DRIVER CONFIGURATION ----------------
// Pure TB6612 Configuration - All 4 motors use TB6612 drivers

// Motor 1 (Front Left) - TB6612 #1
#define M1_PWM   3
#define M1_IN1   22
#define M1_IN2   23

// Motor 2 (Front Right) - TB6612 #1 - REVERSED IN CODE
#define M2_PWM   5
#define M2_IN1   24
#define M2_IN2   25

// Motor 3 (Rear Left) - TB6612 #2
#define M3_PWM   6
#define M3_IN1   26
#define M3_IN2   27

// Motor 4 (Rear Right) - TB6612 #2
#define M4_PWM   9
#define M4_IN1   28
#define M4_IN2   29

#define MOTOR_STBY 40   // Shared standby pin for both TB6612 drivers (HIGH = enable)

// ---------------- ENCODER CONFIGURATION ----------------
// Encoders - Hardware interrupts for high-frequency odometry
// Updated to match actual circuit connections
#define ENC1_A_PIN 2    // INT0 - Front Left A (Yellow)
#define ENC1_B_PIN 30   // Front Left B (Green)

#define ENC2_A_PIN 18   // INT5 - Front Right A (Yellow) - CORRECTED!
#define ENC2_B_PIN 31   // Front Right B (Green)

#define ENC3_A_PIN 19   // INT4 - Rear Left A (Yellow) - CORRECTED!
#define ENC3_B_PIN 32   // Rear Left B (Green)

#define ENC4_A_PIN 14   // PCINT - Rear Right A (Yellow)
#define ENC4_B_PIN 33   // Rear Right B (Green)

// Ultrasonic Sensors (4 sensors for comprehensive mapping during skidding)
#define ULTRASONIC_LEFT_TRIG_PIN      A6   // Left side trigger
#define ULTRASONIC_LEFT_ECHO_PIN      A4   // Left side echo
#define ULTRASONIC_RIGHT_TRIG_PIN     A7   // Right side trigger  
#define ULTRASONIC_RIGHT_ECHO_PIN     A5   // Right side echo
#define ULTRASONIC_BACK_LEFT_TRIG_PIN  36  // Back left trigger
#define ULTRASONIC_BACK_LEFT_ECHO_PIN  37  // Back left echo
#define ULTRASONIC_BACK_RIGHT_TRIG_PIN 38  // Back right trigger
#define ULTRASONIC_BACK_RIGHT_ECHO_PIN 39  // Back right echo

// I2C Sensors (SDA=20, SCL=21)
#define MPU6050_INT_PIN       A1    // MPU6050 interrupt pin
#define TCS34725_INT_PIN      A0    // Color sensor interrupt pin
#define TCS34725_LED_PIN      A3    // Color sensor LED control

// Servo Pins
#define CAMERA_SERVO_PIN      42    // Camera tilt servo
#define TIPPER_SERVO_PIN      44    // Tipper servo

// Stepper Motor Pins (ULN2003AN) - Distance-based control
#define STEPPER_IN1_PIN       45
#define STEPPER_IN2_PIN       47  
#define STEPPER_IN3_PIN       49
#define STEPPER_IN4_PIN       51

// Motor Configuration
#define MOTOR_MAX_SPEED       255
#define MOTOR_MIN_SPEED       50

// Physical constants (adjust to match your robot)
const float WHEEL_RADIUS = 0.0425;  // meters
const float GEAR_RATIO = 1.0;       // gearbox ratio if encoder before gear
const int PULSES_PER_REV = 20;      // encoder CPR for 25GA370
const float PI_F = 3.141592653589793;
const float DIST_PER_TICK = (2.0 * PI_F * WHEEL_RADIUS) / (PULSES_PER_REV * GEAR_RATIO);

// Sensor Configuration  
#define ULTRASONIC_MAX_DISTANCE   200   // cm
#define ULTRASONIC_TIMEOUT        1000 // microseconds

// 28BYJ-48 5V Stepper Motor Configuration (with ULN2003AN driver)
#define STEPPER_STEPS_PER_REV     2048  // 28BYJ-48: 32 steps * 64:1 gear ratio = 2048 steps/rev
#define STEPPER_MAX_SPEED         15    // Max RPM for 28BYJ-48 (recommended: 10-15 RPM)
#define STEPPER_DEFAULT_SPEED     10    // Default RPM for smooth operation

// Stepper pin aliases for compatibility
#define STEPPER_IN1               STEPPER_IN1_PIN
#define STEPPER_IN2               STEPPER_IN2_PIN
#define STEPPER_IN3               STEPPER_IN3_PIN
#define STEPPER_IN4               STEPPER_IN4_PIN

// Ultrasonic pin aliases for compatibility  
#define ULTRASONIC_LEFT_TRIG      ULTRASONIC_LEFT_TRIG_PIN
#define ULTRASONIC_LEFT_ECHO      ULTRASONIC_LEFT_ECHO_PIN
#define ULTRASONIC_RIGHT_TRIG     ULTRASONIC_RIGHT_TRIG_PIN
#define ULTRASONIC_RIGHT_ECHO     ULTRASONIC_RIGHT_ECHO_PIN
#define ULTRASONIC_BACK_LEFT_TRIG ULTRASONIC_BACK_LEFT_TRIG_PIN
#define ULTRASONIC_BACK_LEFT_ECHO ULTRASONIC_BACK_LEFT_ECHO_PIN
#define ULTRASONIC_BACK_RIGHT_TRIG ULTRASONIC_BACK_RIGHT_TRIG_PIN
#define ULTRASONIC_BACK_RIGHT_ECHO ULTRASONIC_BACK_RIGHT_ECHO_PIN

// Legacy ultrasonic pins for compatibility
#define ULTRASONIC1_TRIG          ULTRASONIC_LEFT_TRIG_PIN
#define ULTRASONIC1_ECHO          ULTRASONIC_LEFT_ECHO_PIN
#define ULTRASONIC2_TRIG          ULTRASONIC_RIGHT_TRIG_PIN
#define ULTRASONIC2_ECHO          ULTRASONIC_RIGHT_ECHO_PIN

// MPU6050 Calibration Configuration
#define MPU6050_CALIBRATION_SAMPLES 1000

// Servo Limits
#define CAMERA_SERVO_MIN          0
#define CAMERA_SERVO_MAX          180
#define TIPPER_SERVO_MIN          0
#define TIPPER_SERVO_MAX          180

// I2C Addresses
#define MPU6050_ADDRESS       0x68
// TCS34725_ADDRESS is defined by Adafruit library

// Timing Constants (High frequency for ROS integration)
#define ODOM_UPDATE_INTERVAL     10    // ms (100Hz for ROS)
#define SENSOR_UPDATE_INTERVAL   20    // ms (50Hz)
#define IMU_UPDATE_INTERVAL      10    // ms (100Hz - same as odometry)
#define STATUS_UPDATE_INTERVAL   100   // ms (10Hz)

// ROS Integration
#define ROS_ODOM_RATE           100   // Hz - odometry publishing rate
#define ROS_SENSOR_RATE         50    // Hz - sensor data publishing rate

// Odometry update interval (high frequency for ROS)
const unsigned long ODOM_MS = 10;  // 100Hz for precise ROS integration

#endif // CONFIG_H
