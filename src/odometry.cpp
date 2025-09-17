#include "odometry.h"
#include "encoder.h"
#include "config.h"

// ---------------- Odometry ----------------
void sendOdomPacket(unsigned long now, unsigned long dt,
                    long d1, long d2, long d3, long d4,
                    float distL, float distR, float vL, float vR) {
  RADIO_SERIAL.print("ODOM ");
  RADIO_SERIAL.print(now); RADIO_SERIAL.print(' ');
  RADIO_SERIAL.print(dt); RADIO_SERIAL.print(' ');
  RADIO_SERIAL.print(d1); RADIO_SERIAL.print(' ');
  RADIO_SERIAL.print(d2); RADIO_SERIAL.print(' ');
  RADIO_SERIAL.print(d3); RADIO_SERIAL.print(' ');
  RADIO_SERIAL.print(d4); RADIO_SERIAL.print(' ');
  RADIO_SERIAL.print(distL, 6); RADIO_SERIAL.print(' ');
  RADIO_SERIAL.print(distR, 6); RADIO_SERIAL.print(' ');
  RADIO_SERIAL.print(vL, 6); RADIO_SERIAL.print(' ');
  RADIO_SERIAL.println(vR, 6);
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

    sendOdomPacket(now, dt, c1, c2, c3, c4, distL, distR, vL, vR);

    lastOdomMillis = now;
  }
}
