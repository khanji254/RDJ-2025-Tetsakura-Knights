#ifndef ROBOT_COMM_H
#define ROBOT_COMM_H

#include <Arduino.h>

struct RobotStatus {
  bool connected;
  unsigned long lastResponse;
  String lastOdometry;
  bool motorsEnabled;
  int currentSpeed;
};

extern RobotStatus robotStatus;

void setupRobotCommunication();
void sendCommandToRobot(String command);
void processRobotResponse();
void handleRobotMessage(String message);
bool waitForRobotResponse(unsigned long timeout = 1000);
void updateRobotStatus();
void requestOdometry();

#endif // ROBOT_COMM_H
