/* command_parser_millis.cpp 
   JSON-only command parsing for robot control - Millis Version
   
   Handles JSON commands from ESP8266 via Serial2
   No FreeRTOS dependencies - direct function calls
   Format: {"cmd":"M","args":[m1,m2,m3,m4,enable]}
*/

#include "command_parser.h"
#include "motor_control.h"
#include "encoder.h"
#include "config.h"
#include "millis_config.h"
#include "sensor_manager.h"
#include "ArduinoJson.h"

// Global variables
extern bool motorsEnabled;
extern bool systemInitialized;

// ================= JSON COMMAND PROCESSING =================

// Motor command: {"cmd":"M","args":[m1,m2,m3,m4,enable]}
void parseMotorCommand(JsonArray arr) {
  DEBUG_SERIAL.print("Motor command array size: ");
  DEBUG_SERIAL.println(arr.size());
  for (size_t i = 0; i < arr.size(); ++i) {
    DEBUG_SERIAL.print("Arg[");
    DEBUG_SERIAL.print(i);
    DEBUG_SERIAL.print("]: ");
    DEBUG_SERIAL.println(arr[i].as<int>());
  }

  // Only proceed if array is exactly 5 and all are integers
  if (arr.size() != 5) {
    DEBUG_SERIAL.println("Motor command: expected 5 arguments");
    return;
  }
  for (size_t i = 0; i < 5; ++i) {
    if (!arr[i].is<int>()) {
      DEBUG_SERIAL.print("Motor command: argument ");
      DEBUG_SERIAL.print(i);
      DEBUG_SERIAL.println(" is not an integer");
      return;
    }
  }

  int m1 = arr[0];
  int m2 = arr[1];
  int m3 = arr[2];
  int m4 = arr[3];
  int enable = arr[4];

  motorsEnabled = (enable == 1);

  // CRITICAL: Always ensure STBY is HIGH when motors should be active
  if (motorsEnabled && (m1 != 0 || m2 != 0 || m3 != 0 || m4 != 0)) {
    digitalWrite(MOTOR_STBY, HIGH);  // Enable TB6612 drivers
    DEBUG_SERIAL.println("STBY enabled for motor command");
  }

  if (motorsEnabled) {
    driveAll(m1, m2, m3, m4);
    DEBUG_SERIAL.print("Motors: ");
    DEBUG_SERIAL.print(m1); DEBUG_SERIAL.print(",");
    DEBUG_SERIAL.print(m2); DEBUG_SERIAL.print(",");
    DEBUG_SERIAL.print(m3); DEBUG_SERIAL.print(",");
    DEBUG_SERIAL.print(m4); DEBUG_SERIAL.print(" EN=");
    DEBUG_SERIAL.println(enable);
  } else {
    driveAll(0, 0, 0, 0);
    digitalWrite(MOTOR_STBY, LOW);   // Disable TB6612 drivers when motors disabled
    DEBUG_SERIAL.println("Motors disabled");
  }
}

// Simple test function
void runSimpleMotorTest() {
  int testSpeed = 180; // 0-255 PWM value

  DEBUG_SERIAL.println("Starting simple motor test");

  // Ensure STBY is HIGH (critical!)
  digitalWrite(MOTOR_STBY, HIGH);
  DEBUG_SERIAL.println("STBY enabled");

  // Forward test
  DEBUG_SERIAL.println("Forward");
  driveAll(testSpeed, testSpeed, testSpeed, testSpeed);
  delay(2000);

  // Stop
  DEBUG_SERIAL.println("Stop");
  driveAll(0, 0, 0, 0);
  delay(1000);

  DEBUG_SERIAL.println("Motor test completed");
  // Always send a valid response from processJsonCommand, not here
}

// PWM command: {"cmd":"PWM","args":[m1,m2,m3,m4]}
// or          {"cmd":"PWM","args":{"motor":2,"value":150}}
void parsePWMCommand(JsonVariant args) {
  if (args.is<JsonArray>()) {
    JsonArray arr = args.as<JsonArray>();
    if (arr.size() == 4) {
      driveAll(arr[0], arr[1], arr[2], arr[3]);
      DEBUG_SERIAL.print("PWM All: ");
      DEBUG_SERIAL.print(arr[0].as<int>()); DEBUG_SERIAL.print(",");
      DEBUG_SERIAL.print(arr[1].as<int>()); DEBUG_SERIAL.print(",");
      DEBUG_SERIAL.print(arr[2].as<int>()); DEBUG_SERIAL.print(",");
      DEBUG_SERIAL.println(arr[3].as<int>());
    } else {
      DEBUG_SERIAL.println("PWM Array: expected 4 arguments");
    }
  } else if (args.is<JsonObject>()) {
    int motorNum = args["motor"] | 0;
    int value = args["value"] | 0;

    if (motorNum >= 1 && motorNum <= 4) {
      switch (motorNum) {
        case 1: setM1(value); break;
        case 2: setM2(value); break;
        case 3: setM3(value); break;
        case 4: setM4(value); break;
      }
      DEBUG_SERIAL.print("PWM Motor ");
      DEBUG_SERIAL.print(motorNum);
      DEBUG_SERIAL.print(": ");
      DEBUG_SERIAL.println(value);
    } else {
      DEBUG_SERIAL.println("PWM Object: invalid motor number (1-4)");
    }
  } else {
    DEBUG_SERIAL.println("PWM: invalid argument format");
  }
}

// Init command: {"cmd":"INIT","args":"SYSTEM"}
// or           {"cmd":"INIT","args":"MOTORS"}
// or           {"cmd":"INIT","args":"SENSORS"}
void parseInitCommand(const String& command) {
  DEBUG_SERIAL.print("INIT command received: ");
  DEBUG_SERIAL.println(command);
  if (command == "SYSTEM") {
      systemInitialized = true;
      DEBUG_SERIAL.println("System initialized via command");
  } else if (command == "MOTORS") {
      initializeMotors();
      DEBUG_SERIAL.println("Motors re-initialized via command");
  } else if (command == "SENSORS") {
      initializeSensors();
      DEBUG_SERIAL.println("Sensors re-initialized via command");
  } else {
      DEBUG_SERIAL.println("INIT: unknown argument");
    }
} 
// Debug command: {"cmd":"DEBUG","args":"ON"} or {"cmd":"DEBUG","args":"OFF"}
void parseDebugCommand(JsonVariant args) {
  if (args.is<const char*>()) {
    String arg = args.as<const char*>();
    if (arg == "ON") {
      DEBUG_SERIAL.println("Debug mode enabled via command");
    } else if (arg == "OFF") {
      DEBUG_SERIAL.println("Debug mode disabled via command");
    } else {
      DEBUG_SERIAL.println("DEBUG: unknown argument");
    }
  } else {
    DEBUG_SERIAL.println("DEBUG: argument must be a string");
  }
}

// Enable/Disable commands
void parseEnableCommand(bool enable) {
  motorsEnabled = enable;
  if (enable) {
    digitalWrite(MOTOR_STBY, HIGH);
    DEBUG_SERIAL.println("Motors enabled via command");
  } else {
    driveAll(0, 0, 0, 0);
    digitalWrite(MOTOR_STBY, LOW);
    DEBUG_SERIAL.println("Motors disabled via command");
  }
  // Response is sent from processJsonCommand
}

// Status command handler
void handleStatusCommand() {
  DEBUG_SERIAL.println("Status command received");
  // Let updateStatus() handle the actual status response
}

// Ping command handler
void handlePingCommand() {
  DEBUG_SERIAL.println("Ping command received");
  // Response is sent from processJsonCommand
}




// Main JSON command processor - called from the main sketch
void processJsonCommand(JsonDocument& doc) {
  const char* cmd = doc["cmd"] | "";

  DEBUG_SERIAL.print("[JSON CMD] Processing: ");
  DEBUG_SERIAL.println(cmd);

  if (strcmp(cmd, "M") == 0) {
    if (doc.containsKey("args")) {
      JsonVariant args = doc["args"];
      if (args.is<JsonArray>()) {
        parseMotorCommand(args.as<JsonArray>());
        sendJsonResponse("ACK", "MOTOR");
      } else if (args.is<const char*>()) {
        DEBUG_SERIAL.println("M command: received string args (not array)");
        sendJsonResponse("ERROR", "MOTOR", "ARGS_NOT_ARRAY");
      } else {
        DEBUG_SERIAL.println("M command: invalid args type");
        sendJsonResponse("ERROR", "MOTOR", "BAD_ARGS_TYPE");
      }
    } else {
      DEBUG_SERIAL.println("M command: missing args");
      sendJsonResponse("ERROR", "MOTOR", "MISSING_ARGS");
    }
  }
  else if (strcmp(cmd, "PWM") == 0) {
    if (doc.containsKey("args")) {
      JsonVariant args = doc["args"];
      parsePWMCommand(args);
      sendJsonResponse("ACK", "PWM");
    } else {
      DEBUG_SERIAL.println("PWM command: missing args");
      sendJsonResponse("ERROR", "PWM", "BAD_ARGS");
    }
  }
  else if (strcmp(cmd, "INIT") == 0) {
    if (doc.containsKey("args") && doc["args"].is<const char*>()) {
      parseInitCommand(String(doc["args"].as<const char*>()));
      sendJsonResponse("ACK", "INIT");
    } else {
      DEBUG_SERIAL.println("INIT command: missing or invalid args");
      sendJsonResponse("ERROR", "INIT", "BAD_ARGS");
    }
  }
  else if (strcmp(cmd, "HELLO") == 0) {
    const char* arg = doc["args"] | "";
    DEBUG_SERIAL.print("[HELLO] ");
    DEBUG_SERIAL.println(arg);
    sendJsonResponse("ACK", "HELLO", "MEGA_READY");
  }


  else if (strcmp(cmd, "SENSORS") == 0) {
    SensorData_t data = readAllSensors();
    StaticJsonDocument<256> respDoc;
    respDoc["resp"] = "SENSORS";
    respDoc["yaw"] = data.yaw;
    respDoc["pitch"] = data.pitch;
    respDoc["roll"] = data.roll;
    String out;
    serializeJson(respDoc, out);
    RADIO_SERIAL.println(out);
    RADIO_SERIAL.flush();
  }
  else if (strcmp(cmd, "DEBUG") == 0) {
    if (doc.containsKey("args")) {
      JsonVariant args = doc["args"];
      parseDebugCommand(args);
      sendJsonResponse("ACK", "DEBUG");
    } else {
      DEBUG_SERIAL.println("DEBUG command: missing args");
      sendJsonResponse("ERROR", "DEBUG", "BAD_ARGS");
    }
  }
  else if (strcmp(cmd, "ENABLE") == 0) {
    parseEnableCommand(true);
    sendJsonResponse("ACK", "ENABLE");
  }
  else if (strcmp(cmd, "DISABLE") == 0) {
    parseEnableCommand(false);
    sendJsonResponse("ACK", "DISABLE");
  }
  else if (strcmp(cmd, "STATUS") == 0) {
    // Always send a valid JSON status response!
    StaticJsonDocument<128> respDoc;
    respDoc["resp"] = "STATUS";
    respDoc["motors"] = motorsEnabled ? 1 : 0;
    respDoc["system"] = systemInitialized ? "ready" : "init";
    String out;
    serializeJson(respDoc, out);
    RADIO_SERIAL.println(out);
    RADIO_SERIAL.flush();
  }
  else if (strcmp(cmd, "PING") == 0) {
    handlePingCommand();
    sendJsonResponse("PONG");
  }
  else if (strcmp(cmd, "TEST") == 0) {
    runSimpleMotorTest();
    sendJsonResponse("ACK", "TEST_COMPLETE");
  }
  else if (strcmp(cmd, "ESP8266") == 0) {
    const char* arg = doc["args"] | "";
    DEBUG_SERIAL.print("[ESP] Startup: ");
    DEBUG_SERIAL.println(arg);
    sendJsonResponse("ACK", "ARDUINO_READY");
  }
  else {
    DEBUG_SERIAL.print("Unknown command: ");
    DEBUG_SERIAL.println(cmd);
    sendJsonResponse("ERROR", "UNKNOWN_CMD");
  }
}
// JSON-specific thin wrappers to avoid overload ambiguity when calling from processJsonCommand
void parsePWMJson(JsonVariant args) {
  parsePWMCommand(args);
}

void parseInitJson(JsonVariant args) {
  parseInitCommand(args);
}

void parseDebugJson(JsonVariant args) {
  parseDebugCommand(args);
}