#include <Arduino.h>
#include <Wire.h>
#include "comm.h"
#include "touchSensors.h"
#include "printf.h"
#include "Print.h"
#include "util.h"
#include "GPS.h"
#include "touchSensors.h"
#include "scheduler.h"

const uint8_t commAddress =  0x44;

const uint8_t rebootActivity = 255;

CommData commData;
ActivityData activityData;
unsigned long nextComm = 0;
uint8_t lastActivity = rebootActivity;

void setupComm() {
  commData.sheepNum = sheepNumber;
  addScheduledActivity(100, sendComm, "comm");
}


void getLastActivity() {

  int received = Wire.requestFrom(commAddress, sizeof(ActivityData));
  myprintf(Serial, "Requesting last activity, received %d bytes\n", received);

  uint8_t * p = (uint8_t *) & activityData;
  if (received != sizeof(sizeof(ActivityData))) {
    Serial.println("Wrong number of bytes");
    return;
  }
  for (int i = 0; i < received; i++)
    * (p++) = Wire.read();
  myprintf(Serial, "%d reboots, %d lastActivity, %d lastSubActivity\n",
           activityData. reboots,
           activityData.lastActivity,
           activityData. subActivity);
  myprintf(Serial, "%d seconds since last activity, %d seconds since slave reboot\n",
           activityData. secondsSinceLastActivity,
           activityData. secondsSinceBoot);
}

void sendBoot() {
  Serial.println("Sending boot");
  sendActivity(rebootActivity);
}

volatile uint8_t currentSubActivity;
uint8_t sendActivity(uint8_t activity) {
  lastActivity = activity;
  currentSubActivity = 0;
  Wire.beginTransmission(commAddress);
  Wire.write(57);
  Wire.write(activity);
  Wire.write(0);
  uint8_t result = Wire.endTransmission();
  return result;
}

uint8_t sendSubActivity(uint8_t subActivity) {
  currentSubActivity = subActivity;
  Wire.beginTransmission(commAddress);
  Wire.write(57);
  Wire.write(lastActivity);
  Wire.write(subActivity);
  uint8_t result =  Wire.endTransmission();
  return result;
}

void sendComm() {

  unsigned long now = millis();
  if (  commData.state != currentSheepState->state
        || commData.currTouched != currTouched
        || nextComm < now) {
    nextComm = now + 1000;
    commData.BRC_time = BRC_now();
    if (distanceFromMan < 0)
      commData.feetFromMan  = 0;
    else
      commData.feetFromMan = (uint16_t) distanceFromMan;
    commData.state = currentSheepState->state;
    uint8_t touched = currTouched;
    if (sheepNumber == 9 || sheepNumber == 13) {

      uint8_t t = swapLeftRightSensors(touched);
      //myprintf(Serial, "touch %02x -> %02x\n", touched, t);
      touched = t;
    }
    commData.currTouched = touched;
    commData.backTouchQuality = millisToSecondsCapped(qualityTime(BACK_SENSOR));
    commData.headTouchQuality = millisToSecondsCapped(qualityTime(HEAD_SENSOR));
    commData. haveFix = haveFixNow();
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
    Wire.beginTransmission(commAddress);

    Wire.write(42); // check byte
    Wire.write(p, sizeof(CommData));

    Wire.flush();
    uint8_t result =  Wire.endTransmission();
    interrupts();
    delay(1);
    return;
  }
}
