#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

extern volatile long encCount1, encCount2, encCount3, encCount4;

void initializeEncoders();
void ISR_enc1();
void ISR_enc2();
void ISR_enc3();
void ISR_enc4();
void resetEncoderCounts(long &c1, long &c2, long &c3, long &c4);

// Encoder reading functions
long getM1Encoder();
long getM2Encoder();
long getM3Encoder();
long getM4Encoder();

#endif // ENCODER_H
