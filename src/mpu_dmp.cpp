/* mpu_dmp.cpp
   MPU6050 DMP (Digital Motion Processor) Implementation
   
   Simplified DMP implementation optimized for robot control:
   - Uses DMP for efficient quaternion calculation
   - Focuses on Z-axis (yaw) for robot turning
   - Provides acceleration data for ROS integration
   - Interrupt-driven operation
*/

#include "mpu_dmp.h"

// ================ GLOBAL VARIABLES ================
MPU6050 mpu;
volatile bool mpuInterrupt = false;
volatile bool dmpDataReady = false;
DMPData_t currentDMPData;

// DMP control variables
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];

// DMP calculation variables
Quaternion q;
VectorFloat gravity;
float ypr[3];              // [yaw, pitch, roll]
float yawOffset = 0.0;     // Calibration offset for yaw
VectorInt16 aa;            // Raw acceleration
VectorInt16 aaReal;        // Gravity-free acceleration
VectorInt16 aaWorld;       // World-frame acceleration

// ================ INITIALIZATION FUNCTIONS ================

bool initializeMPU_DMP() {
     DEBUG_SERIAL.println("Initializing MPU6050 DMP...");
    
    // Initialize I2C
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
        Wire.setClock(400000); // 400kHz I2C clock
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif
    
    // Initialize MPU6050
    mpu.initialize();
    
    // Verify connection
    if (!mpu.testConnection()) {
         DEBUG_SERIAL.println("MPU6050 connection failed!");
        return false;
    }
     DEBUG_SERIAL.println("MPU6050 connection successful");
    
    // Initialize DMP
     DEBUG_SERIAL.println("Initializing DMP...");
    devStatus = mpu.dmpInitialize();
    
    // Set initial offsets (these can be calibrated)
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788);
    
    if (devStatus == 0) {
        // Skip auto-calibration during initialization to prevent hanging
         DEBUG_SERIAL.println("DMP initialized - skipping auto-calibration for fast startup");
        
        // Use preset offsets (can be calibrated later)
        mpu.setXGyroOffset(220);
        mpu.setYGyroOffset(76);
        mpu.setZGyroOffset(-85);
        mpu.setXAccelOffset(-1137);
        mpu.setYAccelOffset(-1459);
        mpu.setZAccelOffset(1788);
        
         DEBUG_SERIAL.println("Using preset sensor offsets");
        
        // Enable DMP
         DEBUG_SERIAL.println("Enabling DMP...");
        mpu.setDMPEnabled(true);
        
        // Setup interrupt
        setupDMPInterrupt();
        
        // Get expected packet size
        packetSize = mpu.dmpGetFIFOPacketSize();
        dmpReady = true;
        
         DEBUG_SERIAL.println("DMP initialized successfully!");
         DEBUG_SERIAL.print("Expected packet size: ");
         DEBUG_SERIAL.println(packetSize);
        
        return true;
    } else {
         DEBUG_SERIAL.print("DMP Initialization failed (code ");
         DEBUG_SERIAL.print(devStatus);
         DEBUG_SERIAL.println(")");
        return false;
    }
}

void setupDMPInterrupt() {
     DEBUG_SERIAL.print("Setting up DMP interrupt on pin ");
     DEBUG_SERIAL.println(MPU6050_INT_PIN);
    
    pinMode(MPU6050_INT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(MPU6050_INT_PIN), dmpDataReadyISR, RISING);
    mpuIntStatus = mpu.getIntStatus();
    
     DEBUG_SERIAL.println("DMP interrupt configured");
}

// ================ DATA READING FUNCTIONS ================

bool readDMPData(DMPData_t &dmpData) {
    if (!dmpReady) return false;
    
    // Check if new data is available
    if (!mpuInterrupt) return false;
    mpuInterrupt = false;
    
    // Get current FIFO count
    fifoCount = mpu.getFIFOCount();
    
    // Check for overflow
    if (fifoCount >= 1024) {
        mpu.resetFIFO();
         DEBUG_SERIAL.println("FIFO overflow!");
        return false;
    }
    
    // Wait for correct available data length
    if (fifoCount < packetSize) return false;
    
    // Read a packet from FIFO
    if (!mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
        return false;
    }
    
    // Process the DMP packet
    processDMPPacket();
    
    // Fill the DMP data structure
    dmpData.yaw = ypr[0] * 180/M_PI - yawOffset;     // Convert to degrees, apply offset
    dmpData.pitch = ypr[1] * 180/M_PI;               // Convert to degrees
    dmpData.roll = ypr[2] * 180/M_PI;                // Convert to degrees
    
    // Convert acceleration to m/s² (assuming ±2g range)
    dmpData.accel_x = (aaWorld.x / 16384.0) * 9.81;  // Convert to m/s²
    dmpData.accel_y = (aaWorld.y / 16384.0) * 9.81;  // Convert to m/s²
    dmpData.accel_z = (aaWorld.z / 16384.0) * 9.81;  // Convert to m/s²
    
    dmpData.dataReady = true;
    dmpData.timestamp = millis();
    
    // Update global current data
    currentDMPData = dmpData;
    dmpDataReady = true;
    
    return true;
}

void processDMPPacket() {
    // Extract quaternion from DMP packet
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    
    // Calculate gravity vector
    mpu.dmpGetGravity(&gravity, &q);
    
    // Calculate yaw/pitch/roll
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    
    // Get raw acceleration
    mpu.dmpGetAccel(&aa, fifoBuffer);
    
    // Remove gravity from acceleration
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
    
    // Get world-frame acceleration (compensated for orientation)
    // Note: Some MPU6050 library versions may not have this function
    // Using linear acceleration instead
    aaWorld = aaReal;
}

bool isDMPDataReady() {
    return dmpDataReady;
}

// ================ UTILITY FUNCTIONS ================

float getYawAngle() {
    if (!dmpReady) return 0.0;
    return currentDMPData.yaw;
}

float getYawRate() {
    static float lastYaw = 0.0;
    static unsigned long lastTime = 0;
    
    unsigned long currentTime = millis();
    float currentYaw = getYawAngle();
    
    if (lastTime == 0) {
        lastTime = currentTime;
        lastYaw = currentYaw;
        return 0.0;
    }
    
    float deltaTime = (currentTime - lastTime) / 1000.0; // Convert to seconds
    float deltaYaw = currentYaw - lastYaw;
    
    // Handle wraparound
    if (deltaYaw > 180) deltaYaw -= 360;
    if (deltaYaw < -180) deltaYaw += 360;
    
    lastTime = currentTime;
    lastYaw = currentYaw;
    
    return deltaYaw / deltaTime; // degrees per second
}

void resetYawReference() {
    if (dmpReady && currentDMPData.dataReady) {
        yawOffset = ypr[0] * 180/M_PI;
         DEBUG_SERIAL.println("Yaw reference reset to current position");
    }
}

bool isDMPHealthy() {
    if (!dmpReady) return false;
    
    // Check if we're getting regular updates
    unsigned long currentTime = millis();
    if (currentTime - currentDMPData.timestamp > 100) { // No data for 100ms
        return false;
    }
    
    // Check for reasonable values
    if (abs(currentDMPData.yaw) > 360 || 
        abs(currentDMPData.pitch) > 90 || 
        abs(currentDMPData.roll) > 180) {
        return false;
    }
    
    return true;
}

// ================ CALIBRATION FUNCTIONS ================

void printDMPOffsets() {
    if (!dmpReady) return;
    
     DEBUG_SERIAL.println("Current DMP Offsets:");
     DEBUG_SERIAL.print("Accel X: ");  DEBUG_SERIAL.println(mpu.getXAccelOffset());
     DEBUG_SERIAL.print("Accel Y: ");  DEBUG_SERIAL.println(mpu.getYAccelOffset());
     DEBUG_SERIAL.print("Accel Z: ");  DEBUG_SERIAL.println(mpu.getZAccelOffset());
     DEBUG_SERIAL.print("Gyro X: ");  DEBUG_SERIAL.println(mpu.getXGyroOffset());
     DEBUG_SERIAL.print("Gyro Y: ");  DEBUG_SERIAL.println(mpu.getYGyroOffset());
     DEBUG_SERIAL.print("Gyro Z: ");  DEBUG_SERIAL.println(mpu.getZGyroOffset());
}

void setDMPOffsets(int16_t ax_offset, int16_t ay_offset, int16_t az_offset,
                   int16_t gx_offset, int16_t gy_offset, int16_t gz_offset) {
    mpu.setXAccelOffset(ax_offset);
    mpu.setYAccelOffset(ay_offset);
    mpu.setZAccelOffset(az_offset);
    mpu.setXGyroOffset(gx_offset);
    mpu.setYGyroOffset(gy_offset);
    mpu.setZGyroOffset(gz_offset);
    
     DEBUG_SERIAL.println("DMP offsets updated");
}

// ================ ROS INTEGRATION FUNCTIONS ================

void getDMPDataForROS(float &yaw, float &pitch, float &roll,
                      float &accel_x, float &accel_y, float &accel_z) {
    if (currentDMPData.dataReady) {
        yaw = currentDMPData.yaw;
        pitch = currentDMPData.pitch;
        roll = currentDMPData.roll;
        accel_x = currentDMPData.accel_x;
        accel_y = currentDMPData.accel_y;
        accel_z = currentDMPData.accel_z;
    } else {
        // Return zeros if no data available
        yaw = pitch = roll = 0.0;
        accel_x = accel_y = accel_z = 0.0;
    }
}

// ================ INTERRUPT SERVICE ROUTINE ================

void dmpDataReadyISR() {
    mpuInterrupt = true;
}

// ================ CALIBRATION FUNCTIONS ================

bool calibrateMPU_DMP() {
     DEBUG_SERIAL.println("Starting DMP calibration...");
    
    if (!dmpReady) {
         DEBUG_SERIAL.println("DMP not ready for calibration");
        return false;
    }
    
    // Simple calibration - just zero the yaw offset
    // This assumes the robot starts in a known orientation
    
    // Read a few samples to get a stable yaw reading
    float yawSum = 0.0;
    int validSamples = 0;
    
    for (int i = 0; i < 50; i++) {
        if (isDMPDataReady()) {
            DMPData_t data;
            if (readDMPData(data)) {
                yawSum += data.yaw;
                validSamples++;
            }
        }
        delay(10);
    }
    
    if (validSamples > 10) {
        yawOffset = yawSum / validSamples;
         DEBUG_SERIAL.print("DMP calibration complete - yaw offset: ");
         DEBUG_SERIAL.println(yawOffset);
        return true;
    } else {
         DEBUG_SERIAL.println("DMP calibration failed - insufficient samples");
        yawOffset = 0.0;
        return false;
    }
}
