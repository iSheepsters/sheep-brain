#ifndef _TOUCH_H
#define _TOUCH_H

#include <Wire.h>
#include "Adafruit_MPR121.h"

const uint8_t  ELEPROX_EN = 0; // 0b10; // 0; // 0b10;
const uint8_t  lastTouchSensor = ELEPROX_EN == 0 ? 8 : 12;
const uint8_t  numTouchSensors = lastTouchSensor+1;
extern uint16_t stableValue[numTouchSensors];
extern uint8_t swapLeftRightSensors(uint8_t touched);
enum TouchSensor {
  PRIVATES_SENSOR, 
  RUMP_SENSOR,
  LEFT_SENSOR,
  RIGHT_SENSOR,
  BACK_SENSOR, 
  HEAD_SENSOR,
  UNUSED1_SENSOR,
  UNUSED2_SENSOR,
  WHOLE_BODY_SENSOR
};


extern uint16_t touchedThisInterval;
extern uint16_t currentValue[numTouchSensors];
extern uint16_t pettingDataPosition; 
extern boolean debugTouch;

const uint8_t numPettingSensors = numTouchSensors;

extern Adafruit_MPR121 cap;

extern void setupTouch();
extern void dumpTouchData();
extern uint16_t currTouched;
extern uint16_t newTouched;

extern void dumpConfiguration();
extern void dumpData();
extern void mySetup();


extern void logTouchConfiguration();

extern void updateTouchData();
extern int16_t sensorValue(enum TouchSensor sensor);
extern boolean isTouched(enum TouchSensor sensor);
extern int32_t touchDuration(enum TouchSensor sensor);
extern int32_t recentTouchDuration(enum TouchSensor sensor);
extern int32_t qualityTime(enum TouchSensor sensor);
extern int32_t combinedTouchDuration(enum TouchSensor sensor);
extern int32_t untouchDuration(enum TouchSensor sensor);
extern boolean newTouch(enum TouchSensor sensor);
extern void wasTouchedInappropriately();
extern uint8_t calculateBaseline(uint16_t value);
extern uint8_t CDTx_value(enum TouchSensor sensor);


extern uint8_t CDCx_value(enum TouchSensor sensor);


extern float detectPetting(uint8_t touchSensor, uint16_t sampleSize, float * confidence);


#endif
