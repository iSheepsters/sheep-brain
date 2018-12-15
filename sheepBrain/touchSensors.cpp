#include <Arduino.h>

#include "touchSensors.h"
#include "logging.h"
#include "state.h"
#include "scheduler.h"
#include "sound.h"
#include "comm.h"
#include "tysons.h"
#include "state.h"

#include "Adafruit_ZeroFFT.h"
#include "util.h"

Adafruit_MPR121 cap = Adafruit_MPR121();

uint16_t STABLE_VALUE = 0;
const bool AUTO_CONFIGURE = false;

#define _BV(bit)   (1 << ((uint8_t)bit))

#include "printf.h"

const uint16_t pettingBufferSize = 512;
uint16_t pettingData[numPettingSensors][pettingBufferSize];
uint16_t pettingDataPosition = 0;

unsigned long nextPettingReport;


boolean debugTouch = false;

boolean plotTouch = false;

uint16_t minRecentValue[numTouchSensors];
uint16_t maxRecentValue[numTouchSensors];
uint16_t stableValue[numTouchSensors];
int32_t timeTouched[numTouchSensors];
int32_t timeUntouched[numTouchSensors];
int32_t qualityTimeIntervals[numTouchSensors];
unsigned long firstTouchThisInterval[numTouchSensors];
uint16_t touchedThisInterval;
unsigned long nextTouchInterval = 2000;
unsigned long lastTouchInterval = 0;
const uint16_t touchInterval = 1000;
const uint16_t resetSensorsInterval = 15000;
unsigned long nextSensorResetInterval = 0;

unsigned long lastTouch[numTouchSensors];

boolean valid[numTouchSensors];

const uint8_t  FFI = 3;
const uint8_t  SFI = 3;
const uint8_t  ESI = 0;
const uint8_t  CDC = 63; // 54;
const uint8_t  CDT = 4;

boolean fixed = false;

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


uint16_t stableV(int i) {
  if (STABLE_VALUE != 0) return STABLE_VALUE;
  return stableValue[i];
}

uint8_t swapLeftRightSensors(uint8_t touched) {
  boolean leftTouched = (touched & _BV(LEFT_SENSOR)) != 0;
  boolean rightTouched = (touched & _BV(RIGHT_SENSOR)) != 0;
  touched = touched & 0xf3;
  if (leftTouched)
    touched |= _BV(RIGHT_SENSOR);
  if (rightTouched)
    touched |= _BV(LEFT_SENSOR);
  return touched;
}

uint8_t CDTx_value(uint8_t addr) {
  uint8_t value = cap.readRegister8(0x6C + addr / 2);
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
    valid[t] = true;  // CDTx_value(t) > 2;
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
  Serial.println("Applying configuration");
  mySetup();
  setupDelay(100);
  dumpConfiguration();
  checkValid();
  for (int i = 0; i < numTouchSensors; i++) {
    uint16_t value = cap.filteredData(i);
    if (!valid[i])
      myprintf(Serial, "sensor %d not connected\n", i);
    else if (value > 10)
      stableValue[i] = value - 1;
    firstTouchThisInterval[i] = 0;
    myprintf(Serial, "sensor %d = %d\n", i, value);

  }
  nextSensorResetInterval = nextTouchInterval = millis() + 1000;
  nextPettingReport = millis() + 2000;
  addScheduledActivity(50, updateTouchData, "touch");
  Serial.println("done with touch setup");
}

const uint16_t  intervalBetweenSamples = 15;
const uint16_t sampleRate = 1000 / intervalBetweenSamples;

void dumpTouchData(int i) {
  Serial.print(i);
  Serial.print(" ");
  Serial.print(isTouched((TouchSensor)i) ? "t " : "- ");
  Serial.print(cap.filteredData(i));
  Serial.print(" ");
  Serial.print(stableV(i));
  Serial.print("  ");
  Serial.print(timeTouched[i]);
  Serial.print("  ");
  Serial.println(timeUntouched[i]);
}
void dumpTouchData() {
  if (MALL_SHEEP) {
    dumpTouchData(WHOLE_BODY_SENSOR);
    return;
  }
  if (true) {
    for (int i = 0; i < 6; i++)
      dumpTouchData(i);
  } else
    for (int i = 0; i < 6; i++) {
      Serial.print(cap.filteredData(i) - stableV(i));
      Serial.print("  ");
    }
  Serial.println();
}

void wasTouchedInappropriately() {
  qualityTimeIntervals[BACK_SENSOR] = 0;
  qualityTimeIntervals[HEAD_SENSOR] = 0;
  qualityTimeIntervals[RUMP_SENSOR] = 0;

}

uint8_t touchThreshold(uint8_t i) {
  enum TouchSensor sensor = (TouchSensor) i;
  switch (sensor) {
    case HEAD_SENSOR:
      return 3;
    case LEFT_SENSOR:
    case RIGHT_SENSOR:
      return 5;
    case PRIVATES_SENSOR:
      return 10;
    case WHOLE_BODY_SENSOR:
      return 40;
    case BACK_SENSOR:
    case RUMP_SENSOR:
    default:
      return 10;
  }
}

int16_t sensorValue(enum TouchSensor sensor) {
  int value = cap.filteredData((uint8_t) sensor);
  if (value < 10) return 0;
  return value - ( stableValue[sensor] - touchThreshold((uint8_t)sensor));
}

unsigned long lastReset = 20000;

boolean isStable(int sensor) {
  int range = maxRecentValue[sensor] - minRecentValue[sensor];
  int maxRange = MALL_SHEEP ? 30 : 5;
  unsigned long secondsSinceLastReset = (millis() - lastReset) / 1000;
  if (lastReset > 5 * 60)
    maxRange = 10;
  return range <= maxRange;
}

void resetStableValue(int sensor) {

  if (isStable(sensor)) {
    stableValue[sensor] = minRecentValue[sensor] - 1;
  }
  else
    myprintf(Serial, "Asked to reset %d, but it isn't stable\n", sensor);
  if (MALL_SHEEP && sensor == WHOLE_BODY_SENSOR && stableValue[sensor] > 720)
    stableValue[sensor] = 720;
}


uint16_t resetCount = 0;
void considerResettingTouchSensors() {
  boolean allStable = true;
  int potentialMaxChange = 0;
int firstSensor = MALL_SHEEP ? 7 : 0;
  sendSubActivity(10);
  for (int i = firstSensor; i < numTouchSensors; i++) {
    if (i != LEFT_SENSOR && i != RIGHT_SENSOR && !isStable(i))
      allStable = false;
    potentialMaxChange = max(potentialMaxChange, abs(stableV(i) - (minRecentValue[i] - 1)));
  }

  sendSubActivity(11);
  if (!musicPlayer.playingMusic) {
    noInterrupts();
    if (allStable)
      logFile.print("S,");
    else
      logFile.print("U,");
    logFile.print(potentialMaxChange);
    logFile.print(",  ");
    sendSubActivity(21);
    int start = MALL_SHEEP ? 7 : 0;
    for (int i = firstSensor; i < numTouchSensors; i++) {
      logFile.print(minRecentValue[i]);
      logFile.print(",");

      logFile.print(maxRecentValue[i]);
      logFile.print(",");
      logFile.print(stableV(i));
      logFile.print(",");
      logFile.print(combinedTouchDuration((enum TouchSensor)i) / 1000);
      logFile.print(", ");
    }
    sendSubActivity(22);
    logFile.println();
    interrupts();

    if (false) {
      sendSubActivity(12);
      optionalFlush();
    }
  }


  if (allStable || resetCount == 0) {
    sendSubActivity(13);
    resetCount++;
    if (potentialMaxChange > 0) {
      lastReset = millis();
      if (printInfo()) {
        if (allStable)
          myprintf(Serial, "All stable, change of %d, reset #%d\n", potentialMaxChange, resetCount);
        else
          myprintf(Serial, "First reset, change of %d\n", potentialMaxChange);
      }

      for (int i = 0; i < numTouchSensors; i++) {
        resetStableValue(i);
      };
    } else if  (potentialMaxChange == 0) {
      Serial.println("Touch sensors stable and unchanged");
    }
  } else {
    sendSubActivity(14);
    for (int i = 0; i < numTouchSensors; i++)  if (isStable(i)) {
        int touchSeconds = touchDuration((enum TouchSensor) i);
        if (touchSeconds > 5 * 60 && maxRecentValue[i] < stableV(i)) {
          myprintf(Serial, "Sensor %d stable, touched for %d seconds, recent values %d - %d, stableValue %d, resetting\n",
                   i, touchSeconds, minRecentValue[i], maxRecentValue[i], stableV(i));
          resetStableValue(i);
        }
      }
  }

  sendSubActivity(15);
  for (int i = 0; i < numTouchSensors; i++) {
    minRecentValue[i] = maxRecentValue[i] = currentValue[i];
  };

}
uint16_t currentValue[numTouchSensors];

uint16_t tripValue(int i) {
  int16_t threshold = touchThreshold(i);
  if (lastTouched & _BV(i))
    threshold = threshold * 2 / 3;
  return stableV(i) - threshold;
}
void updateTouchData() {
  unsigned long now = millis();
  boolean debug = false;
  lastTouched = currTouched;
  currTouched = 0;
  for (int i = 0; i < numTouchSensors; i++) if (valid[i]) {
      uint16_t value = cap.filteredData(i);
      if (value > 2) {
        currentValue[i] = value;
        int16_t threshold = touchThreshold(i);
        if (lastTouched & _BV(i))
          threshold = threshold * 3 / 4;
        if (value < stableV(i) - threshold ) {
          currTouched |= 1 << i;

          if ( firstTouchThisInterval[i] == 0)
            firstTouchThisInterval[i] = now;
        }
      }
    }

  newTouched = currTouched & ~lastTouched;
  if (nextSensorResetInterval < now) {
    sendSubActivity(1);

    considerResettingTouchSensors();
    nextSensorResetInterval = now + resetSensorsInterval;
  } else {
    sendSubActivity(2);
    if (!ampOn)
    for (int i = 0; i < numTouchSensors; i++) {
      if (minRecentValue[i] == 0 )
        minRecentValue[i] = currentValue[i];
      if (stableV(i) == 0)
        stableValue[i] = currentValue[i] - 3;
      if (minRecentValue[i] > currentValue[i]  )
        minRecentValue[i] = currentValue[i] ;
      else if (maxRecentValue[i] < currentValue[i] )
        maxRecentValue[i] = currentValue[i] ;
    }
  }
  if (nextTouchInterval < now) {
    sendSubActivity(3);
    // advance to next touch interval
    for (int i = 0; i < numTouchSensors; i++) {

      if ((touchedThisInterval & _BV(i)) != 0) {
        timeTouched[i]++;
        qualityTimeIntervals[i]++;
        if (qualityTimeIntervals[i] > 20)
          qualityTimeIntervals[i] = 20;
        timeUntouched[i] = 0;
        firstTouchThisInterval[i] = now;
        if (debug) Serial.print(timeTouched[i]);
      } else {
        timeTouched[i] = 0;
        firstTouchThisInterval[i] = 0;
        timeUntouched[i]++;
        int q = qualityTimeIntervals[i] - timeUntouched[i];
        if (q < 0)
          qualityTimeIntervals[i] = 0;
        else
          qualityTimeIntervals[i] = q;

        if (debug) Serial.print(-timeUntouched[i]);
      }
      if (debug) Serial.print(" ");

    }
    if (debug) Serial.println();
    if (false) {
      Serial.print("Touch: ");
      if (! privateSensorEnabled())
        Serial.print("(private sensor disabled) ");
      for (int i = 0; i < numTouchSensors; i++) {
        myprintf(Serial, "%2d %2d %2d  ", timeTouched[i], qualityTimeIntervals[i], timeUntouched[i]);
      }
      Serial.println();
    }

    touchedThisInterval = currTouched;
    lastTouchInterval = nextTouchInterval;
    nextTouchInterval = now + touchInterval;

  } else {
    touchedThisInterval |= currTouched;
    for (int i = 0; i < numTouchSensors; i++)
      if ((currTouched & _BV(i)) != 0 && firstTouchThisInterval[i] == 0) {
        firstTouchThisInterval[i] = now;
      }
  }

  sendSubActivity(4);
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
  sendSubActivity(5);
  if (false && nextPettingReport < now) {
    nextPettingReport = now + 250;
    for (int i = 0; i < numPettingSensors; i++) {

      float confidence = 0.0;
      /// int i = 7;
      float hz = detectPetting(i, 64, &confidence);
      digitalWrite(LED_BUILTIN, hz > 0.0);
      Serial.print(hz);
      Serial.print(" ");
      //       Serial.print(confidence);
      //      Serial.print(" ");
    }
    Serial.println();
  }
  if (debugTouch || plotTouch) {
    int start, end;
    if (MALL_SHEEP) {
      start = end = WHOLE_BODY_SENSOR;
    } else {
      start = PRIVATES_SENSOR;
      end = HEAD_SENSOR;
    }
    if (plotTouch) {
      for (int i = start; i <= end; i++) {
        if (false && currentValue[i] > stableV(i)+60) 
        myprintf(Serial, "%3d %3d %3d ", stableV(i), tripValue(i),
                 stableV(i)+50);
        else 
                 myprintf(Serial, "%3d %3d %3d ", stableV(i), tripValue(i),
                 currentValue[i]);
      }
      if (STABLE_VALUE != 0) {
        myprintf(Serial, "  %3d", STABLE_VALUE - 50 + 100 * currentSheepState->state);
      }
      else myprintf(Serial, "  %3d", 350 + 100 * currentSheepState->state);
    } else {
      Serial.print("TD ");
      myprintf(Serial, "%2x ", currTouched);
      for (int i = start; i <= end; i++) {
        Serial.print(isTouched((TouchSensor)i) ? "t " : "- ");

        myprintf(Serial, " %3d - %3d, %3d   %3d %3d %4d ", minRecentValue[i], maxRecentValue[i], stableV(i),
                 currentValue[i], sensorValue((TouchSensor)i),
                 combinedTouchDuration((TouchSensor)i));
      }
    }
    Serial.println();
  }
  sendSubActivity(6);
}

int32_t combinedTouchDuration(enum TouchSensor sensor) {
  if (timeTouched[sensor] > 0)
    return timeTouched[sensor] * touchInterval;
  return -timeUntouched[sensor] * touchInterval;

}
int32_t recentTouchDuration(enum TouchSensor sensor) {
  int8_t i = sensor;
  if ( !(touchedThisInterval & _BV(i)) )
    return 0;
  if ( firstTouchThisInterval[i] < lastTouchInterval) {
    if (printInfo()) myprintf(Serial, "Touch error; firstTouchThisInterval[%d] = %d, lastTouchInterval=%d\n",
             i, firstTouchThisInterval[i], lastTouchInterval);
    return millis() - lastTouchInterval;
  }
  return millis() - firstTouchThisInterval[i];
}

int32_t touchDuration(enum TouchSensor sensor) {
  return timeTouched[sensor] * touchInterval
         + recentTouchDuration(sensor);
}
int32_t qualityTime(enum TouchSensor sensor) {
  return qualityTimeIntervals[sensor] * touchInterval + recentTouchDuration(sensor);
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
  for (int i = 0; i < numTouchSensors; i++) {
    Serial.print(CDCx_value(i));
    Serial.print(" ");
  }
  Serial.println();
  // 6c 6d 6e 6f 70 71 72
  Serial.print("CDT: ");
  for (int i = 0; i < numTouchSensors; i++) {
    Serial.print(CDTx_value(i));
    Serial.print(" ");
  }
  Serial.println();

}

void logTouchConfiguration() {
  noInterrupts();
  logFile.print("TC, ");

  sendSubActivity(42);
  for (int i = 0; i < numTouchSensors; i++) {
    logFile.print(CDCx_value(i));
    logFile.print(",");
    logFile.print(CDTx_value(i));
    logFile.print(", ");
  }
  logFile.println();
  interrupts();
}
void changeMode(uint8_t newMode) {
  cap.writeRegister(MPR121_ECR, 0);
  delay(2);
  cap.writeRegister(MPR121_ECR, newMode);
  delay(2);
}


const uint8_t ELE_EN = numTouchSensors;
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

  cap.writeRegister(MPR121_AUTOCONFIG0, (FFI << 6) | (AUTO_CONFIGURE ? 0x17 : 0x16));
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
    touchData[i] = (((int32_t)touchData[i]) * 10000) / max;

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
