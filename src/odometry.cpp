#include "odometry.h"
#include "encoder.h"
#include "config.h"
#include "ArduinoJson.h"
// ---------------- Odometry ----------------
void sendOdomPacket(unsigned long now, unsigned long dt,
                    long d1, long d2, long d3, long d4,
                    float distL, float distR, float vL, float vR) {
  // Send structured JSON over RADIO_SERIAL so the ESP8266 can parse it reliably
  StaticJsonDocument<256> doc;
  doc["resp"] = "ODOM";
  doc["t"] = now;
  doc["dt"] = dt;
  JsonArray counts = doc.createNestedArray("c");
  counts.add(d1);
  counts.add(d2);
  counts.add(d3);
  counts.add(d4);
  doc["distL"] = distL;
  doc["distR"] = distR;
  doc["vL"] = vL;
  doc["vR"] = vR;

  String out;
  serializeJson(doc, out);
  RADIO_SERIAL.println(out);
  RADIO_SERIAL.flush();

  // // Also keep a human-readable debug copy on DEBUG_SERIAL
  // DEBUG_SERIAL.print("ODOM t="); DEBUG_SERIAL.print(now);
  // DEBUG_SERIAL.print(" dt="); DEBUG_SERIAL.print(dt);
  // DEBUG_SERIAL.print(" counts="); DEBUG_SERIAL.print(d1); DEBUG_SERIAL.print(","); DEBUG_SERIAL.print(d2);
  // DEBUG_SERIAL.print(","); DEBUG_SERIAL.print(d3); DEBUG_SERIAL.print(","); DEBUG_SERIAL.print(d4);
  // DEBUG_SERIAL.print(" distL="); DEBUG_SERIAL.print(distL, 6);
  // DEBUG_SERIAL.print(" distR="); DEBUG_SERIAL.print(distR, 6);
  // DEBUG_SERIAL.print(" vL="); DEBUG_SERIAL.print(vL, 6);
  // DEBUG_SERIAL.print(" vR="); DEBUG_SERIAL.println(vR, 6);
}

void processOdometry(unsigned long &lastOdomMillis) {
  unsigned long now = millis();
  if (now - lastOdomMillis >= ODOM_MS) {
    unsigned long dt = now - lastOdomMillis;
    
    long c1, c2, c3, c4;
    resetEncoderCounts(c1, c2, c3, c4);

    float distL = (c1 + c3) * DIST_PER_TICK * 0.5;
    float distR = (c2 + c4) * DIST_PER_TICK * 0.5;

    float dt_s = dt / 1000.0;
    float vL = distL / dt_s;
    float vR = distR / dt_s;

    //sendOdomPacket(now, dt, c1, c2, c3, c4, distL, distR, vL, vR);

    lastOdomMillis = now;
  }
}
