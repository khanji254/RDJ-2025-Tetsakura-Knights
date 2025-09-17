#include "command_parser.h"
#include "motor_control.h"
#include "config.h"

// ---------------- Command parser ----------------
void processLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  char buf[line.length() + 1];
  line.toCharArray(buf, sizeof(buf));
  char *tok = strtok(buf, " ");

  if (!tok) return;

  if (strcmp(tok, "SET_V") == 0) {
    char *a = strtok(NULL, " ");
    char *b = strtok(NULL, " ");
    if (a && b) {
      int L = atoi(a), R = atoi(b);
      setM1(L); setM3(L); setM2(R); setM4(R);
      RADIO_SERIAL.println("OK SET_V");
    } else RADIO_SERIAL.println("ERR SET_V params");

  } else if (strcmp(tok, "MALL") == 0) {
    char *a = strtok(NULL, " ");
    char *b = strtok(NULL, " ");
    char *c = strtok(NULL, " ");
    char *d = strtok(NULL, " ");
    if (a && b && c && d) {
      setM1(atoi(a)); setM2(atoi(b)); setM3(atoi(c)); setM4(atoi(d));
      RADIO_SERIAL.println("OK MALL");
    } else RADIO_SERIAL.println("ERR MALL params");

  } else if (strcmp(tok, "M1") == 0) {
    char *a = strtok(NULL, " ");
    if (a) { setM1(atoi(a)); RADIO_SERIAL.println("OK M1"); }

  } else if (strcmp(tok, "M2") == 0) {
    char *a = strtok(NULL, " ");
    if (a) { setM2(atoi(a)); RADIO_SERIAL.println("OK M2"); }

  } else if (strcmp(tok, "M3") == 0) {
    char *a = strtok(NULL, " ");
    if (a) { setM3(atoi(a)); RADIO_SERIAL.println("OK M3"); }

  } else if (strcmp(tok, "M4") == 0) {
    char *a = strtok(NULL, " ");
    if (a) { setM4(atoi(a)); RADIO_SERIAL.println("OK M4"); }

  } else if (strcmp(tok, "FWD") == 0) {
    char *a = strtok(NULL, " ");
    int spd = a ? atoi(a) : 150;
    driveForward(spd);
    RADIO_SERIAL.println("OK FWD");

  } else if (strcmp(tok, "BACK") == 0) {
    char *a = strtok(NULL, " ");
    int spd = a ? atoi(a) : 150;
    driveBackward(spd);
    RADIO_SERIAL.println("OK BACK");

  } else if (strcmp(tok, "LEFT") == 0) {
    char *a = strtok(NULL, " ");
    int spd = a ? atoi(a) : 150;
    turnLeft(spd);
    RADIO_SERIAL.println("OK LEFT");

  } else if (strcmp(tok, "RIGHT") == 0) {
    char *a = strtok(NULL, " ");
    int spd = a ? atoi(a) : 150;
    turnRight(spd);
    RADIO_SERIAL.println("OK RIGHT");

  } else if (strcmp(tok, "STOP") == 0) {
    stopAll();
    RADIO_SERIAL.println("OK STOP");

  } else if (strcmp(tok, "ENABLE") == 0) {
    enableMotors();
    RADIO_SERIAL.println("OK ENABLE");

  } else if (strcmp(tok, "DISABLE") == 0) {
    disableMotors();
    RADIO_SERIAL.println("OK DISABLE");

  } else if (strcmp(tok, "REQ_ODOM") == 0) {
    RADIO_SERIAL.println("OK REQ_ODOM");
    // Will emit next cycle

  } else {
    RADIO_SERIAL.print("ERR UNKNOWN_CMD ");
    RADIO_SERIAL.println(tok);
  }
}

void handleSerialCommands(String &rxBuf) {
  // Parse commands from ESP
  while (RADIO_SERIAL.available()) {
    char c = RADIO_SERIAL.read();
    if (c == '\r') continue;
    if (c == '\n') {
      String L = rxBuf; 
      rxBuf = "";
      processLine(L);
    } else {
      rxBuf += c;
      if (rxBuf.length() > 200) rxBuf = "";
    }
  }
}
