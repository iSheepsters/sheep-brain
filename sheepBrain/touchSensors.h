#ifndef _TOUCH_H
#define _TOUCH_H

#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "tysons.h"

#if MALL_SHEEP
const uint8_t  lastTouchSensor = 7;
const uint8_t  firstTouchSensor = 7;
#else
const uint8_t  lastTouchSensor = 6;
const uint8_t  firstTouchSensor = 0;
#endif

const uint8_t  numTouchSensors = lastTouchSensor+1;
extern uint16_t stableValue[numTouchSensors];
extern uint16_t STABLE_VALUE;
extern uint8_t swapLeftRightSensors(uint8_t touched);
enum TouchSensor {
  PRIVATES_SENSOR, 
  RUMP_SENSOR,
  LEFT_SENSOR,
  RIGHT_SENSOR,
  BACK_SENSOR, 
  HEAD_SENSOR,
  UNUSED1_SENSOR,
  WHOLE_BODY_SENSOR
};

enum TouchReading {
    TOUCHED_YES,
    TOUCH_UNKNOWN,
    TOUCHED_NO,
};


extern uint16_t touchedThisInterval;
extern uint16_t currentValue[numTouchSensors];
extern uint16_t filteredValue[numTouchSensors];

extern Adafruit_MPR121 cap;

extern void setupTouch();
extern void dumpTouchData();
extern uint16_t currTouched;


extern void dumpConfiguration();
extern void dumpData();
extern void mySetup();


extern void logTouchConfiguration();

extern void updateTouchData();
extern int16_t sensorValue(enum TouchSensor sensor);
extern int16_t sensorValue(enum TouchSensor sensor);
extern boolean isTouched(enum TouchSensor sensor);
extern int32_t touchDuration(enum TouchSensor sensor);
extern int32_t qualityTime(enum TouchSensor sensor);
extern int32_t combinedTouchDuration(enum TouchSensor sensor);
extern uint32_t timeSinceLastKnownTouch(enum TouchSensor sensor);
extern uint32_t timeSinceLastKnownUntouch(enum TouchSensor sensor);

extern int32_t untouchDuration(enum TouchSensor sensor);
extern boolean newTouch(enum TouchSensor sensor);
extern void wasTouchedInappropriately();
extern uint8_t calculateBaseline(uint16_t value);
extern uint8_t CDTx_value(enum TouchSensor sensor);
extern uint8_t CDCx_value(enum TouchSensor sensor);




#endif
