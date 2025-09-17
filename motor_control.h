#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

void initializeMotors();
void setM1(int speed);
void setM2(int speed);
void setM3(int speed);
void setM4(int speed);
void driveAll(int m1, int m2, int m3, int m4);
void driveForward(int speed);
void driveBackward(int speed);
void turnLeft(int speed);
void turnRight(int speed);
void stopAll();
void setMotorRaw(int pwmPin, int in1, int in2, int speed);
int clamp255(long v);
void enableMotors();
void disableMotors();

#endif // MOTOR_CONTROL_H
