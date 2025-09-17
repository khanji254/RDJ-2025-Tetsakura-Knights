/* mpu_dmp.h
   MPU6050 DMP (Digital Motion Processor) Module
   
   Simplified DMP implementation focusing on:
   - Z-axis rotation (yaw) for robot turning
   - Acceleration data for ROS integration
   - Interrupt-driven operation for efficiency
   - Reduced computational load on Arduino Mega
*/

#ifndef MPU_DMP_H
#define MPU_DMP_H

#include <Arduino.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "config.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// ================ DMP DATA STRUCTURES ================

// Simplified DMP data structure for robot control
typedef struct {
    float yaw;              // Z-axis rotation in degrees (for turns)
    float pitch;            // X-axis rotation in degrees (for tilt detection)
    float roll;             // Y-axis rotation in degrees (for balance)
    float accel_x;          // Linear acceleration X (m/s²) - for ROS
    float accel_y;          // Linear acceleration Y (m/s²) - for ROS  
    float accel_z;          // Linear acceleration Z (m/s²) - for ROS
    bool dataReady;         // True when new DMP data is available
    unsigned long timestamp; // Data timestamp
} DMPData_t;

// ================ DMP CONFIGURATION ================

// DMP output configuration - focus on what we need
#define DMP_OUTPUT_YAW_PITCH_ROLL    // Enable yaw/pitch/roll calculation
#define DMP_OUTPUT_LINEAR_ACCEL      // Enable gravity-compensated acceleration

// DMP constants
#define DMP_FIFO_RATE           100  // 100Hz DMP output rate (matches odometry)
#define DMP_PACKET_SIZE         42   // Standard DMP packet size
#define DMP_CALIBRATION_SAMPLES 1000 // Auto-calibration samples

// ================ GLOBAL VARIABLES ================
extern MPU6050 mpu;
extern volatile bool mpuInterrupt;
extern volatile bool dmpDataReady;
extern DMPData_t currentDMPData;

// DMP control variables
extern bool dmpReady;
extern uint8_t mpuIntStatus;
extern uint8_t devStatus;
extern uint16_t packetSize;
extern uint16_t fifoCount;
extern uint8_t fifoBuffer[64];

// DMP calculation variables
extern Quaternion q;
extern VectorFloat gravity;
extern float ypr[3];              // [yaw, pitch, roll]
extern VectorInt16 aa;            // Raw acceleration
extern VectorInt16 aaReal;        // Gravity-free acceleration
extern VectorInt16 aaWorld;       // World-frame acceleration

// ================ FUNCTION DECLARATIONS ================

// Initialization functions
bool initializeMPU_DMP();
bool calibrateMPU_DMP();
void setupDMPInterrupt();

// Data reading functions
bool readDMPData(DMPData_t &dmpData);
bool isDMPDataReady();
void processDMPPacket();

// Interrupt service routine
void dmpDataReadyISR();

// Utility functions
float getYawAngle();              // Get current yaw angle (Z-axis rotation)
float getYawRate();               // Get yaw rate (degrees/second)
void resetYawReference();         // Reset yaw to zero reference
bool isDMPHealthy();              // Check DMP operation health

// Calibration functions
void printDMPOffsets();
void setDMPOffsets(int16_t ax_offset, int16_t ay_offset, int16_t az_offset,
                   int16_t gx_offset, int16_t gy_offset, int16_t gz_offset);

// ROS integration functions
void getDMPDataForROS(float &yaw, float &pitch, float &roll,
                      float &accel_x, float &accel_y, float &accel_z);

#endif // MPU_DMP_H
