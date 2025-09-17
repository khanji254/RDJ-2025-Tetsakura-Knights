#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Main command handling functions
void handleSerialCommands(String &rxBuf);
void processCommand(const String &command);

// Command parsing functions (string and JSON variants)
void parseMotorCommand(const String &command);
void parseMotorCommand(JsonArray arr);
void parseServoCommand(const String &command);
void parseStepperCommand(const String &command);
void parseModeCommand(const String &command);
void parseInitCommand(const String &command);
void parseInitCommand(JsonVariant args);
void parseDebugCommand(const String &command);
void parseDebugCommand(JsonVariant args);
void parsePWMCommand(const String &command);
void parsePWMCommand(JsonVariant args);
// JSON-only helpers to avoid overload ambiguity
void parseInitJson(JsonVariant args);
void parseDebugJson(JsonVariant args);
void parsePWMJson(JsonVariant args);

// Status and response functions
void sendStatusUpdate();
void sendSensorUpdate();
void resetRobotState();
// Send a JSON response on RADIO_SERIAL (resp, optional type and message)
void sendJsonResponse(const char* resp, const char* type = nullptr, const char* msg = nullptr);

// Debug forwarding functions
void forwardDebugMessage(const String &message);
extern bool debugForwardingEnabled;

// Test functions
void runSimpleMotorTest();

#endif // COMMAND_PARSER_H
