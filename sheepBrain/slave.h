#include <inttypes.h>
#include "state.h"


enum DayState {
  Night, Dawn, Morning_twilight, Sunrise, Day, Sunset, Twilight, Dusk};


struct __attribute__ ((packed)) CommData {
  time_t BRC_time;
  float feetFromMan;
  uint8_t sheepNum;
  enum State state;
  uint8_t currTouched;
  enum DayState when;
};

extern uint8_t sendComm();

extern void setupComm();


