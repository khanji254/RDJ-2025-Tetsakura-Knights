/* Mega_Robot_Control.cpp
   Arduino Mega sketch for:
   - 4 motors via TB6612 (4 PWMs + 8 dir pins)
   - 4 encoder channels (A = interrupts, B = digital)
   - Serial1 <-> ESP-01 link (115200)
   - Odometry reporting
   - Motion commands via UART
   
   Modularized version for PlatformIO
*/

#include <Arduino.h>
#include "config.h"
#include "motor_control.h"
#include "encoder.h"
#include "odometry.h"
#include "command_parser.h"

// ---------------- Globals ----------------
unsigned long lastOdomMillis = 0;
String rxBuf = "";

// ---------------- Setup ----------------
void setup() {
  DEBUG_SERIAL.begin(115200);
  RADIO_SERIAL.begin(115200);
  DEBUG_SERIAL.println("Mega Robot Control Start...");

  // Initialize all modules
  initializeMotors();
  initializeEncoders();

  lastOdomMillis = millis();
  
  DEBUG_SERIAL.println("Robot controller initialized successfully!");
}

// ---------------- Loop ----------------
void loop() {
  // Handle incoming serial commands
  handleSerialCommands(rxBuf);

  // Process periodic odometry
  processOdometry(lastOdomMillis);
}
