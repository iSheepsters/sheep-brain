#include <Arduino.h>

#include "touchSensors.h"

#include "Adafruit_ZeroFFT.h"
#include "util.h"

Adafruit_MPR121 cap = Adafruit_MPR121();

#define _BV(bit)   (1 << ((uint8_t)bit))

#include "printf.h"

const uint16_t pettingBufferSize = 512;
uint16_t pettingData[numPettingSensors][pettingBufferSize];
uint16_t pettingDataPosition = 0;

uint16_t minRecentValue[numTouchSensors];
uint16_t maxRecentValue[numTouchSensors];
uint16_t stableValue[numTouchSensors];
int32_t timeTouched[numTouchSensors];
int32_t timeUntouched[numTouchSensors];
uint16_t touchedThisInterval;
unsigned long nextTouchInterval = 2000;
const uint16_t touchInterval = 1000;
const uint16_t recentInterval = 15000;
unsigned long nextRecentInterval = 2000;

unsigned long lastTouch[numTouchSensors];

boolean valid[numTouchSensors];

const uint8_t  FFI = 3;
const uint8_t  SFI = 3;
const uint8_t  ESI = 0;
const uint8_t  CDC = 16;
const uint8_t  CDT = 1;

boolean  fixed = false;

unsigned long nextTouchSample = 0;

const boolean trace = false;

uint16_t lastTouched = 0;
uint16_t currTouched = 0;
uint16_t newTouched = 0;
int16_t touchData[pettingBufferSize];

boolean isTouched(enum TouchSensor sensor) {
  return (currTouched & _BV(sensor)) != 0;
}
boolean newTouch(enum TouchSensor sensor) {
  return (newTouched & _BV(sensor)) != 0;
}


uint8_t CDTx_value(uint8_t addr) {
  uint8_t value =  cap.readRegister8(0x6C + addr / 2);
  if (addr % 2 == 0)
    return value & 0x07;
  return (value >> 4) & 0x07;
}

uint8_t CDTx_value(enum TouchSensor sensor) {
  return CDTx_value((uint8_t) sensor);
}

uint8_t CDCx_value(uint8_t sensor) {
  return cap.readRegister8(0x5f + sensor) & 0x3f;
}
uint8_t CDCx_value(enum TouchSensor sensor) {
  return CDCx_value((uint8_t) sensor);

}

void checkValid() {
  for (int t = 0; t < numTouchSensors; t++)
    valid[t] = CDTx_value(t) > 2;
}
void setupTouch() {
  Serial.println("setting up touch");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  while  (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    setupDelay(1000);
  }
  Serial.println("MPR121 found!");
  for (int t = 0; t < numTouchSensors; t++) {
    lastTouch[t] = 0;
    for (int i = 0; i < pettingBufferSize; i++) {
      pettingData[t][i] = 0;
    }
  }
  dumpConfiguration();
  Serial.println("Applying configuration");
  mySetup();
  dumpConfiguration();

  setupDelay(100);
  checkValid();
  for (int i = 0; i < numTouchSensors; i++) {
    uint16_t value = cap.filteredData(i);
    if (!valid[i])
      myprintf(Serial, "sensor %d not connected\n", i);
    else if (value > 10)
      stableValue[i] = value - 2;
  }
  nextRecentInterval = nextTouchInterval = millis() + 1000;
  Serial.println("done with touch ");

}

const uint16_t  intervalBetweenSamples = 15;
const uint16_t sampleRate = 1000 / intervalBetweenSamples;

void dumpTouchData() {
  if (true) {
    for (int i = 0; i < 6; i++) {
      Serial.print(i);
      Serial.print(" ");
      Serial.print(isTouched((TouchSensor)i) ? "t " : "- ");
      Serial.print(cap.filteredData(i));
      Serial.print(" ");
      Serial.print(stableValue[i]);
      Serial.print("  ");
      Serial.print(timeTouched[i]);
      Serial.print("  ");
      Serial.print(timeUntouched[i]);
      Serial.print("  ");
      if (i % 2 == 1)
        Serial.println();
    }

  } else
    for (int i = 0; i < 6; i++) {
      Serial.print(cap.filteredData(i) - stableValue[i]);
      Serial.print("  ");
    }
  Serial.println();
}

int16_t sensorValue(enum TouchSensor sensor) {
  int value = cap.filteredData((uint8_t) sensor);
  if (value < 10) return 0;
  return value - stableValue[sensor];
}

boolean touchThreshold(uint8_t i) {
  if (i == (uint8_t) LEFT_SENSOR || i == (uint8_t) RIGHT_SENSOR)
    return 7;
  return 15;
}

uint16_t currentValue[numTouchSensors];
void updateTouchData(unsigned long now, boolean debug) {
  lastTouched = currTouched;
  currTouched = 0;
  for (int i = 0; i < numTouchSensors; i++) if (valid[i]) {
      uint16_t value = cap.filteredData(i);
      if (value > 10) {
        currentValue[i] = value;
        if (currentValue[i] < stableValue[i] - touchThreshold(i))
          currTouched |= 1 << i;
      }
    }

  newTouched = currTouched & ~lastTouched;
  if (nextRecentInterval < now) {
    boolean allStable = true;
    int potentialMaxChange = 0;

    int maxRange = 0;
    for (int i = 0; i < numTouchSensors; i++) if (i != LEFT_SENSOR && i != RIGHT_SENSOR) {
        int range = maxRecentValue[i] - minRecentValue[i];

        if (range > 5 || minRecentValue[i] < 680)
          allStable = false;
        if (false)
          myprintf(Serial, "Range of %d: %d\n", i, range);
        maxRange = max(maxRange, range);
        potentialMaxChange = max(potentialMaxChange, abs(stableValue[i] - (minRecentValue[i] - 1)));
      }
    myprintf(Serial, "Max range = %d\n", maxRange);
    if (allStable && potentialMaxChange > 0) {
      myprintf(Serial, "All stable, change of %d, resetting\n", potentialMaxChange);
      for (int i = 0; i < numTouchSensors; i++) {
        stableValue[i] = minRecentValue[i] - 1;
      };
    } else if  (allStable && potentialMaxChange == 0) {
      Serial.println("Touch sensors stable and unchanged");
    }
    for (int i = 0; i < numTouchSensors; i++) {
      minRecentValue[i] = maxRecentValue[i] = currentValue[i];
    };
    nextRecentInterval = now + recentInterval;
  }
  else
    for (int i = 0; i < numTouchSensors; i++) {
      if (minRecentValue[i] == 0 )
        minRecentValue[i] = currentValue[i];
      if (stableValue[i] == 0)
        stableValue[i] =  currentValue[i] - 3;
      if (minRecentValue[i] > currentValue[i]  )
        minRecentValue[i] = currentValue[i] ;
      else if (maxRecentValue[i] < currentValue[i] )
        maxRecentValue[i] = currentValue[i] ;
    }

  if (nextTouchInterval < now) {
    for (int i = 0; i < numTouchSensors; i++) {
      if ((touchedThisInterval & _BV(i)) != 0) {
        timeTouched[i]++;
        timeUntouched[i] = 0;
        if (debug) Serial.print(timeTouched[i]);
      } else {
        timeTouched[i] = 0;
        timeUntouched[i]++;
        if (debug) Serial.print(-timeUntouched[i]);
      }
      if (debug) Serial.print(" ");
    }
    if (debug) Serial.println();
    touchedThisInterval = currTouched;
    nextTouchInterval = now + touchInterval;
  } else {
    touchedThisInterval |= currTouched;

  }



  if (nextTouchSample <= now) {
    nextTouchSample = now + 15;
    pettingDataPosition = pettingDataPosition + 1;
    if (pettingDataPosition >= pettingBufferSize)
      pettingDataPosition = 0;
    for (int t = 0; t < numTouchSensors; t++) {
      uint16_t v = currentValue[t] ;
      if (v < stableValue[t]) {
        lastTouch[t] = now;
      }
      if (t < numPettingSensors) {
        pettingData[t][pettingDataPosition] = v;
      }

    }
  }
}

int32_t combinedTouchDuration(enum TouchSensor sensor) {
  if (timeTouched[sensor] > 0)
    return timeTouched[sensor] * touchInterval;
  return -timeUntouched[sensor] * touchInterval;

}
int32_t touchDuration(enum TouchSensor sensor) {
  return timeTouched[sensor] * touchInterval;
}
int32_t untouchDuration(enum TouchSensor sensor) {
  return timeUntouched[sensor] * touchInterval;
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
    Serial.print(CDCx_value(i));
    Serial.print(" ");
  }
  Serial.println();
  // 6c 6d 6e 6f 70 71 72
  Serial.print("CDT: ");
  for (int i = 0; i <= 12; i++) {
    Serial.print(CDTx_value(i));
    Serial.print(" ");
  }
  Serial.println();

}
void changeMode(uint8_t newMode) {
  cap.writeRegister(MPR121_ECR, 0);
  delay(2);
  cap.writeRegister(MPR121_ECR, newMode);
  delay(2);
}


const uint8_t ELE_EN = 6;
void setECR(bool updateBaseline) {
  // 0b10111111
  // CL = 01
  // EXPROX = ELEPROX_EN
  // ELE_EN = (number of sensors to enable)
  uint8_t CL = updateBaseline ? 0x10 : 0x01;

  cap.writeRegister(MPR121_ECR, (CL << 6) | (ELEPROX_EN << 4) | ELE_EN);

}

void mySetup() {
  cap.writeRegister(MPR121_ECR, 0);
  setupDelay(10);
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
  setupDelay(10);
}


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
    float hzNext = FFT_BIN(i + 1, sampleRate, sampleSize);
    uint16_t value = touchData[i] + touchData[i + 1];
    if (trace) {
      Serial.print(hz);
      Serial.print(" Hz: ");
      Serial.println(value / ((float)avg));
    }

    if (hz > 4) break;
    if (hz > 0.5 && max < value) {
      max = value;
      answer = (touchData[i] * hz + touchData[i + 1] * hzNext) / value;
    }
  }

  if (max < 5 * avg)
    answer = 0.0;
  *confidence = max / avg;
  return answer;
}



