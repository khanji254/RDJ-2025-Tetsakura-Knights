#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <Stepper.h>
#include "config.h"
#include "millis_config.h"
#include "mpu_dmp.h"  // Use DMP module instead of basic MPU6050

// ================ SENSOR OBJECTS ================
extern Servo cameraServo;
extern Servo tipperServo;
extern Stepper stepperMotor;

// ================ CALIBRATION DATA ================
typedef struct {
    float gyro_x_offset;
    float gyro_y_offset; 
    float gyro_z_offset;
    float accel_x_offset;
    float accel_y_offset;
    float accel_z_offset;
} MPU6050_Calibration_t;

extern MPU6050_Calibration_t mpu_calibration;

// ================ FUNCTION DECLARATIONS ================

// Initialization functions
void initializeSensors();
void initializeMPU6050();
void initializeUltrasonicSensors();
void initializeTCS34725();
void initializeServos();
void initializeStepper();

// Calibration functions
void calibrateMPU6050();
void saveMPU6050Calibration();
void loadMPU6050Calibration();

// Sensor reading functions - Updated for 4 ultrasonic sensors and DMP
SensorData_t readAllSensors();
void readMPU_DMP(float &yaw, float &pitch, float &roll, Vector3_t &linearAccel);
uint16_t readUltrasonicLeft();
uint16_t readUltrasonicRight();
uint16_t readUltrasonicBackLeft();
uint16_t readUltrasonicBackRight();
float readUltrasonicGeneric(uint8_t trigPin, uint8_t echoPin);
void readTCS34725(ColorData_t &colorData);
void enableColorSensorLED(bool enable);

// Interrupt setup and handlers
void setupSensorInterrupts();
void mpuDataReadyISR();
void colorSensorReadyISR();

// Actuator control functions - Updated for angle and step control
void setCameraServoAngle(uint8_t angle);     // Exact angle control
void setTipperServoAngle(uint8_t angle);     // Exact angle control  
void runStepperSteps(int steps, uint16_t speed); // Step-based control for 28BYJ-48
void runStepperRevolutions(float revolutions, uint16_t speed); // Revolution-based control
void stepperContinuous(int speed, bool clockwise);
void stopStepper();

// Data processing functions
float calculateHeading(float gyroZ, float dt);
bool detectColor(const ColorData_t &color, String &detectedColor);
float filterSensorData(float newValue, float oldValue, float alpha = 0.3);

// Safety and health monitoring
bool checkSensorHealth();
bool checkMPU6050Health();
bool checkColorSensorHealth();
void sensorEmergencyStop();

// Utility functions for ROS integration
void convertSensorDataToROS(const SensorData_t &data);
bool areSensorsReady();
void resetSensorErrorFlags();

// Interrupt flags
extern volatile bool mpuDataReady;
extern volatile bool colorSensorDataReady;

#endif // SENSOR_MANAGER_H