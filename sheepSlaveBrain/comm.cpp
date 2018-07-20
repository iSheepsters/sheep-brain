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

void setupComm() {
  // Setup for Slave mode, address 0x44, pins 18/19, external pullups, 400kHz
  Wire.begin(slaveAddress);
  Serial.print("i2c Slave address: ");
  Serial.println(slaveAddress, HEX);
  // init vars
  addr = 0;
  for (size_t i = 0; i < MEM_LEN; i++)
    mem[i] = 0;
  // register events
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);


}
//
// handle Rx Event (incoming I2C request/data)
//
void receiveEvent(int len)
{
  if (!Wire.available()) return;
  Serial.print("receive event at ");
  Serial.println(millis());
  // grab addr
  addr = Wire.read();
  while (Wire.available()) {
    uint8_t value =  Wire.read();
    if (addr <= MEM_LEN) {
      mem[addr] = value; // copy data to mem

      printf(Serial, "mem[%d] = %d\n", addr, value);
      addr++;
    }
  }
  Serial.println();
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
  printf(Serial, "Sent mem[%d] which is %d\n", addr, v);
  addr++;
}

