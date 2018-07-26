#ifndef _GPS_H
#define _GPS_H

#include "Adafruit_GPS.h"

extern Adafruit_GPS GPS;
extern unsigned long lastGPSReading;
extern boolean setupGPS();
extern boolean updateGPS(unsigned long now);
extern void logGPS(unsigned long now);


#endif

