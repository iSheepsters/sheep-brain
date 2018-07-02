
#include <Arduino.h>

#include "touchSensors.h"

#include "Adafruit_ZeroFFT.h"

Adafruit_MPR121 cap = Adafruit_MPR121();

#include "printf.h"


const uint16_t pettingBufferSize = 512;
uint16_t pettingData[numPettingSensors][pettingBufferSize];
uint16_t pettingDataPosition = 0;

unsigned long lastTouch[numTouchSensors];

const uint8_t  FFI = 3;
const uint8_t  SFI = 2;
const uint8_t  ESI = 0;
const uint8_t  CDC = 16;
const uint8_t  CDT = 1;

boolean  fixed = false;

unsigned long nextTouchSample = 0;

void setupTouch() {
  for (int t = 0; t < numTouchSensors; t++) {
    lastTouch[t] = 0;
    for (int i = 0; i < pettingBufferSize; i++) {
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
  delay(200);
  freeze();
}

const boolean trace = false;
int16_t touchData[pettingBufferSize];

const uint16_t  intervalBetweenSamples = 15;
const uint16_t sampleRate = 1000 / intervalBetweenSamples;
float detectPetting(uint8_t touchSensor, uint16_t sampleSize, float * confidence) {
  if (touchSensor >= numPettingSensors) return 0;


  uint16_t i = 0;
  int16_t pos = pettingDataPosition - sampleSize;
  if (pos < 0) pos += pettingBufferSize;
  uint32_t total = 0;
  while (i < sampleSize) {
    uint16_t v = pettingData[touchSensor][pos];
    touchData[i] = v;
    total += v;
    pos++;
    if (pos >= pettingBufferSize)
      pos = 0;
    i++;
  }
  uint16_t avg = total / sampleSize;
  //  printf(Serial, "avg = %d\n", avg);
  uint16_t max = 10;
  for (int i = 0; i < sampleSize; i++) {
    int16_t v = touchData[i] - avg;
    touchData[i] = v;
    //     Serial.println(touchData[i]);
    if (max < v)
      max = v;
    else if (max < -v)
      max = -v;
  }
  for (int i = 0; i < sampleSize; i++) {
    touchData[i] =  (((int32_t)touchData[i]) * 10000) / max;

  }
  ZeroFFT(touchData, sampleSize);
  total = 0;
  for (int i = 0; i < sampleSize / 2; i++) {
    total += touchData[i];
  }
  avg = total / sampleSize;
  if (trace) {
    Serial.print("avg: ");
    Serial.println(avg);
  }

  if (avg < 50)
    avg = 50;
  float answer = 0.0;
  max = avg;
  for (int i = 1; i < sampleSize / 2; i++) {
    float hz = FFT_BIN(i, sampleRate, sampleSize);
    float hzNext = FFT_BIN(i+1, sampleRate, sampleSize);
    uint16_t value = touchData[i] + touchData[i+1];
    if (trace) {
      Serial.print(hz);
      Serial.print(" Hz: ");
      Serial.println(value / ((float)avg));
    }

    if (hz > 4) break;
    if (hz > 0.5 && max < value) {
      max = value;
      answer = (touchData[i]*hz + touchData[i+1]*hzNext)/value;
    }
  }

  if (max < 5 * avg)
    answer = 0.0;
  *confidence = max / avg;
  return answer;
}


void updateTouchData(unsigned long now) {
  if (nextTouchSample <= now) {
    nextTouchSample = now + 15;
    pettingDataPosition = pettingDataPosition + 1;
    if (pettingDataPosition >= pettingBufferSize)
      pettingDataPosition = 0;
    for (int t = 0; t < numTouchSensors; t++) {
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
  if (true) for (uint8_t i = 0; i <= lastTouchSensor; i++) {
      Serial.print(cap.filteredData(i)); Serial.print("\t");
    }
  if (!fixed) for (uint8_t i = 0; i <= lastTouchSensor; i++) {
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
  for (int i = 0; i <= lastTouchSensor; i++) {
    cap.writeRegister(MPR121_BASELINE_0 + i, calculateBaseline(cap.filteredData(i)));
  }
  cap.writeRegister(MPR121_ECR, mode);
}
