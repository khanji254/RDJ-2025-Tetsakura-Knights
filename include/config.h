#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ---------------- USER CONFIG ----------------
#define DEBUG_SERIAL      Serial      // USB serial
#define RADIO_SERIAL      Serial2     // UART2: pins 16(TX2)/17(RX2) - ESP8266 connection

// Motor driver pins - Mixed setup
// Front motors: TB6612 | Rear motors: L298N

// Motor 1 (Front Left) - TB6612
#define M1_PWM   3
#define M1_IN1   22
#define M1_IN2   23

// Motor 2 (Front Right) - TB6612 - REVERSED IN CODE
#define M2_PWM   5
#define M2_IN1   24
#define M2_IN2   25

// Motor 3 (Rear Left - Motor A) - L298N
#define M3_PWM   6    // ENA pin (speed control)
#define M3_IN1   26   // IN1 pin -> OUT1
#define M3_IN2   27   // IN2 pin -> OUT2

// Motor 4 (Rear Right - Motor B) - L298N - INVERTED
#define M4_PWM   9    // ENB pin (speed control)
#define M4_IN1   28   // IN3 pin -> OUT3
#define M4_IN2   29   // IN4 pin -> OUT4

#define MOTOR_STBY 40   // HIGH = enable TB6612 (front motors only)
                        // L298N doesn't need standby pin

// Encoders
#define ENC1_A_PIN 2
#define ENC1_B_PIN 30

#define ENC2_A_PIN 18
#define ENC2_B_PIN 31

#define ENC3_A_PIN 19
#define ENC3_B_PIN 32

#define ENC4_A_PIN 20
#define ENC4_B_PIN 33

// Physical constants (adjust to match your robot)
const float WHEEL_RADIUS = 0.0425;  // meters
const float GEAR_RATIO = 1.0;       // gearbox ratio if encoder before gear
const int PULSES_PER_REV = 11;      // encoder CPR (check datasheet)
const float PI_F = 3.141592653589793;
const float DIST_PER_TICK = (2.0 * PI_F * WHEEL_RADIUS) / (PULSES_PER_REV * GEAR_RATIO);

// Odometry update interval
const unsigned long ODOM_MS = 200;

#endif // CONFIG_H
