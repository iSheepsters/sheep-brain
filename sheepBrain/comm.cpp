#include <Arduino.h>
#include <Wire.h>
#include "comm.h"
#include "printf.h"
#include "Print.h"

const uint8_t slaveAddress =  0x44;
#define MEM_LEN 10
uint8_t slaveMemory[MEM_LEN];

unsigned long lastSent[MEM_LEN];
void setupComm() {
  for (int i = 0; i < MEM_LEN; i++) {
    slaveMemory[i] = 0;
    lastSent[i] = 0;
  }
}
uint8_t sendSlave(uint8_t addr, uint8_t value) {
  unsigned long now = millis();
  if (slaveMemory[addr] == value && lastSent[addr] > 0 && lastSent[addr] + 10000 > now)
    return 0;
  lastSent[addr] = now;
  slaveMemory[addr] = value;

  myprintf(Serial, "mem[%d] <- %d\n", addr, value);
  Wire.beginTransmission(slaveAddress);
  Wire.write(42); // check byte
  Wire.write(addr);
  Wire.write(value);
  uint8_t result =  Wire.endTransmission();
  delay(2);
  return result;
}


