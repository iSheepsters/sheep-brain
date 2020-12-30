#ifndef _time_H
#define _time_H

#include "RTClib.h"

extern bool setupTime();
extern  time_t now();
extern uint8_t adjustedHour();
extern time_t adjustedNow();

extern uint16_t year();
extern uint8_t month();
extern uint8_t day();
extern uint8_t hour();
extern uint8_t minute();
extern uint8_t second();


#endif
