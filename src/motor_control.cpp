#include "motor_control.h"
#include "config.h"

// ---------------- Helpers ----------------
int clamp255(long v) {
  if (v > 255) return 255;
  if (v < -255) return -255;
  return (int)v;
}

// ---------------- Motor control ----------------
void setMotorRaw(int pwmPin, int in1, int in2, int speed) {
  if (speed > 0) {
    digitalWrite(in1, HIGH); 
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, speed);
  } else if (speed < 0) {
    digitalWrite(in1, LOW); 
    digitalWrite(in2, HIGH);
    analogWrite(pwmPin, -speed);
  } else {
    digitalWrite(in1, LOW); 
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, 0);
  }
}

// L298N motor control function (same logic as TB6612 but for clarity)
void setMotorL298N(int enPin, int in1, int in2, int speed) {
  if (speed > 0) {
    digitalWrite(in1, HIGH); 
    digitalWrite(in2, LOW);
    analogWrite(enPin, speed);
  } else if (speed < 0) {
    digitalWrite(in1, LOW); 
    digitalWrite(in2, HIGH);
    analogWrite(enPin, -speed);
  } else {
    digitalWrite(in1, LOW); 
    digitalWrite(in2, LOW);
    analogWrite(enPin, 0);
  }
}

// Individual motors
void setM1(int speed) { 
  // Front Left - TB6612
  setMotorRaw(M1_PWM, M1_IN1, M1_IN2, clamp255(speed)); 
}

void setM2(int speed) { 
  // Front Right - TB6612 - REVERSED
  setMotorRaw(M2_PWM, M2_IN1, M2_IN2, clamp255(-speed)); 
}

void setM3(int speed) { 
  // Rear Left - L298N Motor A (OUT1, OUT2)
  setMotorL298N(M3_PWM, M3_IN1, M3_IN2, clamp255(speed)); 
}

void setM4(int speed) { 
  // Rear Right - L298N Motor B (OUT3, OUT4) - INVERTED
  setMotorL298N(M4_PWM, M4_IN1, M4_IN2, clamp255(-speed)); 
}

// Drive all four
void driveAll(int m1, int m2, int m3, int m4) {
  setM1(m1); 
  setM2(m2); 
  setM3(m3); 
  setM4(m4);
}

// Motion patterns
void driveForward(int speed) { 
  driveAll(speed, speed, speed, speed); 
}

void driveBackward(int speed) { 
  driveAll(-speed, -speed, -speed, -speed); 
}

void turnLeft(int speed) { 
  driveAll(-speed, speed, -speed, speed); 
}

void turnRight(int speed) { 
  driveAll(speed, -speed, speed, -speed); 
}

void stopAll() { 
  driveAll(0, 0, 0, 0); 
}

void enableMotors() {
  digitalWrite(MOTOR_STBY, HIGH);
}

void disableMotors() {
  digitalWrite(MOTOR_STBY, LOW);
}

void initializeMotors() {
  // Front motors (M1, M2) - TB6612 pins
  pinMode(M1_IN1, OUTPUT); 
  pinMode(M1_IN2, OUTPUT); 
  pinMode(M1_PWM, OUTPUT);
  pinMode(M2_IN1, OUTPUT); 
  pinMode(M2_IN2, OUTPUT); 
  pinMode(M2_PWM, OUTPUT);
  
  // Rear motors (M3, M4) - L298N pins
  pinMode(M3_IN1, OUTPUT);  // L298N IN1
  pinMode(M3_IN2, OUTPUT);  // L298N IN2
  pinMode(M3_PWM, OUTPUT);  // L298N ENA
  pinMode(M4_IN1, OUTPUT);  // L298N IN3
  pinMode(M4_IN2, OUTPUT);  // L298N IN4
  pinMode(M4_PWM, OUTPUT);  // L298N ENB

  // TB6612 standby pin (front motors only)
  pinMode(MOTOR_STBY, OUTPUT);
  digitalWrite(MOTOR_STBY, HIGH);
  
  // L298N doesn't need standby - always enabled when powered
}
