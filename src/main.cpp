/*
 * Arduino Mega Robot Controller - MILLIS-BASED VERSION
 * Simple cooperative multitasking using millis() timers
 * Much more memory efficient than FreeRTOS
 */

#include "config.h"
#include "motor_control.h"
#include "sensor_manager.h"
#include "command_parser.h"
#include "encoder.h"
#include "odometry.h"
#include "mpu_dmp.h"
#include <ArduinoJson.h>



// Timing intervals using millis_config.h definitions
const unsigned long STATUS_LOOP_INTERVAL = 1000;  // 1Hz status updates
const unsigned long ODOMETRY_LOOP_INTERVAL = 25;  // 40Hz odometry
const unsigned long WATCHDOG_LOOP_INTERVAL = 5000; // 5 second watchdog

// Last update times
unsigned long lastSensorUpdate = 0;
unsigned long lastMotorUpdate = 0;
unsigned long lastCommUpdate = 0;
unsigned long lastStatusUpdate = 0;
unsigned long lastOdometryUpdate = 0;
unsigned long lastWatchdog = 0;

// System state
bool systemInitialized = false;
bool motorsEnabled = false;
unsigned long bootTime = 0;
unsigned long lastCommandTime = 0;
unsigned long lastOdomMillis = 0;

// Simple command buffer
String commandBuffer = "";

// ================ FORWARD DECLARATIONS ================
void processJsonCommand(JsonDocument& doc);
void handleCommunication();
void updateMotorControl();
void updateSensors();
void updateOdometry();
void updateStatus();
void runWatchdog();
void processCommand(const String& command);
int getFreeRAM();

// ================ UTILITY FUNCTIONS ================
int getFreeRAM() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void sendJsonResponse(const char* resp, const char* type, const char* msg) {
  StaticJsonDocument<200> doc;
  doc["resp"] = resp;
  if (type != nullptr && strlen(type) > 0) doc["type"] = type;
  if (msg != nullptr && strlen(msg) > 0) doc["msg"] = msg;

  String output;
  serializeJson(doc, output);
  RADIO_SERIAL.println(output);
  RADIO_SERIAL.flush();
}

// ================ INITIALIZATION ================
void setup() {
  bootTime = millis();
  
  // Initialize serial portsDEBUG_SERIAL.begin(115200);  // USB debug
  DEBUG_SERIAL.begin(115200);  // ESP8266 communication
  RADIO_SERIAL.begin(115200);  // ESP8266 communication
  DEBUG3_SERIAL.begin(115200); // Serial3 debug port
  
  delay(2000);  // Allow serial to stabilize
  
  DEBUG_SERIAL.println("=== Arduino Mega Robot Controller - Millis Version ===");
  DEBUG_SERIAL.println("Initializing...");
  
  // Initialize subsystems
  DEBUG_SERIAL.println("Step 1: Initializing motors...");
  initializeMotors();
  DEBUG_SERIAL.println("Motors initialized successfully");
  
  DEBUG_SERIAL.println("Step 2: Initializing encoders...");
  initializeEncoders();
  DEBUG_SERIAL.println("Encoders initialized successfully");
  
  DEBUG_SERIAL.println("Step 3: Initializing sensors...");
  initializeSensors();
  DEBUG_SERIAL.println("Sensors initialized successfully");
  
  DEBUG_SERIAL.println("Step 4: Initializing DMP...");
  if (!initializeMPU_DMP()) {
    DEBUG_SERIAL.println("WARNING: DMP initialization failed");
  } else {
    DEBUG_SERIAL.println("DMP initialized successfully");
  }
  
  // Initialize odometry
  DEBUG_SERIAL.println("Odometry system ready");
  
  // System ready
  systemInitialized = true;
  lastCommandTime = millis();
  lastOdomMillis = millis();
  
  DEBUG_SERIAL.println("=== System Ready ===");
  DEBUG_SERIAL.print("Free RAM: ");
  DEBUG_SERIAL.print(getFreeRAM());
  DEBUG_SERIAL.println(" bytes");
  
  // Send ready signal to ESP8266
  DEBUG_SERIAL.println("Sending READY signal to ESP8266 via Serial2...");
  //sendJsonResponse("READY");
  DEBUG_SERIAL.println("Arduino ready. Waiting for JSON commands via Serial2...");
  DEBUG_SERIAL.println("JSON format: {\"cmd\":\"M\",\"args\":[100,100,100,100,1]}");
}

// ================ MAIN LOOP ================
void loop() {
  unsigned long currentTime = millis();
  
  // Communication handling - Highest priority
  if (currentTime - lastCommUpdate >= COMM_LOOP_INTERVAL) {
    handleCommunication();
    lastCommUpdate = currentTime;
  }
  
  // Sensor reading - Medium priority
  if (currentTime - lastSensorUpdate >= SENSOR_LOOP_INTERVAL) {
    updateSensors();
    lastSensorUpdate = currentTime;
  }
  
  // Odometry calculation - Medium priority
  if (currentTime - lastOdometryUpdate >= ODOMETRY_LOOP_INTERVAL) {
    updateOdometry();
    lastOdometryUpdate = currentTime;
  }
  
  // Status updates - Low priority
  if (currentTime - lastStatusUpdate >= STATUS_LOOP_INTERVAL) {
     updateStatus();
    lastStatusUpdate = currentTime;
  }
  
  // Watchdog - Safety check
  if (currentTime - lastWatchdog >= WATCHDOG_LOOP_INTERVAL) {
     runWatchdog();
    lastWatchdog = currentTime;
  }
}

// ================ UPDATE FUNCTIONS ================
void handleCommunication() {
  // Handle Serial2 (ESP8266) commands  
  while (RADIO_SERIAL.available()) {
    char c = RADIO_SERIAL.read();
    
    if (c == '\n' || c == '\r') {
      commandBuffer.trim();
      if (commandBuffer.length() > 0) {
        DEBUG_SERIAL.println("[ESP CMD] " + commandBuffer);
        processCommand(commandBuffer);
        lastCommandTime = millis();
      }
      commandBuffer = "";
    } else if (c >= 32 && c <= 126) { // Printable characters only
      commandBuffer += c;
      if (commandBuffer.length() > 200) {
        DEBUG_SERIAL.println("[ESP] Buffer overflow, clearing");
        commandBuffer = "";
      }
    }
  }
  
  // Also check USB Serial for manual testing - BUT IGNORE DEBUG OUTPUT
  // while (DEBUG_SERIAL.available()) {
  //   char c = DEBUG_SERIAL.read();
    
  //   // Skip characters that are part of our own debug output
  //   // Look for the start of a real command (typically '{')
  //   if (c == '{') {
  //     commandBuffer = "{"; // Start new command
  //   } 
  //   else if (commandBuffer.length() > 0) {
  //     // We're in the middle of processing a command
  //     if (c == '\n' || c == '\r') {
  //       commandBuffer.trim();
  //       if (commandBuffer.length() > 0 && commandBuffer.charAt(0) == '{') {
  //         DEBUG_SERIAL.println("[USB] Processing command: " + commandBuffer);
  //         processCommand(commandBuffer);
  //         lastCommandTime = millis();
  //       }
  //       commandBuffer = "";
  //     } else if (c >= 32 && c <= 126) {
  //       commandBuffer += c;
  //       if (commandBuffer.length() > 200) {
  //         commandBuffer = "";
  //       }
  //     }
  //   }
  //   // Ignore all other characters (debug output)
  // }
}

void updateMotorControl() {
  // Safety: Stop motors if no command received recently
  if (millis() - lastCommandTime > 2000) { // 2 second timeout
    if (motorsEnabled) {
      driveAll(0, 0, 0, 0);
      motorsEnabled = false;
      DEBUG_SERIAL.println("Motors stopped - command timeout");
    }
  }
}

void updateSensors() {
  // Update all sensors
  readAllSensors();
  
  // Update DMP if available
  if (isDMPDataReady()) {
    DMPData_t dmpData;
    if (readDMPData(dmpData)) {
      // DMP data is automatically stored in global variables
    }
  }
}

void updateOdometry() {
  // Update odometry calculations
  processOdometry(lastOdomMillis);
}

void updateStatus() {
  // Send periodic status to ESP8266 as JSON
  StaticJsonDocument<200> statusDoc;
  statusDoc["resp"] = "STATUS";
  statusDoc["uptime"] = (millis() - bootTime) / 1000;
  statusDoc["motors"] = motorsEnabled ? 1 : 0;
  statusDoc["system"] = systemInitialized ? "ready" : "init";
  statusDoc["ram"] = getFreeRAM();
  String out;
  serializeJson(statusDoc, out);
  RADIO_SERIAL.println(out);
  RADIO_SERIAL.flush();
  
  // Debug output
  DEBUG_SERIAL.print("Uptime: ");
  DEBUG_SERIAL.print((millis() - bootTime) / 1000);
  DEBUG_SERIAL.print("s, RAM: ");
  DEBUG_SERIAL.print(getFreeRAM());
  DEBUG_SERIAL.println(" bytes");
}

void runWatchdog() {
  // Check for system health
  unsigned long uptime = millis() - bootTime;
  
  // Log system status
  DEBUG_SERIAL.print("Watchdog: Uptime ");
  DEBUG_SERIAL.print(uptime / 1000);
  DEBUG_SERIAL.print("s, RAM: ");
  DEBUG_SERIAL.print(getFreeRAM());
  DEBUG_SERIAL.print(", Motors: ");
  DEBUG_SERIAL.println(motorsEnabled ? "ON" : "OFF");
}

// ================= COMMAND PROCESSING =================
void processCommand(const String& commandRaw) {
  DEBUG_SERIAL.print("[CMD] Raw input: ");
  DEBUG_SERIAL.println(commandRaw);

  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, commandRaw);

  if (error) {
    //sendJsonResponse("ERROR", "PARSE", error.c_str());
    DEBUG_SERIAL.print("[CMD] JSON parse error: ");
    DEBUG_SERIAL.println(error.c_str());
    return;
  }

  // Use the new JSON-only command processor
  processJsonCommand(doc);
}

void forwardDebugMessage(const String &message) {
  // Forward debug messages to ESP8266 for web display as JSON
  StaticJsonDocument<200> doc;
  doc["resp"] = "DEBUG";
  // Limit message length to avoid buffer issues
  String safeMsg = message.substring(0, 120);
  doc["msg"] = safeMsg;

  String output;
  serializeJson(doc, output);
  RADIO_SERIAL.println(output);
  RADIO_SERIAL.flush(); // Ensure message is sent

  // Also output to local debug serial
  DEBUG_SERIAL.println("[DBG] " + safeMsg);
}