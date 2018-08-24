#include <Arduino.h>
#include "all.h"
#include "comm.h"
#include <TimeLib.h>

#define slaveAddress  0x44

void receiveEvent(size_t len);
void requestEvent(void);

boolean receivedMsg = false;


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
//
// handle Rx Event (incoming I2C request/data)
//
void receiveEvent(size_t len)
{
  int bytes = Wire.available();
  if (!bytes) return;
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
    myprintf("Received message, %d bytes available\n");
    setupSchedule();
  }
  Serial.print("Received: ");
  for (unsigned int i = 0; i < sizeof(CommData); i++)
    myprintf("%02x ", p[i]);
  Serial.println();
}

//
// handle Tx Event (outgoing I2C data)
//
void requestEvent(void)
{
  Serial.println("requestEvent...");

  Wire.write(42);

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

