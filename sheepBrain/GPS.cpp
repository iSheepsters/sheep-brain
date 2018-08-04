#include <Arduino.h>

#include "GPS.h"

#include <math.h>

Adafruit_GPS GPS(&Serial1);

int lastSecond, lastMinute;

boolean setupGPS() {
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  //GPS.sendCommand(PGCMD_ANTENNA);

  GPS.sendCommand(PMTK_ENABLE_SBAS);
  GPS.sendCommand(PMTK_ENABLE_WAAS);
  delay(10);
  lastMinute = lastSecond = -1;
  return true;

}

// The following are calculated for 41N
const float METERS_PER_DEGREE_LATITUDE = 111049.68627269473;
const float METERS_PER_DEGREE_LONGITUDE = 84410.58121284918;


const float GERLACH_LATITUDE = 40.6516;
const float GERLACH_LONGITUDE = -119.3568;

const float MAN_LATITUDE = 40.6516;
const float MAN_LONGITUDE = -119.3568;

const float CORAL_LATITUDE = 40.78292556690011596332941314224;
const float CORAL_LONGITUDE = -119.2062896809198096208703440047891035;

// measured in meters
float distanceFromMan;
float distanceFromCoral;

// measured in radians; 0 = north, pi/2 = east
float angleFromCoral;
float angleFromMan;

uint32_t timer = millis();
boolean haveFix = false;
uint32_t fixCount = 0;
double latitudeDegreesAvg;
double longitudeDegreesAvg;
double latitudeDegreesMin;
double longitudeDegreesMin;
double latitudeDegreesMax;
double longitudeDegreesMax;
unsigned long lastGPSReading;
const boolean ECHO_GPS = false;

void updateRelativePosition() {
  float coralEW = METERS_PER_DEGREE_LONGITUDE * (GPS.longitudeDegrees - CORAL_LONGITUDE);
  float coralNS = METERS_PER_DEGREE_LATITUDE * (GPS.latitudeDegrees - CORAL_LATITUDE);
  distanceFromCoral = sqrt(coralEW * coralEW + coralNS * coralNS);
  angleFromCoral = atan2(coralEW, coralNS);

  float manEW = METERS_PER_DEGREE_LONGITUDE * (GPS.longitudeDegrees - MAN_LONGITUDE);
  float manNS = METERS_PER_DEGREE_LATITUDE * (GPS.latitudeDegrees - MAN_LATITUDE);
  distanceFromMan = sqrt(manEW * manEW + manNS * manNS);
  angleFromMan = atan2(manEW, manNS);
}

boolean parseGPS(unsigned long now) {

  if (!GPS.parse(GPS.lastNMEA()))
    return false;

  lastGPSReading = now;

  if (GPS.year >= 2018 && (lastMinute != GPS.minute || GPS.seconds != lastSecond))
    setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);

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
      updateRelativePosition();
    }
  } else if (haveFix) {
    Serial.println("Lost fix");
    haveFix = false;
  }
  return true;
}

boolean updateGPS(unsigned long now) {

  boolean anyPrinted = false;
  while (true) {
    char c = GPS.read();
    if (!c || GPS.newNMEAreceived()) break;
    if (ECHO_GPS) {
      if (!anyPrinted) {
        anyPrinted = true;
        Serial.println();
      }
      Serial.print(c);
    }
  }
  if (anyPrinted)
    Serial.println();

  // if a sentence is received, we can check the checksum, parse it...
  if ( GPS.newNMEAreceived() && parseGPS(now))
    return true;

  return false;
}

void logGPS(unsigned long now) {
  if (timer > now)  timer = now;
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
  } else if (haveFix)
    Serial.println("Lost fix");
}

