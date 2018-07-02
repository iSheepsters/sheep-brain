#include <Wire.h>
#include "Adafruit_MPR121.h"

const uint8_t  ELEPROX_EN = 0; // 0b10; // 0; // 0b10;
const uint8_t  lastTouchSensor = ELEPROX_EN == 0 ? 11 : 12;
const uint8_t  numTouchSensors = lastTouchSensor+1;

const uint8_t HEAD_SENSOR = 0;
const uint8_t BACK_SENSOR = 1;
const uint8_t LEFT_SENSOR = 2;
const uint8_t RIGHT_SENSOR = 3;
const uint8_t RUMP_SENSOR = 4;
const uint8_t PRIVATES_SENSOR = 5;
extern uint16_t pettingDataPosition; 

const uint8_t numPettingSensors = numTouchSensors;

extern Adafruit_MPR121 cap;

extern void setupTouch();

extern void dumpConfiguration();
extern void dumpData();
extern void mySetup();
extern void calibrate();
extern void freeze();
extern void adjust();
extern void updateTouchData(unsigned long now);
extern float detectPetting(uint8_t touchSensor, uint16_t sampleSize, float * confidence);
