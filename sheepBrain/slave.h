#include <inttypes.h>


enum DayState {
  Night, Dawn, Morning_twilight, Sunrise, Day, Sunset, Twilight, Dusk};


struct __attribute__ ((packed)) CommData {
  time_t time;
  uint16_t feetFromMan;
  uint8_t sheepNum;
  uint8_t state;
  uint8_t currTouched;
  enum DayState when;
};

extern uint8_t sendSlave(uint8_t addr, uint8_t value);

extern void setupComm();


