#ifndef _ALL_H
#define _ALL_H

#include <FastLED.h>

const uint8_t GRID_HEIGHT = 25;
const uint8_t QUARTER_GRID_WIDTH = 8;
const uint8_t HALF_GRID_WIDTH = 16;
const uint8_t GRID_WIDTH = 2 * HALF_GRID_WIDTH;



enum State {
  Bored,
  Attentive,
  Riding,
  ReadyToRide,
  NotInTheMood,
  Violated,
};

enum DayState {
  Night, Dawn, Morning_twilight, Sunrise, Day, Sunset, Twilight, Dusk
};

struct __attribute__ ((packed)) CommData {
  CommData() : BRC_time(0), feetFromMan(0), sheepNum(0), state(Bored),
    currTouched(0), when(Night) {};

  time_t BRC_time;
  uint16_t feetFromMan;
  uint8_t sheepNum;
  enum State state;
  uint8_t currTouched;
  uint8_t backTouchQuality; // seconds
  uint8_t headTouchQuality; // seconds
  boolean haveFix;
  enum DayState when;
};

extern CommData commData;

extern CRGB & getSheepLEDFor(uint8_t x, uint8_t y);

extern CRGB leds[];
extern void setupSchedule();

#endif

