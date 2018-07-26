#include <Wire.h>
#include "Adafruit_MPR121.h"

const uint8_t  ELEPROX_EN = 0; // 0b10; // 0; // 0b10;
const uint8_t  lastTouchSensor = ELEPROX_EN == 0 ? 5 : 12;
const uint8_t  numTouchSensors = lastTouchSensor+1;

enum TouchSensor {
  PRIVATES_SENSOR, 
  RUMP_SENSOR,
  LEFT_SENSOR,
  RIGHT_SENSOR,
  BACK_SENSOR, 
  HEAD_SENSOR
};

extern uint16_t pettingDataPosition; 

const uint8_t numPettingSensors = numTouchSensors;

extern Adafruit_MPR121 cap;

extern void setupTouch();
extern void dumpTouchData();
extern uint16_t currTouched;
extern uint16_t newTouched;

extern void dumpConfiguration();
extern void dumpData();
extern void mySetup();
extern void calibrate();
extern void freeze();
extern void adjust();
extern void updateTouchData(unsigned long now);
extern boolean isTouched(enum TouchSensor sensor);
extern int32_t touchDuration(enum TouchSensor sensor);
extern int32_t combinedTouchDuration(enum TouchSensor sensor);
extern int32_t untouchDuration(enum TouchSensor sensor);
extern boolean newTouch(enum TouchSensor sensor);
extern uint8_t calculateBaseline(uint16_t value);

extern float detectPetting(uint8_t touchSensor, uint16_t sampleSize, float * confidence);

