#ifndef ODOMETRY_H
#define ODOMETRY_H

#include <Arduino.h>

void sendOdomPacket(unsigned long now, unsigned long dt,
                    long d1, long d2, long d3, long d4,
                    float distL, float distR, float vL, float vR);

void processOdometry(unsigned long &lastOdomMillis);

#endif // ODOMETRY_H
