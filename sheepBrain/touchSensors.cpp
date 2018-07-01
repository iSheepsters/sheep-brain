
#include <Arduino.h>

#include "touchSensors.h"



Adafruit_MPR121 cap = Adafruit_MPR121();


const uint16_t pettingBufferSize = 128;
uint16_t pettingData[numPettingSensors][pettingBufferSize];
uint16_t pettingDataPosition = 0;

unsigned long lastTouch[numSensors];

uint16_t touchData[pettingBufferSize];


const uint8_t  FFI = 3;
const uint8_t  SFI = 2;
const uint8_t  ESI = 0;
const uint8_t  CDC = 16;
const uint8_t  CDT = 1;

boolean  fixed = false;

unsigned long nextTouchSample = 0;

void setupTouch() {
  for(int t = 0; t < numTouchSensors; t++) {
    lastTouch[t] = 0;
    for(int i = 0; i < pettingBufferSize; i++) {
      pettingData[t][i] = 0;
    }
  }
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
   while  (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    delay(1000);
  }
  Serial.println("MPR121 found!");
  dumpConfiguration();
  Serial.println("Applying configuration");
  mySetup();
  dumpConfiguration();
  delay(200);
  calibrate();
  dumpConfiguration();
}

uint16_t getTouchData(unsigned long now) {
  if (nextTouchSample <= now) {
    pettingDataPosition = pettingDataPosition+1;
    if (pettingDataPosition >= pettingBufferSize)
    pettingDataPosition = 0;
    for(int t = 0; t < numTouchSensors; t++) { 
      uint16_t v = cap.filteredData(t);
      uint16_t b = cap.baselineData(t);
      if (v < b) {
        lastTouch[t] = now;
      }
      if (t < numPettingSensors) {
         pettingData[t][pettingDataPosition] = v;
      }
      
    }
  }
}

void dumpData() {

  // debugging info, what
  if (true) for (uint8_t i = 0; i <= lastSensor; i++) {
      Serial.print(cap.filteredData(i)); Serial.print("\t");
    }
  if (!fixed) for (uint8_t i = 0; i <= lastSensor; i++) {
      Serial.print(cap.baselineData(i)); Serial.print("\t");
    }
  Serial.println();
}

void dumpConfiguration() {
  uint8_t r5C = cap.readRegister8(0x5C);
  uint8_t r5D = cap.readRegister8(0x5D);
  uint8_t r7B = cap.readRegister8(0x7B);
  uint8_t r7C = cap.readRegister8(0x7C);
  Serial.print("FFI: " );
  Serial.println((r5C >> 6) & 0x3);
  Serial.print("CDC: " );
  Serial.println((r5C >> 0) & 0x3f);
  Serial.print("CDT: " );
  Serial.println((r5D >> 5) & 0x7);
  Serial.print("SFI: " );
  Serial.println((r5D >> 3) & 0x3);
  Serial.print("ESI: " );
  Serial.println((r5D >> 9) & 0x7);

  Serial.print("UL: 0x");
  Serial.println(cap.readRegister8(MPR121_UPLIMIT));
  Serial.print("TL: 0x");
  Serial.println(cap.readRegister8(MPR121_TARGETLIMIT));
  Serial.print("LL: 0x");
  Serial.println(cap.readRegister8(MPR121_LOWLIMIT));

  Serial.print("OOR0: 0x");
  Serial.println(cap.readRegister8(0x02));
  Serial.print("OOR1: 0x");
  Serial.println(cap.readRegister8(0x03));

  Serial.print("ACCR0 (0x7B): 0x");
  Serial.println(r7B, HEX);
  Serial.print("ACCR1 (0x7C): 0x");
  Serial.println(r7C, HEX);

  Serial.print("USL: ");
  Serial.println(cap.readRegister8(MPR121_UPLIMIT));
  Serial.print("TL: ");
  Serial.println( cap.readRegister8(MPR121_TARGETLIMIT));
  Serial.print("LSL: ");
  Serial.println( cap.readRegister8(MPR121_LOWLIMIT));

  // 5f 60 61 62 63 64 65 66 67 68 69 6a 6b
  Serial.print("CDC: ");
  for (int i = 0; i <= 12; i++) {
    uint8_t CDCx = cap.readRegister8(0x5f + i);
    Serial.print(CDCx & 0x3f);
    Serial.print(" ");
  }
  Serial.println();
  // 6c 6d 6e 6f 70 71 72
  Serial.print("CDT: ");
  for (int i = 0; i <= 6; i++) {
    uint8_t CDTx = cap.readRegister8(0x6C + i);
    Serial.print(CDTx & 0x7);
    Serial.print(" ");
    Serial.print((CDTx >> 4) & 0x7);
    Serial.print(" ");
  }
  Serial.println();

}
void changeMode(uint8_t newMode) {
  cap.writeRegister(MPR121_ECR, 0);
  delay(10);
  cap.writeRegister(MPR121_ECR, newMode);
  delay(10);
}


void setECR(bool updateBaseline) {
  // 0b10111111
  // CL = 01
  // EXPROX = ELEPROX_EN
  // ELE_EN = 1111
  uint8_t CL = updateBaseline ? 0x10 : 0x01;

  cap.writeRegister(MPR121_ECR, (CL << 6) | (ELEPROX_EN << 4) | 0x0f);

}

void mySetup() {
  cap.writeRegister(MPR121_ECR, 0);
  delay(10);
  cap.writeRegister(MPR121_UPLIMIT, 200);
  cap.writeRegister(MPR121_TARGETLIMIT, 181);
  cap.writeRegister(MPR121_LOWLIMIT, 131);

  cap.writeRegister(MPR121_CONFIG1, (FFI << 6) | CDC); // default, 16uA charge current

  cap.writeRegister(MPR121_CONFIG2, (CDT << 5) | (SFI << 3) | ESI); // 0.5uS CDT, 4 samples, 16ms period


  // FFI = 0b00
  // RETRY = 0b01
  // BVA = 01
  // ACE = 1
  // ARE = 1

  cap.writeRegister(MPR121_AUTOCONFIG0, (FFI << 6) |  0x17);
  setECR(true);
  delay(10);
}


void adjust() {
  Serial.println("Changing to adjustable baselines");

  changeMode(0xBF);
   fixed = false;
}


void freeze() {
  Serial.println("stopping calibration");
  changeMode(0x7f);
  fixed = true;
}

uint8_t calculateBaseline(uint16_t value) {
  return  (value - 16) >> 2;
}
void calibrate() {
  uint8_t mode = cap.readRegister8(MPR121_ECR);

  cap.writeRegister(MPR121_ECR, 0);
  for (int i = 0; i <= lastSensor; i++) {
    cap.writeRegister(MPR121_BASELINE_0 + i, calculateBaseline(cap.filteredData(i)));
  }
  cap.writeRegister(MPR121_ECR, mode);
}
