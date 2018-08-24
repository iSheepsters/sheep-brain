#include <Arduino.h>
#include <Wire.h>
#include "slave.h"
#include "printf.h"
#include "Print.h"
#include "util.h"
#include "GPS.h"
#include "touchSensors.h"

const uint8_t slaveAddress =  0x44;

CommData commData;
unsigned long nextComm = 0;

void setupComm() {
  commData.sheepNum = sheepNumber;
}

uint8_t sendComm(unsigned long now) {
  if (  commData.state != currentSheepState->state
        || commData.currTouched != currTouched
        || nextComm < now) {
    nextComm = now + 5000;
    commData. BRC_time = BRC_now();
    if (distanceFromMan < 0)
      commData.feetFromMan  = 0;
    else
      commData.feetFromMan = (uint16_t) distanceFromMan;
    commData.state = currentSheepState->state;

    commData.currTouched = currTouched;
    commData.when = Night;
    uint8_t * p = (uint8_t *)&commData;
    if (false) {
      Serial.print("Sending ");
      for (int i = 0; i < sizeof(CommData); i++) {
        myprintf(Serial, "%02x ", p[i]);
      }
      Serial.println();
    }

    noInterrupts();
    Wire.beginTransmission(slaveAddress);
    //Wire.write(42); // check byte
    Wire.write(p, sizeof(CommData));

    Wire.flush();
    uint8_t result =  Wire.endTransmission();
    interrupts();
    delay(1);
    return result;
  }
}



