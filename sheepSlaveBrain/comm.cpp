#include <Arduino.h>
#include "all.h"
#include "comm.h"
#include <TimeLib.h>

const uint8_t slaveAddress =  0x44;
const uint8_t rebootActivity = 255;
const uint8_t slaveRebootActivity = 254;

void receiveEvent(size_t len);
void requestEvent(void);

volatile boolean receivedMsg = false;

unsigned long lastActivityAt = 0;

struct __attribute__ ((packed)) ActivityData {
  uint16_t secondsSinceLastActivity;
  uint16_t secondsSinceBoot;
  uint8_t lastActivity = slaveRebootActivity;
  uint8_t subActivity = 0;
  uint8_t reboots = 0;
};

ActivityData activityData;

CommData commData;

void setupComm() {
  // Setup for Slave mode, address 0x44, pins 18/19, external pullups, 400kHz
  Wire.begin(I2C_SLAVE, 0x44, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  myprintf("i2c Slave address: 0x%02x\n", slaveAddress);
  // init vars

  // register events
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);


}
int activityReports = 0;
//
// handle Rx Event (incoming I2C request/data)
//
void receiveEvent(size_t len)
{
  int bytes = Wire.available();
  //if (!bytes) return;
  uint8_t kind = Wire.read();
  if (kind == 42) {
    State oldState = commData.state;

    uint8_t * p = (uint8_t *)&commData;
    for (unsigned int i = 0; i < sizeof(CommData); i++) {
      p[i] =  Wire.read();
    }
    if (oldState != commData.state)
      myprintf("commData.state = %d\n", commData.state);
    setTime(commData.BRC_time);
    if (!receivedMsg && year() == 2018) {
      receivedMsg = true;
      myprintf("Received message\n");
      setupSchedule();
    }
    if (false) {
      Serial.print("Received: ");
      for (unsigned int i = 0; i < sizeof(CommData); i++)
        myprintf("%02x ", p[i]);
      Serial.println();
    }
  } else if (kind == 57) {
    if (activityReports < 10)
      Serial.println("Got activity report\n");

    uint8_t thisActivity = Wire.read();
    uint8_t thisSubActivity = Wire.read();
    if (activityReports < 10)
      myprintf( "activity %d.%d\n", thisActivity, thisSubActivity);
    activityReports++;
    if (activityData.lastActivity != slaveRebootActivity
        && thisActivity == rebootActivity)  {
      if (activityData.reboots < 255)
        activityData.reboots++;
    } else {
      activityData.lastActivity = thisActivity;
      activityData.subActivity = thisSubActivity;
      activityData.reboots = 0;
      lastActivityAt = now();
    }
  } else {
    myprintf("Got unknown i2c message, kind %d, %d bytes\n",
             kind, Wire.available());
    while (Wire.available() > 0)
      Wire.read();
  }
}


uint16_t secondsBetween(unsigned long start, unsigned long end) {
  unsigned long diff = (end - start) / 1000;
  if (diff > 65535) return 65535;
  return diff;
}
//
// handle Tx Event (outgoing I2C data)
//
void requestEvent(void)
{
  Serial.println("requestEvent...");
  unsigned long now = millis();
  activityData.secondsSinceLastActivity = secondsBetween(lastActivityAt, now);
  activityData.secondsSinceBoot = secondsBetween(0, now);
  uint8_t * p = (uint8_t *)&activityData;
  Wire.write(p, sizeof(ActivityData));
  Wire.flush();
  Wire.endTransmission();
}


void print_i2c_status(void)
{
  switch (Wire.status())
  {
    case I2C_WAITING:
      //Serial.print("I2C waiting, no errors\n");
      break;
    case I2C_ADDR_NAK: Serial.print("Slave addr not acknowledged\n"); break;
    case I2C_DATA_NAK: Serial.print("Slave data not acknowledged\n"); break;
    case I2C_ARB_LOST: Serial.print("Bus Error: Arbitration Lost\n"); break;
    default:           Serial.print("I2C busy\n"); break;
  }
}
