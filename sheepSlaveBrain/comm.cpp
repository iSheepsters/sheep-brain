#include <Arduino.h>
#include "all.h"
#include "comm.h"
#include <TimeLib.h>

const uint8_t slaveAddress =  0x44;

const uint8_t rebootStarted = 255;
const uint8_t rebootActivity = 254;
const uint8_t slaveRebootStarted = 253;

void receiveEvent(size_t len);
void requestEvent(void);

volatile boolean receivedMsg = false;

unsigned long lastActivityAt = 0;

struct __attribute__ ((packed)) ActivityData {
  uint16_t secondsSinceLastActivity;
  uint16_t secondsSinceBoot;
  uint8_t lastActivity = slaveRebootStarted;
  uint8_t subActivity = 0;
  uint8_t reboots = 0;
};

ActivityData activityData;

CommData commData;

void setupComm() {
  // Setup for Slave mode, address 0x44, pins 18/19, external pullups, 400kHz
  Wire.begin(I2C_SLAVE, 0x44, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  Wire.setDefaultTimeout(100); // 100 usecs timeout
  myprintf("i2c Slave address: 0x%02x\n", slaveAddress);
  // init vars

  // register events
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

}
unsigned long activityReports = 0;
//
// handle Rx Event (incoming I2C request/data)
//
void receiveEvent(size_t len)
{
  uint8_t kind = Wire.read();
  if (kind == 42) {
    State oldState = commData.state;

    uint8_t * p = (uint8_t *)&commData;
    unsigned int bytesRead = Wire.read(p, sizeof(CommData));
    if (bytesRead != sizeof(CommData)) {
      myprintf("Only read %d bytes\n", bytesRead);
      return;
    }
    if (oldState != commData.state)
      myprintf("commData.state = %d\n", commData.state);
    setTime(commData.BRC_time);
    if (!receivedMsg && year() == 2018 && animationsSetUp) {
      receivedMsg = true;
      myprintf("Received message and animations setup\n");
      setupSchedule();
    }
    if (false) {
      Serial.print("Received: ");
      for (unsigned int i = 0; i < sizeof(CommData); i++)
        myprintf("%02x ", p[i]);
      Serial.println();
    }
  } else if (kind == 57) {

    uint8_t thisActivity = Wire.read();
    uint8_t thisSubActivity = Wire.read();

    activityReports++;
    if (activityData.lastActivity != slaveRebootStarted
        && thisActivity == rebootStarted)  {
      if (activityData.reboots < 255)
        activityData.reboots++;
    } else {
      activityData.lastActivity = thisActivity;
      activityData.subActivity = thisSubActivity;
      if (thisActivity != rebootActivity)
        activityData.reboots = 0;
      lastActivityAt = now();
    }
    if (activityReports < 300)
      myprintf( "activity %d.%d, %d reboots\n", activityData.lastActivity,
                activityData.subActivity, activityData.reboots);
  } else {
    myprintf("Got unknown i2c message, kind %d, %d bytes\n",
             kind, Wire.available());
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
