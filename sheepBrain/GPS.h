#ifndef _GPS_H
#define _GPS_H

#include "Adafruit_GPS.h"

#include <TimeLib.h>

extern Adafruit_GPS GPS;
extern uint32_t fixCount;
extern unsigned long lastGPSReading;
extern boolean setupGPS();
extern boolean updateGPS(unsigned long now);
extern void logGPS(unsigned long now);


#endif

