#ifndef _GPS_H
#define _GPS_H

#include "Adafruit_GPS.h"

#include <TimeLib.h>

extern Adafruit_GPS GPS;
extern uint32_t fixCount;
extern unsigned long lastGPSReading;
extern boolean setupGPS();
extern boolean discardGPS();
extern void quickGPSUpdate();
extern boolean updateGPS(unsigned long now);
extern void logGPS(unsigned long now);
extern time_t BRC_now();

extern boolean inCorral(int sheep) ;
extern boolean isClose(int sheep);
extern boolean isInFriendlyTerritory() ;
extern float distanceFromMan;
extern float distanceFromCoral;

extern volatile boolean read_gps_in_interrupt;

extern int32_t lastLatitudeDegreesFixed, lastLongitudeDegreesFixed;
#endif


