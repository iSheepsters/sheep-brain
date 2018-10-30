#include <inttypes.h>
#include "state.h"


enum DayState {
  Night, Dawn, Morning_twilight, Sunrise, Day, Sunset, Twilight, Dusk
};


struct __attribute__ ((packed)) ActivityData {
    uint16_t secondsSinceLastActivity;
  uint16_t secondsSinceBoot;
  uint8_t lastActivity;
  uint8_t subActivity;
  uint8_t reboots;
};

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

extern int I2C_ClearBus();
extern void sendComm();

extern void setupComm();
extern void getLastActivity();

extern volatile uint8_t currentSubActivity;
extern void sendBoot();
extern uint8_t sendActivity(uint8_t activity);
extern uint8_t sendSubActivity(uint8_t subActivity);
extern ActivityData activityData;
