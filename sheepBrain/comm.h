#include <inttypes.h>
#include "state.h"


enum DayState {
  Night, Dawn, Morning_twilight, Sunrise, Day, Sunset, Twilight, Dusk};


struct __attribute__ ((packed)) CommData {
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

extern void sendComm();

extern void setupComm();
