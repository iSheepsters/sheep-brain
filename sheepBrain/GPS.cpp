#include <Arduino.h>

#include "GPS.h"
#include "util.h"
#include "printf.h"

#include <math.h>

const boolean GPS_DEBUG = false;
boolean GPS_ready = false;
uint16_t badGPS = 0;
uint16_t total_good_GPS = 0;
uint16_t total_bad_GPS = 0;

volatile boolean read_gps_in_interrupt = false;

Adafruit_GPS GPS(&Serial1);


boolean setupGPS() {
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);

  // uncomment this line to turn on GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);

  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  //GPS.sendCommand(PGCMD_ANTENNA);

  GPS.sendCommand(PMTK_ENABLE_SBAS);
  GPS.sendCommand(PMTK_ENABLE_WAAS);
  unsigned long now = millis();
  while (year() < 2018 && millis() < now + 4000) {
    delay(1);
    updateGPS(millis());
  }
  total_good_GPS = total_bad_GPS = 0;
  GPS_ready = true;
  return year() >= 2018;
}

// The following are calculated for 41N
const float METERS_PER_DEGREE_LATITUDE = 111049.68627269473;
const float METERS_PER_DEGREE_LONGITUDE = 84410.58121284918;


const float GERLACH_LATITUDE = 40.6516;
const float GERLACH_LONGITUDE = -119.3568;

const float MAN_LATITUDE = 40.7864;
const float MAN_LONGITUDE = -119.2065;

const float CORRAL_LATITUDE  =   40.78292556690011596332941314224;
const float CORRAL_LONGITUDE = -119.2062896809198096208703440047891035;

// measured in meters
float distanceFromMan;
float distanceFromCoral;

// measured in radians; 0 = north, pi/2 = east
float angleFromCoral;
float angleFromMan;

uint32_t timer = millis();
boolean haveFix = false;
boolean anyFix = false;
uint32_t fixCount = 0;
double latitudeDegreesAvg;
double longitudeDegreesAvg;
double latitudeDegreesMin;
double longitudeDegreesMin;
double latitudeDegreesMax;
double longitudeDegreesMax;
unsigned long lastGPSReading;
const boolean ECHO_GPS = false;
unsigned long firstFix = 0;
uint16_t totalFixes = 0;

time_t BRC_now() {
  return now() - 7 * 60 * 60;
}

boolean inCorral(int sheep) {
  float corralEW = METERS_PER_DEGREE_LONGITUDE * (getSheep(sheep).longitude - CORRAL_LONGITUDE);
  float corralNS = METERS_PER_DEGREE_LATITUDE * (getSheep(sheep).latitude  - CORRAL_LATITUDE);
  float distanceFromCorral = sqrt(corralEW * corralEW + corralNS * corralNS);
  return distanceFromCorral < 30;
}

boolean isClose(int sheep) {
  float EW = METERS_PER_DEGREE_LONGITUDE * (getSheep(sheep).longitude - getSheep().longitude );
  float NS = METERS_PER_DEGREE_LATITUDE * (getSheep(sheep).latitude  - getSheep().latitude);
  float distance = sqrt(EW * EW + NS * NS);
  return distance < 15;
}

boolean isInFriendlyTerritory() {
  if (!anyFix)
    return true;
  if (inCorral(sheepNumber))
    return true;
  for (int s = 0; s < NUMBER_OF_SHEEP; s++)
    if (s != sheepNumber && isClose(s))
      return true;
  return false;
};

void updateRelativePosition() {
  float corralEW = METERS_PER_DEGREE_LONGITUDE * (GPS.longitudeDegrees - CORRAL_LONGITUDE);
  float corralNS = METERS_PER_DEGREE_LATITUDE * (GPS.latitudeDegrees - CORRAL_LATITUDE);
  distanceFromCoral = sqrt(corralEW * corralEW + corralNS * corralNS);
  angleFromCoral = atan2(corralEW, corralNS);

  float manEW = METERS_PER_DEGREE_LONGITUDE * (GPS.longitudeDegrees - MAN_LONGITUDE);
  float manNS = METERS_PER_DEGREE_LATITUDE * (GPS.latitudeDegrees - MAN_LATITUDE);
  distanceFromMan = sqrt(manEW * manEW + manNS * manNS);
  angleFromMan = atan2(manEW, manNS);
}


time_t getGPSTime() {
  static tmElements_t tm;
  //it is converted to years since 1970

  tm.Year = GPS.year + 30;
  tm.Month = GPS.month;
  tm.Day = GPS.day;
  tm.Hour = GPS.hour;
  tm.Minute = GPS.minute;
  tm.Second = GPS.seconds;
  return makeTime(tm);
}

float lastLatitudeDegrees = -1000;
float lastLongitudeDegrees = 0.0;

time_t lastGPSTime = 0;

// millis value for when lastGPSTime was recorded.

unsigned long lastGPSTimeAt = 0;

boolean parseGPS(unsigned long now) {
  char * txt = GPS.lastNMEA();
  if (strstr(txt, "$PMTK"))
    return true;
  size_t len = strlen(txt);
  char checksumStart = txt[len - 4];
  if (checksumStart != '*') {
    if (GPS_DEBUG) myprintf(Serial, "No GPS checksum (%d:%c %02x%02x%02x%02x): \"%s\"\n", len - 4, checksumStart, checksumStart,
                              txt[len - 3], txt[len - 2], txt[len - 1], (txt + 1) );
    return false;
  }
  if (true) for (int i = 1; i < len - 1; i++) if (txt[i] < ' ' || txt[i] >= 'z') {
        if (GPS_DEBUG) myprintf(Serial, "bad character (%d:%x): \"%s\"\n", i, txt[i], (txt + 1));
        return false;
      }
  if (!GPS.parse(txt)) {
    if (GPS_DEBUG) {
      myprintf(Serial, "Could not parse GPS string: \"%s\"\n", (txt + 1));
    }
    return false;
  }

  if (GPS.year != 18 || GPS.month < 8 || GPS.month > 9 || GPS.day < 1 || GPS.day > 31) {
    if (GPS_DEBUG) myprintf(Serial, "Bad date from GPS string: \"%s\"\n", (txt + 1));
    return false;
  }

  time_t thisGPSTime = getGPSTime();
  boolean timeOK = true;
  time_t difference = thisGPSTime - lastGPSTime;

  if (difference <= 0)
    timeOK = false;
  int fastBy = difference * 1000 - (now - lastGPSTimeAt);
  if (fastBy > 2000)
    timeOK = false;
  lastGPSTime = thisGPSTime;
  lastGPSTimeAt = now;
  if (!timeOK) {
    if (GPS_DEBUG) myprintf(Serial, "Inconsistent date delta %d:%d from GPS string: \"%s\"\n",
                              difference, fastBy, (txt + 1));

    return false;
  }
  setTime(thisGPSTime);

  if (GPS.fix) {
    if (lastLatitudeDegrees != -1000 && (abs( GPS.latitudeDegrees  - lastLatitudeDegrees) > 0.001 ||
                                         abs( GPS.longitudeDegrees  - lastLongitudeDegrees) > 0.001)) {
      // too much change
      if (GPS_DEBUG)  {

        myprintf(Serial,  "Bad lat/long change from GPS string: \"%s\"\n . ", (txt + 1));
        Serial.print(lastLatitudeDegrees, 6);
        Serial.print(", ");
        Serial.print(lastLongitudeDegrees, 6);
        Serial.print(" -> ");
        Serial.print(GPS.latitudeDegrees, 6);
        Serial.print(" ");
        Serial.print(GPS.longitudeDegrees, 6);
        Serial.println();
      }


      lastLatitudeDegrees = GPS.latitudeDegrees;
      lastLongitudeDegrees = GPS.longitudeDegrees;
      return false;
    }
    if ( GPS.latitudeDegrees < 38 ||  GPS.latitudeDegrees > 41 || GPS.longitudeDegrees < -120
         || GPS.longitudeDegrees > -76)  {
      if (GPS_DEBUG)
        myprintf(Serial, "Bad lat/long from GPS string: \"%s\"\n", (txt + 1));
      return false;
    }


    lastLatitudeDegrees = GPS.latitudeDegrees;
    lastLongitudeDegrees = GPS.longitudeDegrees;
    getSheep().latitude = GPS.latitudeDegrees;
    getSheep().longitude = GPS.longitudeDegrees;

    lastGPSReading = now;
    anyFix = true;
    totalFixes++;
    if (totalFixes % 1000 == 0) {
      uint16_t minutes =  (now - firstFix) / 1000 / 60;
      myprintf(Serial, "%d GPS fixes per minute\n", totalFixes / minutes);
    }
    if (!haveFix) {
      Serial.println("Acquired fix");
      if (firstFix == 0)
        firstFix = now;
      latitudeDegreesAvg = GPS.latitudeDegrees;
      longitudeDegreesAvg = GPS.longitudeDegrees;
      fixCount = 0;
      haveFix = true;
      latitudeDegreesMin = longitudeDegreesMin = 1000;
      latitudeDegreesMax = longitudeDegreesMax = -1000;
    } else {
      latitudeDegreesAvg = (3 * latitudeDegreesAvg + GPS.latitudeDegrees) / 4;
      longitudeDegreesAvg = (3 * longitudeDegreesAvg + GPS.longitudeDegrees) / 4;
      fixCount++;
      if (fixCount >= 30) {
        if (latitudeDegreesMin > latitudeDegreesAvg)
          latitudeDegreesMin = latitudeDegreesAvg;
        if (latitudeDegreesMax < latitudeDegreesAvg)
          latitudeDegreesMax = latitudeDegreesAvg;
        if (longitudeDegreesMin > longitudeDegreesAvg)
          longitudeDegreesMin = longitudeDegreesAvg;
        if (longitudeDegreesMax < longitudeDegreesAvg)
          longitudeDegreesMax = longitudeDegreesAvg;
      }
      updateRelativePosition();
    }
  } else if (haveFix) {
    Serial.println("Lost fix");
    haveFix = false;
  }
  return true;
}

unsigned long lastGPSUpdate = 0;

// Discard characters read from GPS; called after startup, when we might have characters we
boolean discardGPS() {
  Serial.println("Discarding GPS data");
  int count = 0;
  while (true) {
    char c = GPS.read();
    if (!c) break;
    Serial.print(c);
    count++;
  }

  Serial.println();
  return count > 0;
}


void quickGPSUpdate() {
  noInterrupts();
  boolean oldValue = read_gps_in_interrupt;
  read_gps_in_interrupt = false;
  interrupts();
  while (Serial1.available()) {
    GPS.read();
    //    if (!c) return;
    //    c = GPS.read();
    //    if (!c) return;
    //    c = GPS.read();
    //    if (!c) return;
  }
  noInterrupts();
  read_gps_in_interrupt = oldValue;
  interrupts();
}


unsigned long nextGPSReport = 10000;
boolean updateGPS(unsigned long now) {
  if (nextGPSReport < now) {
    myprintf(Serial, "GPS readings %d good, %d bad\n", total_good_GPS, total_bad_GPS);
    nextGPSReport = now + 10000;
  }
  if (!read_gps_in_interrupt) {
    while (true) {
      char c = GPS.read();
      if (c == 0) break;
    }
  }
  if (!GPS.newNMEAreceived())
    return false;

  if (!parseGPS(now)) {
    total_bad_GPS++;
    badGPS++;
    if (badGPS > 2)
      fixCount = 0;
  } else {
    badGPS = 0;
    total_good_GPS++;
  }
}


void logGPS(unsigned long now) {
  if (GPS.fix) {
    if (!haveFix) {
      Serial.println("Acquired fix");
      latitudeDegreesAvg = GPS.latitudeDegrees;
      longitudeDegreesAvg = GPS.longitudeDegrees;
      fixCount = 0;
      haveFix = true;
      latitudeDegreesMin = longitudeDegreesMin = 1000;
      latitudeDegreesMax = longitudeDegreesMax = -1000;
    } else {
      latitudeDegreesAvg = (3 * latitudeDegreesAvg + GPS.latitudeDegrees) / 4;
      longitudeDegreesAvg = (3 * longitudeDegreesAvg + GPS.longitudeDegrees) / 4;
      fixCount++;
      if (fixCount >= 30) {
        if (latitudeDegreesMin > latitudeDegreesAvg)
          latitudeDegreesMin = latitudeDegreesAvg;
        if (latitudeDegreesMax < latitudeDegreesAvg)
          latitudeDegreesMax = latitudeDegreesAvg;
        if (longitudeDegreesMin > longitudeDegreesAvg)
          longitudeDegreesMin = longitudeDegreesAvg;
        if (longitudeDegreesMax < longitudeDegreesAvg)
          longitudeDegreesMax = longitudeDegreesAvg;
      }
      if (now - timer > 1000) {
        timer = now; // reset the timer
        Serial.print(latitudeDegreesAvg, 6);
        Serial.print(", ");
        Serial.print(longitudeDegreesAvg, 6);
        Serial.print(", ");
        Serial.print(GPS.latitudeDegrees, 6);
        Serial.print(", ");
        Serial.print(GPS.longitudeDegrees, 6);
        Serial.print(", ");
        Serial.print((int)GPS.fixquality);
        Serial.print(", ");
        Serial.print((int)fixCount);
        if (fixCount >= 30) {
          Serial.print(", ");
          Serial.print(latitudeDegreesMax - latitudeDegreesMin, 6);
          Serial.print(", ");
          Serial.print(longitudeDegreesMax - longitudeDegreesMin, 6);
        }
        Serial.println();

      }
    }
  } else if (haveFix) {
    Serial.println("Lost fix");
    fixCount = 0;
  }
}

