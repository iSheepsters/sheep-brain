
#include <Wire.h>
#include "comm.h"

const uint8_t slaveAddress =  0x44;

void setupComm() {
 
  
}
uint8_t sendSlave(uint8_t addr, uint8_t value) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(addr);
  Wire.write(value);
  return Wire.endTransmission();
}

