#include <Arduino.h>

#include "GPS.h"

Adafruit_GPS GPS(&Serial1);


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

  return Serial1.available() > 0;

}




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

boolean updateGPS(unsigned long now) {

  while (GPS.read()) {
    // already processed above
  }
  // if a sentence is received, we can check the checksum, parse it...
  if ( GPS.newNMEAreceived() && GPS.parse(GPS.lastNMEA())) {
    lastGPSReading = now;
    setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
    return true;
  }
  return false;
}

void logGPS(unsigned long now) {
  // if millis() or timer wraps around, we'll just reset it
  if (timer > now)  timer = now;
  if (GPS.fix) {
    if (!haveFix) {
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
  }
}

