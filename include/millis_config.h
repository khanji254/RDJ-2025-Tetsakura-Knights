/* millis_config.h
   Configuration for millis-based robot controller
   Cooperative multitasking system - no FreeRTOS dependencies
*/

#ifndef MILLIS_CONFIG_H
#define MILLIS_CONFIG_H

#include <Arduino.h>

// ================ TIMING CONFIGURATION ================
#define SENSOR_LOOP_INTERVAL         50    // 50ms = 20Hz
#define MOTOR_LOOP_INTERVAL          20    // 20ms = 50Hz  
#define COMM_LOOP_INTERVAL           10    // 10ms = 100Hz for quick command response
#define HEARTBEAT_INTERVAL           1000  // 1s status update

// ================ DATA STRUCTURES ================

// 3D vector for accelerometer/gyroscope data
typedef struct {
    float x;
    float y;
    float z;
} Vector3_t;

// RGB color data - matching original structure
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t clear;
    bool ledEnabled;
} ColorData_t;

// Complete sensor data structure matching original rtos_config.h
typedef struct {
    // IMU data from DMP
    float yaw;
    float pitch;
    float roll;
    Vector3_t linearAccel;
    
    // Ultrasonic sensors
    uint8_t ultrasonicLeftDistance;
    uint8_t ultrasonicRightDistance;
    uint8_t ultrasonicBackLeftDistance;
    uint8_t ultrasonicBackRightDistance;
    
    // Color sensor
    ColorData_t colorSensor;
    
    // Servo positions
    uint8_t cameraServoPosition;
    uint8_t tipperServoPosition;
    
    // Stepper position
    int16_t stepperPosition;
    
    // System status
    unsigned long timestamp;
    uint8_t dataValidFlags;
} SensorData_t;

// Motor command structure
typedef struct {
    int m1Speed;
    int m2Speed;
    int m3Speed;
    int m4Speed;
    bool motorsEnabled;
    unsigned long timestamp;
} MotorCommand_t;

#endif // MILLIS_CONFIG_H