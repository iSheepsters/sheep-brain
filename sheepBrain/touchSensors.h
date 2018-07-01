#include <Wire.h>
#include "Adafruit_MPR121.h"

const uint8_t  ELEPROX_EN = 0; // 0b10; // 0; // 0b10;
const uint8_t  lastTouchSensor = ELEPROX_EN == 0 ? 11 : 12;
const uint8_t  numTouchSensors = lastTouchSensor+1;

const uint8_t numPettingSensors = 6;



extern Adafruit_MPR121 cap;

extern void setupTouch();

extern void dumpConfiguration();
extern void dumpData();
extern void mySetup();
extern void calibrate();
extern void freeze();
extern void adjust();
