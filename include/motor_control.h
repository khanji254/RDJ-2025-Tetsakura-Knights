#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

// Pure TB6612 motor control functions
void initializeMotors();        // Initialize both TB6612 drivers
void setM1(int speed);         // Front Left (TB6612 #1)
void setM2(int speed);         // Front Right (TB6612 #1, reversed)
void setM3(int speed);         // Rear Left (TB6612 #2)
void setM4(int speed);         // Rear Right (TB6612 #2, inverted)
void driveAll(int m1, int m2, int m3, int m4);  // Drive all 4 motors
void driveForward(int speed);   // Drive forward
void driveBackward(int speed);  // Drive backward
void turnLeft(int speed);       // Turn left
void turnRight(int speed);      // Turn right
void stopAll();                // Stop all motors
void setMotorRaw(int pwmPin, int in1, int in2, int speed);  // Raw TB6612 control
int clamp255(long v);          // Clamp speed to ±255 range
void enableMotors();           // Enable TB6612 drivers (STBY high)
void disableMotors();

#endif // MOTOR_CONTROL_H
