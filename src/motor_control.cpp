#include "motor_control.h"
#include "config.h"
#include "command_parser.h"  // For debug forwarding

// ---------------- Helpers ----------------
int clamp255(long v) {
  if (v > 255) return 255;
  if (v < -255) return -255;
  return (int)v;
}

// ---------------- Motor control ----------------
void setMotorRaw(int pwmPin, int in1, int in2, int speed) {
  String debugMsg = "setMotorRaw(PWM=" + String(pwmPin) + ", IN1=" + String(in1) + 
                   ", IN2=" + String(in2) + ", speed=" + String(speed) + ")";
  DEBUG_SERIAL.println("DEBUG3: " + debugMsg);
  //forwardDebugMessage(debugMsg);
  
  if (speed > 0) {
    digitalWrite(in1, HIGH); 
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, speed);
    String dirMsg = "Motor FORWARD - IN1=HIGH, IN2=LOW, PWM=" + String(speed);
    DEBUG_SERIAL.println("DEBUG3: " + dirMsg);
    //forwardDebugMessage(dirMsg);
  } else if (speed < 0) {
    digitalWrite(in1, LOW); 
    digitalWrite(in2, HIGH);
    analogWrite(pwmPin, -speed);
    String dirMsg = "Motor REVERSE - IN1=LOW, IN2=HIGH, PWM=" + String(-speed);
    DEBUG_SERIAL.println("DEBUG3: " + dirMsg);
    //forwardDebugMessage(dirMsg);
  } else {
    digitalWrite(in1, LOW); 
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, 0);
    String dirMsg = "Motor STOP - IN1=LOW, IN2=LOW, PWM=0";
    DEBUG_SERIAL.println("DEBUG3: " + dirMsg);
    //forwardDebugMessage(dirMsg);
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
  String msg = "setM1(speed=" + String(speed) + ") - Front Left TB6612, clamped=" + String(clamp255(speed));
  DEBUG_SERIAL.println("DEBUG3: " + msg);
  //forwardDebugMessage(msg);
  setMotorRaw(M1_PWM, M1_IN1, M1_IN2, clamp255(speed)); 
}

void setM2(int speed) { 
  // Front Right - TB6612 - NO REVERSAL FOR NOW
  String msg = "setM2(speed=" + String(speed) + ") - Front Right TB6612, clamped=" + String(clamp255(speed));
  DEBUG_SERIAL.println("DEBUG3: " + msg);
  //forwardDebugMessage(msg);
  setMotorRaw(M2_PWM, M2_IN1, M2_IN2, clamp255(speed)); 
}

void setM3(int speed) { 
  // Rear Left - TB6612 #2 Motor A
  String msg = "setM3(speed=" + String(speed) + ") - Rear Left TB6612, clamped=" + String(clamp255(speed));
  DEBUG_SERIAL.println("DEBUG3: " + msg);
  //forwardDebugMessage(msg);
  setMotorRaw(M3_PWM, M3_IN1, M3_IN2, clamp255(speed)); 
}

void setM4(int speed) { 
  // Rear Right - TB6612 #2 Motor B - NO INVERSION FOR NOW
  String msg = "setM4(speed=" + String(speed) + ") - Rear Right TB6612, clamped=" + String(clamp255(speed));
  DEBUG_SERIAL.println("DEBUG3: " + msg);
  //forwardDebugMessage(msg);
  setMotorRaw(M4_PWM, M4_IN1, M4_IN2, clamp255(speed)); 
}

// Drive all four
void driveAll(int m1, int m2, int m3, int m4) {
  String driveMsg = "driveAll(M1=" + String(m1) + ", M2=" + String(m2) + ", M3=" + String(m3) + ", M4=" + String(m4) + ")";
  DEBUG_SERIAL.println("DEBUG3: " + driveMsg);
  //forwardDebugMessage(driveMsg);
  
  // CRITICAL: Ensure STBY is HIGH whenever we set motors (like your working code)
  digitalWrite(MOTOR_STBY, HIGH);
  DEBUG_SERIAL.println("DEBUG3: STBY set HIGH before motor commands");
  
  setM1(m1); 
  setM2(m2); 
  setM3(m3); 
  setM4(m4);
  
  String completedMsg = "driveAll() completed - all motors set with STBY enabled";
  DEBUG_SERIAL.println("DEBUG3: " + completedMsg);
  //forwardDebugMessage(completedMsg);
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
  DEBUG_SERIAL.println("DEBUG3: stopAll() called");
  driveAll(0, 0, 0, 0); 
  DEBUG_SERIAL.println("DEBUG3: All motors stopped");
}

void enableMotors() {
  digitalWrite(MOTOR_STBY, HIGH);
  String msg = "Motors ENABLED - MOTOR_STBY set HIGH";
  DEBUG_SERIAL.println("DEBUG3: " + msg);
  //forwardDebugMessage(msg);
}

void disableMotors() {
  digitalWrite(MOTOR_STBY, LOW);
  String msg = "Motors DISABLED - MOTOR_STBY set LOW";
  DEBUG_SERIAL.println("DEBUG3: " + msg);
  //forwardDebugMessage(msg);
}

void initializeMotors() {
  DEBUG_SERIAL.println("DEBUG: initializeMotors() starting");
  
  // Front motors (M1, M2) - TB6612 pins
  pinMode(M1_IN1, OUTPUT); 
  pinMode(M1_IN2, OUTPUT); 
  pinMode(M1_PWM, OUTPUT);
  pinMode(M2_IN1, OUTPUT); 
  pinMode(M2_IN2, OUTPUT); 
  pinMode(M2_PWM, OUTPUT);
  DEBUG_SERIAL.println("DEBUG: Front motor pins (TB6612 #1) configured");
  
  // Rear motors (M3, M4) - TB6612 #2 pins
  pinMode(M3_IN1, OUTPUT);  // TB6612 #2 AIN1
  pinMode(M3_IN2, OUTPUT);  // TB6612 #2 AIN2
  pinMode(M3_PWM, OUTPUT);  // TB6612 #2 PWMA
  pinMode(M4_IN1, OUTPUT);  // TB6612 #2 BIN1
  pinMode(M4_IN2, OUTPUT);  // TB6612 #2 BIN2
  pinMode(M4_PWM, OUTPUT);  // TB6612 #2 PWMB
  DEBUG_SERIAL.println("DEBUG: Rear motor pins (TB6612 #2) configured");

  // TB6612 shared standby pin (controls both TB6612 drivers)
  pinMode(MOTOR_STBY, OUTPUT);
  digitalWrite(MOTOR_STBY, HIGH);
  DEBUG_SERIAL.println("DEBUG: TB6612 shared STBY pin configured and enabled");
  
  // All motors ready - pure TB6612 configuration
  DEBUG_SERIAL.println("DEBUG: All TB6612 drivers ready and enabled");
  
  // Initialize all motors to stopped state
  driveAll(0, 0, 0, 0);
  DEBUG_SERIAL.println("DEBUG: All motors initialized to stopped state");
}
