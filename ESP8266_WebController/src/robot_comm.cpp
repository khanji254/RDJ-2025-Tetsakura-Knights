#include "robot_comm.h"
#include "esp_config.h"

RobotStatus robotStatus = {
  .connected = false,
  .lastResponse = 0,
  .lastOdometry = "",
  .motorsEnabled = true,
  .currentSpeed = DEFAULT_SPEED
};

String rxBuffer = "";

void setupRobotCommunication() {
  Serial.begin(MEGA_SERIAL_BAUD);
  Serial.println("ESP8266 Robot Controller Initialized");
  
  // Send initial enable command to robot
  delay(2000); // Wait for Mega to boot
  sendCommandToRobot("ENABLE");
}

void sendCommandToRobot(String command) {
  Serial.println(command);
  Serial.flush();
  
  // Only update speed tracking locally, motor status will come from robot response
  if (command == "STOP") {
    robotStatus.currentSpeed = 0;
  } else if (command.startsWith("FWD") || command.startsWith("BACK") || 
             command.startsWith("LEFT") || command.startsWith("RIGHT")) {
    // Extract speed from command if present
    int spaceIndex = command.indexOf(' ');
    if (spaceIndex > 0) {
      robotStatus.currentSpeed = command.substring(spaceIndex + 1).toInt();
    }
  }
  
  Serial.printf("Sent to robot: %s\n", command.c_str());
}

void processRobotResponse() {
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\r') continue;
    
    if (c == '\n') {
      if (rxBuffer.length() > 0) {
        handleRobotMessage(rxBuffer);
        rxBuffer = "";
      }
    } else {
      rxBuffer += c;
      
      // Prevent buffer overflow
      if (rxBuffer.length() > MAX_COMMAND_LENGTH) {
        rxBuffer = "";
      }
    }
  }
}

void handleRobotMessage(String message) {
  robotStatus.lastResponse = millis();
  robotStatus.connected = true;
  
  Serial.printf("Received from robot: %s\n", message.c_str());
  
  if (message.startsWith("ODOM")) {
    robotStatus.lastOdometry = message;
  } else if (message.startsWith("OK")) {
    // Command acknowledged - update status based on response
    if (message.indexOf("ENABLE") >= 0) {
      robotStatus.motorsEnabled = true;
      Serial.println("Motors enabled confirmed by robot");
    } else if (message.indexOf("DISABLE") >= 0) {
      robotStatus.motorsEnabled = false;
      Serial.println("Motors disabled confirmed by robot");
    }
    Serial.printf("Robot acknowledged: %s\n", message.c_str());
  } else if (message.startsWith("ERR")) {
    // Command error
    Serial.printf("Robot error: %s\n", message.c_str());
  }
}

bool waitForRobotResponse(unsigned long timeout) {
  unsigned long startTime = millis();
  
  while (millis() - startTime < timeout) {
    if (Serial.available()) {
      processRobotResponse();
      if (millis() - robotStatus.lastResponse < 100) {
        return true;
      }
    }
    yield(); // Allow ESP8266 to handle WiFi tasks
  }
  
  return false;
}

void updateRobotStatus() {
  unsigned long now = millis();
  
  // Check if robot is still connected (no response in last 5 seconds)
  if (now - robotStatus.lastResponse > COMMAND_TIMEOUT_MS) {
    if (robotStatus.connected) {
      robotStatus.connected = false;
      Serial.println("Robot connection lost");
    }
  }
  
  // Request odometry periodically
  static unsigned long lastOdomRequest = 0;
  if (now - lastOdomRequest > STATUS_UPDATE_INTERVAL_MS) {
    requestOdometry();
    lastOdomRequest = now;
  }
}

void requestOdometry() {
  sendCommandToRobot("REQ_ODOM");
}
