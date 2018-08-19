#include <Arduino.h>
#include "all.h"
#include "comm.h"
#include <TimeLib.h>

#define slaveAddress  0x44

//struct CommData {
//  time_t time;
//  uint8_t state;
//  uint8_t ledMode;
//
//  //6 x Touch values (negative = touched)
//  // Touched time
//  //Untouched time
//};

size_t addr;
// Memory
uint8_t mem[MEM_LEN];

void receiveEvent(size_t len);
void requestEvent(void);

boolean receivedMsg = false;


void setupComm() {
  // Setup for Slave mode, address 0x44, pins 18/19, external pullups, 400kHz
  Wire.begin(I2C_SLAVE, 0x44, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  Serial.print("i2c Slave address: ");
  Serial.println(slaveAddress, HEX);
  // init vars
  addr = 0;
  for (int i = 0; i < MEM_LEN; i++)
    mem[i] = 0;
  // register events
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);


}
//
// handle Rx Event (incoming I2C request/data)
//
void receiveEvent(size_t len)
{
  if (!Wire.available()) return;
  if (!receivedMsg) {
    receivedMsg = true;
    Serial.println("Received message");
  }
  // grab check byte
  uint8_t check = Wire.read();
  if (check != 42) {
    myprintf(Serial, "got bad check byte of %d, discarding rest of data\n");
    while (Wire.available()) {
      myprintf(Serial, "discarding %d\n", Wire.read());
    }
    Serial.println();
    return;
  }

  addr = Wire.read();
  //myprintf(Serial, "addr of %d, %d bytes available\n", addr, Wire.available());
  while (Wire.available()) {
    uint8_t value =  Wire.read();
    if (addr <= MEM_LEN) {
      mem[addr] = value; // copy data to mem
      //myprintf(Serial, "mem[%d] = %d\n", addr, value);
      addr++;
    }
  }
}

//
// handle Tx Event (outgoing I2C data)
//
void requestEvent(void)
{
  Serial.println("requestEvent...");
  uint8_t v = 0;
  if (addr < MEM_LEN)
    v = mem[addr];

  Wire.write(v);
  myprintf(Serial, "Sent mem[%d] which is %d\n", addr, v);
  addr++;
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
