#include "encoder.h"
#include "config.h"

// ---------------- Encoder Variables ----------------
volatile long encCount1 = 0, encCount2 = 0, encCount3 = 0, encCount4 = 0;

// ---------------- Encoder ISRs ----------------
void ISR_enc1() { 
  bool b = digitalRead(ENC1_B_PIN); 
  if (b) encCount1++; 
  else encCount1--; 
}

void ISR_enc2() { 
  bool b = digitalRead(ENC2_B_PIN); 
  if (b) encCount2++; 
  else encCount2--; 
}

void ISR_enc3() { 
  bool b = digitalRead(ENC3_B_PIN); 
  if (b) encCount3++; 
  else encCount3--; 
}

void ISR_enc4() { 
  bool b = digitalRead(ENC4_B_PIN); 
  if (b) encCount4++; 
  else encCount4--; 
}

void resetEncoderCounts(long &c1, long &c2, long &c3, long &c4) {
  noInterrupts();
  c1 = encCount1; encCount1 = 0;
  c2 = encCount2; encCount2 = 0;
  c3 = encCount3; encCount3 = 0;
  c4 = encCount4; encCount4 = 0;
  interrupts();
}

void initializeEncoders() {
  // Encoders
  pinMode(ENC1_A_PIN, INPUT_PULLUP); 
  pinMode(ENC1_B_PIN, INPUT_PULLUP);
  pinMode(ENC2_A_PIN, INPUT_PULLUP); 
  pinMode(ENC2_B_PIN, INPUT_PULLUP);
  pinMode(ENC3_A_PIN, INPUT_PULLUP); 
  pinMode(ENC3_B_PIN, INPUT_PULLUP);
  pinMode(ENC4_A_PIN, INPUT_PULLUP); 
  pinMode(ENC4_B_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC1_A_PIN), ISR_enc1, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC2_A_PIN), ISR_enc2, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC3_A_PIN), ISR_enc3, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC4_A_PIN), ISR_enc4, RISING);
}

// ---------------- Encoder Reading Functions ----------------
long getM1Encoder() {
  long temp;
  noInterrupts();
  temp = encCount1;
  interrupts();
  return temp;
}

long getM2Encoder() {
  long temp;
  noInterrupts();
  temp = encCount2;
  interrupts();
  return temp;
}

long getM3Encoder() {
  long temp;
  noInterrupts();
  temp = encCount3;
  interrupts();
  return temp;
}

long getM4Encoder() {
  long temp;
  noInterrupts();
  temp = encCount4;
  interrupts();
  return temp;
}
