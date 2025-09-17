/*
 * ESP8266-01 Robot Web Controller
 * 
 * Creates a WiFi Access Point and web interface to control the Arduino Mega robot
 * Serial communication: ESP TX -> Mega Pin 17 (RX2), ESP RX -> Mega Pin 16 (TX2)
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include "esp_config.h"
#include "web_interface.h"
#include "robot_comm.h"

void setup() {
  // Initialize serial communication with robot
  setupRobotCommunication();
  
  // Configure WiFi Access Point
  setupWiFiAP();
  
  // Initialize web server
  setupWebServer();
  
  Serial.println("ESP8266 Robot Controller Ready!");
  Serial.println("Connect to WiFi: " + String(WIFI_SSID));
  Serial.println("Password: " + String(WIFI_PASSWORD));
  Serial.println("Open browser to: http://192.168.4.1");
}

void loop() {
  // Handle web server requests
  handleWebRequests();
  
  // Process incoming robot responses
  processRobotResponse();
  
  // Update robot connection status
  updateRobotStatus();
  
  // Small delay to prevent watchdog issues
  delay(10);
}

void setupWiFiAP() {
  // Configure Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP_ADDRESS, AP_GATEWAY, AP_SUBNET);
  
  bool apStarted = WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  
  if (apStarted) {
    Serial.println("Access Point started successfully");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("AP MAC address: ");
    Serial.println(WiFi.softAPmacAddress());
  } else {
    Serial.println("Failed to start Access Point!");
    // Try again after delay
    delay(1000);
    ESP.restart();
  }
  
  // Print network info
  Serial.println("=== WiFi Access Point Info ===");
  Serial.println("SSID: " + String(WIFI_SSID));
  Serial.println("Password: " + String(WIFI_PASSWORD));
  Serial.println("IP: " + WiFi.softAPIP().toString());
  Serial.println("===============================");
}
