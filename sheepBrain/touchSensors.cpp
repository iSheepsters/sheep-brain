
#include <Arduino.h>

#include "touchSensors.h"
#include "logging.h"
#include "state.h"
#include "scheduler.h"
#include "sound.h"
#include "comm.h"
#include "state.h"

#include "util.h"

Adafruit_MPR121 cap = Adafruit_MPR121();

uint16_t STABLE_VALUE = 0;
const bool AUTO_CONFIGURE = true;

#define _BV(bit)   (1 << ((uint8_t)bit))

#include "printf.h"

boolean debugTouch = false;

boolean plotTouch = false;

uint16_t minRecentValue[numTouchSensors];
uint16_t maxRecentValue[numTouchSensors];
uint16_t stableValue[numTouchSensors];
uint16_t currentValue[numTouchSensors];
uint16_t filteredValue[numTouchSensors];
unsigned long lastResetOf[numTouchSensors];


const int HISTOGRAM_SCALE = 2;
const int HISTOGRAM_SIZE = 1024 / HISTOGRAM_SCALE;

uint16_t histogram[numTouchSensors][HISTOGRAM_SIZE];
uint16_t histogramCount[numTouchSensors];

uint16_t currTouched;
const int RING_BUFFER_SIZE = 4;

int16_t ringBuffer[numTouchSensors][RING_BUFFER_SIZE];
uint8_t ringBufferPos = 0;

uint8_t ringBufferFill = 0;

uint16_t numSamples = 0;
const boolean DISABLE_TOUCH_DURING_SOUND = false;

const uint16_t resetSensorsInterval = 15000;
unsigned long nextSensorResetInterval = 0;

unsigned long lastTouch[numTouchSensors];
unsigned long lastUntouch[numTouchSensors];
unsigned long lastTouchStarted[numTouchSensors];
unsigned long lastUntouchedStarted[numTouchSensors];
unsigned long lastDisabled;

boolean valid[numTouchSensors];

const uint8_t  FFI = 3;
const uint8_t  SFI = 3;
const uint8_t  ESI = 0;
const uint8_t  CDC = 43; // 54;
const uint8_t  CDT = 6;

boolean fixed = false;

const boolean trace = false;
boolean wasDisabled = true;
boolean inReboot = false;
unsigned long startedReboot;
extern void turnOnTouch();

extern void turnOffTouch();

boolean touchDisabled() {
  return millis() < 4000 || DISABLE_TOUCH_DURING_SOUND && ampOn || inReboot;
}

boolean isTouched(enum TouchSensor sensor) {
  return (currTouched & _BV(sensor)) != 0;
}


uint16_t stableV(int i) {
  if (false && STABLE_VALUE != 0)
    return STABLE_VALUE;
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
  addr += offsetToFirstSensor;
  uint8_t value = cap.readRegister8(0x6C + addr / 2);
  if (addr % 2 == 0)
    return value & 0x07;
  return (value >> 4) & 0x07;
}

uint8_t CDTx_value(enum TouchSensor sensor) {
  return CDTx_value((uint8_t) sensor);
}

uint8_t CDCx_value(uint8_t sensor) {
  return cap.readRegister8(0x5f + offsetToFirstSensor + sensor) & 0x3f;
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
    lastUntouch[t] = 0;
    lastTouchStarted[t] = 0;
    lastUntouchedStarted[t] = 0;
    lastResetOf[t] = 0;
    histogramCount[t] = 0;
  }
  Serial.println("Applying configuration");
  mySetup();
  setupDelay(100);
  dumpConfiguration();
  checkValid();
  for (int i = firstTouchSensor; i < numTouchSensors; i++) {
    uint16_t value = cap.filteredData(offsetToFirstSensor + i);
    if (!valid[i])
      myprintf(Serial, "sensor %d not connected\n", i);
    else if (value > 10)
      stableValue[i] = value - 1;
    myprintf(Serial, "sensor %d = %d\n", i, value);

  }
  nextSensorResetInterval = millis() + 1000;
  addScheduledActivity(50, updateTouchData, "touch");
  Serial.println("done with touch setup");
}

void dumpTouchData(int i) {
  Serial.print(i);
  Serial.print(" ");
  Serial.print(isTouched((TouchSensor)i) ? "t " : "- ");
  Serial.print(filteredValue[i]);
  Serial.print(" ");
  Serial.print(stableV(i));
  Serial.print("  ");
  Serial.print(touchDuration((TouchSensor)i));
  Serial.print("  ");
  Serial.println(untouchDuration((TouchSensor)i));
}
void dumpTouchData() {

  if (true) {
    for (int i = 0; i < 6; i++)
      dumpTouchData(i);
  } else
    for (int i = 0; i < 6; i++) {
      Serial.print(cap.filteredData(offsetToFirstSensor + i) - stableV(i));
      Serial.print("  ");
    }
  Serial.println();
}

void wasTouchedInappropriately() {


}

uint8_t touchThreshold(uint8_t i) {

  enum TouchSensor sensor = (TouchSensor) i;
  switch (sensor) {
    case HEAD_SENSOR:
      return 6;
    case LEFT_SENSOR:
    case RIGHT_SENSOR:
      return 15;
    case PRIVATES_SENSOR:
      return 20;
    case BACK_SENSOR:
    case RUMP_SENSOR:
    default:
      return 15;
  }
}

int16_t sensorValue(enum TouchSensor sensor) {
  int value = cap.filteredData(offsetToFirstSensor + (uint8_t) sensor);
  if (value < 10) return 0;
  return value - ( stableValue[sensor] - touchThreshold((uint8_t)sensor));
}

unsigned long lastReset = 20000;

boolean isStable(int sensor) {
  if (minRecentValue[sensor] < 200)
    return false;
  int range = maxRecentValue[sensor] - minRecentValue[sensor];
  int maxRange = 5;
  unsigned long secondsSinceLastReset = (millis() - lastReset) / 1000;
  if (lastReset > 5 * 60)
    maxRange *= 2;
  return range <= maxRange;
}

void resetStableValue(int sensor) {

  if (isStable(sensor) ) {
    stableValue[sensor] = minRecentValue[sensor] - 1;
  }
  else if (printInfo())
    myprintf(Serial, "Asked to reset %d, but it isn't stable\n", sensor);

}


uint16_t resetCount = 0;
void considerResettingTouchSensors() {

  if (touchDisabled())
    return;
    
   if (numSamples < 20) {
    if (printInfo()) {
      myprintf(Serial,"Only %d samples, not resetting\n", numSamples);
    }
    return;
   }

  uint16_t histogramPercentiles[numTouchSensors][11];

  for (int t = firstTouchSensor; t <= lastTouchSensor; t++) {
    int countSoFar = 0;
    int pos = 0;
    while (pos < HISTOGRAM_SIZE && histogram[t][pos] == 0) pos++;
    histogramPercentiles[t][0] = pos * HISTOGRAM_SCALE + HISTOGRAM_SCALE / 2;
    for (int i = 1; i <= 10; i++) {
      while (countSoFar < i * histogramCount[t] / 10 && pos < HISTOGRAM_SIZE)  {
        countSoFar += histogram[t][pos];
        histogram[t][pos++] = 0;
      }
      histogramPercentiles[t][i] = pos * HISTOGRAM_SCALE + HISTOGRAM_SCALE / 2;
    }
    while (pos < HISTOGRAM_SIZE)
      histogram[t][pos++] = 0;
  }

  boolean allStable = true;
  int potentialMaxChange = 0;
  sendSubActivity(10);
  //  if (!MALL_SHEEP)
  //    for (int i = firstTouchSensor; i <= lastTouchSensor; i++) {
  //      if (i != LEFT_SENSOR && i != RIGHT_SENSOR && !isStable(i))
  //        allStable = false;
  //      potentialMaxChange = max(potentialMaxChange, abs(stableV(i) - (minRecentValue[i] - 1)));
  //    }

  if (printInfo() ) {
    noInterrupts();
    for (int t = firstTouchSensor; t <= lastTouchSensor; t++) {

      myprintf(Serial, "histogram for %d, stableValue %d\n",
               t, stableV(t));

      myprintf(logFile, "H, %d, %d, ", t, stableV(t));
      // show histogram

      for (int i = 0; i <= 10; i++) {

        myprintf(Serial, "%2d%% %3d \n", i * 10, histogramPercentiles[t][i]);
        myprintf(logFile, "%d, ", histogramPercentiles[t][i]);
      }

      logFile.println();
      logState();

      interrupts();
    }

  }

  sendSubActivity(11);

  noInterrupts();
  if (allStable)
    logFile.print("S,");
  else
    logFile.print("U,");
  logFile.print(potentialMaxChange);
  logFile.print(",  ");
  sendSubActivity(21);
  int start = 0;
  for (int i = firstTouchSensor; i <= lastTouchSensor; i++) {
    logFile.print(minRecentValue[i]);
    logFile.print(",");

    logFile.print(maxRecentValue[i]);
    logFile.print(",");
    logFile.print(stableV(i));
    logFile.print(",");
    logFile.print(timeSinceLastKnownTouch((enum TouchSensor)i) / 1000);
    logFile.print(", ");
    logFile.print(timeSinceLastKnownUntouch((enum TouchSensor)i) / 1000);
    logFile.print(", ");
  }
  sendSubActivity(22);
  logFile.println();
  interrupts();

  int now = millis();
  for (int t = firstTouchSensor; t <= lastTouchSensor; t++) {




    int age = (now - lastResetOf[t]) / 1000;
    int lowBucket = 2;
    int highBucket = 8;
    int secondsTouch = touchDuration((TouchSensor)t) / 1000;
    int secondsUntouch = untouchDuration((TouchSensor)t) / 1000;
    if ( stableValue[t] < histogramPercentiles[t][highBucket]) {
      if (age > 10 * 60)
        lowBucket = 6;
      else if (age > 2 * 60)
        lowBucket = 4;
    }
    int range = min(800, histogramPercentiles[t][highBucket]) - histogramPercentiles[t][lowBucket];
    if (printInfo()) {
      myprintf(Serial, "consider touch reset of %d(%d%%, %d secs) : %3d, %3d...%3d, %3d\n",
               t, lowBucket * 10,
               age,
               stableValue[t], histogramPercentiles[t][lowBucket],
               histogramPercentiles[t][highBucket], range);
    }
    int oldStableValue = stableValue[t];
    bool inRange = range < touchThreshold(t) / 2 + HISTOGRAM_SIZE;
    
    if (inRange
        || stableValue[t] < histogramPercentiles[t][0] && secondsUntouch > 60 ) {
      if (!inRange && histogramPercentiles[t][highBucket - 1] <= stableValue[t] && secondsTouch < 120) {
        if (printInfo())
          myprintf(Serial,
                   "for sensor %d, stableValue = %d, high bucket = %d, been touched for only %d seconds, ignoring\n",
                   t, stableValue[t], histogramPercentiles[t][highBucket], secondsTouch);

        continue;
      }
      stableValue[t] = min(histogramPercentiles[t][lowBucket], 800);
      lastResetOf[t] = now;
      if (oldStableValue != stableValue[t] && printInfo())
        myprintf(Serial, "stable value of %d reset to %d\n", t, stableValue[t] );
    } else if (false && histogramPercentiles[t][highBucket] < stableValue[t]) {
      stableValue[t] = histogramPercentiles[t][highBucket];
      lastResetOf[t] = now;
      if (oldStableValue != stableValue[t] && printInfo())
        myprintf(Serial, "unstable, but too high, stable value reset to %d\n",  stableValue[t] );
    }
    if (oldStableValue != stableValue[t]) {
      noInterrupts();
      myprintf(logFile, "TR, %d, %d, %d, %d, %d, %d\n", t, age, oldStableValue, stableValue[t],
               histogramPercentiles[t][lowBucket],
               histogramPercentiles[t][highBucket]);
      interrupts();

    }
  }

  sendSubActivity(15);
  for (int i = firstTouchSensor; i < numTouchSensors; i++) {
    histogramCount[i] = 0;
    minRecentValue[i] = maxRecentValue[i] = filteredValue[i];
  };
}

uint16_t tripValue(int i) {
  int16_t threshold = touchThreshold(i);
  return stableV(i) - threshold;
}

uint8_t ringBufferTouches(int i) {
  if (!valid[i])
    return 0;
  int count = 0;
  for (int p = 0; p < RING_BUFFER_SIZE; p++)
    if ( ringBuffer[i][p] < 0)
      count++;
  return count;
}

void clearHistogram() {
  for (int t = firstTouchSensor; t < numTouchSensors; t++) {
    histogramCount[t] = 0;
    for (int i = 0; i <= HISTOGRAM_SIZE; i++)
      histogram[t][i] = 0;
  }
}

void updateTouchData() {
  unsigned long now = millis();
  boolean debug = false;
  currTouched = 0;
  if (inReboot && now > startedReboot + 500) {
    inReboot = false;

    turnOnTouch();
    delay(10);
    logTouchConfiguration();
    if (printInfo())
      myprintf(Serial, "turning on touch, CDCx = %d, CDTx = %d\n",
               CDCx_value(WHOLE_BODY_SENSOR),  CDTx_value(WHOLE_BODY_SENSOR));
  }

  
    wasDisabled = false;
    for (int i = firstTouchSensor; i <= lastTouchSensor; i++) if (valid[i]) {
        uint16_t value = cap.filteredData(offsetToFirstSensor + i);
        if (value < 2) continue;
        currentValue[i] = value;
        if (numSamples == 0)
          filteredValue[i] = value;
        else if (numSamples == 1)
          filteredValue[i] = (value + filteredValue[i]) / 2;
        else
          filteredValue[i] = (value + 3 * filteredValue[i]) / 4;
        int16_t threshold = touchThreshold(i);
        int16_t delta = value - (stableV(i) - threshold);
        ringBuffer[i][ringBufferPos] = delta;
        if (numSamples > 5 ) {
          int v = filteredValue[i] / HISTOGRAM_SCALE;
          if (histogramCount[i] < 0xff00 && v >= 0 && v < HISTOGRAM_SIZE) {
            histogram[i][v]++;
            histogramCount[i]++;
          }
          if (minRecentValue[i] == 0 )
            minRecentValue[i] = filteredValue[i];
          if (stableValue[i] == 0)
            stableValue[i] = filteredValue[i] - 3;
          if (minRecentValue[i] > filteredValue[i]  )
            minRecentValue[i] = filteredValue[i] ;
          else if (maxRecentValue[i] < filteredValue[i] )
            maxRecentValue[i] = filteredValue[i] ;
        }
      

    numSamples++;
    if (numSamples > 1000)
      numSamples = 1000;

    if (ringBufferFill < RING_BUFFER_SIZE)
      ringBufferFill++;

    ringBufferPos++;
    if (ringBufferPos >= RING_BUFFER_SIZE)
      ringBufferPos = 0;
    unsigned long checkForTouchesSince = now - 5000;
    if (ringBufferFill == RING_BUFFER_SIZE) {
      currTouched = 0;
      for (int i = firstTouchSensor; i < numTouchSensors; i++)
        if (ringBufferTouches(i) > RING_BUFFER_SIZE / 2) {
          lastTouch[i] = now;
          currTouched |= _BV(i);
          if (lastTouchStarted[i] == 0)
            lastTouchStarted[i] = now;
        } else {
          lastUntouch[i] = now;
          if (lastDisabled < checkForTouchesSince && lastTouch[i] < checkForTouchesSince) {
            lastTouchStarted[i] = 0;
            lastUntouchedStarted[i] = max(lastDisabled, lastTouch[i] );
          }
        }

    }

    if (nextSensorResetInterval < now) {
      sendSubActivity(1);
      considerResettingTouchSensors();
      nextSensorResetInterval = now + resetSensorsInterval;
    }
  }
  sendSubActivity(5);
  if (debugTouch || plotTouch) {

    if (plotTouch) {
      for (int i = firstTouchSensor; i <= lastTouchSensor; i++) {
        int v =  touchDisabled() ? (inReboot ? 500 : cap.filteredData(offsetToFirstSensor + i)) : filteredValue[i];

          myprintf(Serial, "%3d %3d  ", stableValue[i], v);
        
      }
      if (STABLE_VALUE != 0) {
        myprintf(Serial, "  %3d %3d", touchDisabled() ? (inReboot ? 100 : 0) : 400, STABLE_VALUE - 50 + 100 * currentSheepState->state);
      }
      else
        myprintf(Serial, "  %3d %3d ", touchDisabled() ? 300 : 500, 450 + 100 * currentSheepState->state);
    } else {
      Serial.print("TD ");
      myprintf(Serial, "%2x ", currTouched);
      for (int i = firstTouchSensor; i <= lastTouchSensor; i++) {
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
  int32_t result = touchDuration(sensor);
  if (result > 0)
    return result;
  return - untouchDuration(sensor);
}


int32_t touchDuration(enum TouchSensor sensor) {
  if (lastTouchStarted[sensor] == 0)
    return 0;
  return millis() - lastTouchStarted[sensor];

}
int32_t qualityTime(enum TouchSensor sensor) {
  return touchDuration(sensor);
}
int32_t untouchDuration(enum TouchSensor sensor) {
  if (lastTouchStarted[sensor] != 0)
    return 0;
  if (lastDisabled > lastUntouchedStarted[sensor])
    return 0;
  return millis() - lastUntouchedStarted[sensor];
}
uint32_t timeSinceLastKnownTouch(enum TouchSensor sensor) {
  return millis() - lastTouch[sensor];
}
uint32_t timeSinceLastKnownUntouch(enum TouchSensor sensor) {
  return millis() - lastUntouch[sensor];
}

void dumpData() {

  // debugging info, what
  if (true) for (uint8_t i = firstTouchSensor; i <= lastTouchSensor; i++) {
      Serial.print(cap.filteredData(offsetToFirstSensor + i)); Serial.print("\t");
    }
  if (!fixed) for (uint8_t i = firstTouchSensor; i <= lastTouchSensor; i++) {
      Serial.print(cap.baselineData(offsetToFirstSensor + i)); Serial.print("\t");
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
  Serial.print("CDCx: ");
  for (int i = firstTouchSensor; i <= lastTouchSensor; i++) {
    Serial.print(CDCx_value(i));
    Serial.print(" ");
  }
  Serial.println();
  // 6c 6d 6e 6f 70 71 72
  Serial.print("CDTx: ");
  for (int i = firstTouchSensor; i <= lastTouchSensor; i++) {
    Serial.print(CDTx_value(i));
    Serial.print(" ");
  }
  Serial.println();

}

void logTouchConfiguration() {
  noInterrupts();
  logFile.print("TC, ");

  sendSubActivity(42);
  for (int i = firstTouchSensor; i < numTouchSensors; i++) {
    logFile.print(CDCx_value(i));
    logFile.print(",");
    logFile.print(CDTx_value(i));
    logFile.print(", ");
  }
  logFile.println();
  optionalFlush();
  interrupts();
}
void changeMode(uint8_t newMode) {
  cap.writeRegister(MPR121_ECR, 0);
  delay(2);
  cap.writeRegister(MPR121_ECR, newMode);
  delay(2);
}


const uint8_t ELE_EN = numTouchSensors + offsetToFirstSensor;

void turnOffTouch() {
  cap.writeRegister(MPR121_ECR, 0);
}
void setECR(bool updateBaseline) {
  // 0b10111111
  // CL = 01
  // EXPROX = ELEPROX_EN
  // ELE_EN = (number of sensors to enable)
  uint8_t CL = updateBaseline ? 0x10 : 0x01;
  uint8_t ELEPROX_EN = 0;

  cap.writeRegister(MPR121_ECR, (CL << 6) | (ELEPROX_EN << 4) | ELE_EN);

}
void turnOnTouch() {
  setECR(true);
}


void mySetup() {
  turnOffTouch();
  setupDelay(10);
  cap.writeRegister(MPR121_UPLIMIT, 200);
  cap.writeRegister(MPR121_TARGETLIMIT, 181);
  cap.writeRegister(MPR121_LOWLIMIT, 131);

  cap.writeRegister(MPR121_CONFIG1, (FFI << 6) | CDC);

  cap.writeRegister(MPR121_CONFIG2, (CDT << 5) | (SFI << 3) | ESI);
  // FFI = 0b00
  // RETRY = 0b01
  // BVA = 01
  // ACE = 1
  // ARE = 1

  cap.writeRegister(MPR121_AUTOCONFIG0, (FFI << 6) | (AUTO_CONFIGURE ? 0x17 : 0x16));
  turnOnTouch();
  setupDelay(10);
}
