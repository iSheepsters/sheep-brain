#include <Arduino.h>

#include "GPS.h"
#include "radio.h"
#include "util.h"
#include "printf.h"

#include <math.h>

const boolean GPS_DEBUG = false;
boolean GPS_ready = false;
uint16_t badGPS = 0;
uint16_t total_good_GPS = 0;
uint16_t total_bad_GPS = 0;
long longestGPSvoid = 0;


int32_t lastLatitudeDegreesFixed = -1000;
int32_t lastLongitudeDegreesFixed = 0.0;
int32_t latitudeDegreesAvg;
int32_t longitudeDegreesAvg;

volatile boolean read_gps_in_interrupt = false;

Adafruit_GPS GPS(&Serial1);


boolean setupGPS() {
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  GPS.sendCommand("$PMTK251,115200*1F");
  unsigned long finish = 200 + millis();
  while (millis() < finish) {
    char c = GPS.read();
    if (c) Serial.print(c);
  }
  Serial1.end();
  delay(200);
  GPS.begin(115200);

  if (getGPSFixQuality)
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  else
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);


  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_2HZ);

  GPS.sendCommand(PMTK_ENABLE_SBAS);
  GPS.sendCommand(PMTK_ENABLE_WAAS);
  unsigned long now = millis();
  while (year() < 2018 && millis() < now + 4000) {
    delay(1);
    updateGPS();
  }
  total_good_GPS = total_bad_GPS = 0;
  GPS_ready = true;
  return year() >= 2018;
}

// The following are calculated for 41N
const float FEET_PER_DEGREE_LATITUDE = 364349.27;

const float FEET_PER_DEGREE_LONGITUDE = 276033.42;

float FIXED_TO_FLOAT = 10000000.0;
const float FEET_PER_DEGREE_LATITUDE_FIXED = FEET_PER_DEGREE_LATITUDE / FIXED_TO_FLOAT;

const float FEET_PER_DEGREE_LONGITUDE_FIXED =  FEET_PER_DEGREE_LONGITUDE / FIXED_TO_FLOAT;


const int32_t MAN_LATITUDE_FIXED      =    407864000;
const int32_t MAN_LONGITUDE_FIXED     =   -1192065000;

const int32_t CORRAL_LATITUDE_FIXED  =   407829257;
const int32_t CORRAL_LONGITUDE_FIXED = -1192062897;

const float CORRAL_LATITUDE  =   CORRAL_LATITUDE_FIXED / FIXED_TO_FLOAT;
const float CORRAL_LONGITUDE = CORRAL_LONGITUDE_FIXED / FIXED_TO_FLOAT;



// measured in feet
float distanceFromMan = -1;
float distanceFromCoral = -1;

// measured in radians; 0 = north, pi/2 = east
float angleFromCoral = 0;
float angleFromMan = 0;

uint32_t timer = millis();
boolean haveFix = false;
boolean anyFix = false;
uint32_t fixCount = 0;
unsigned long lastGPSReading;
const boolean ECHO_GPS = false;
unsigned long firstFix = 0;
uint16_t totalFixes = 0;

time_t BRC_now() {
  return now() - 7 * 60 * 60;
}

boolean haveFixNow() {
  return haveFix;
}

float distanceFromCorral(int sheep) {
  float corralEW = FEET_PER_DEGREE_LONGITUDE * (getSheep(sheep).longitude - CORRAL_LONGITUDE);
  float corralNS = FEET_PER_DEGREE_LATITUDE * (getSheep(sheep).latitude  - CORRAL_LATITUDE);
  return sqrt(corralEW * corralEW + corralNS * corralNS);
}

boolean inCorral(int sheep) {
  return distanceFromCorral(sheep) < 100;
}

float distanceToSheep(int sheep) {
  float EW = FEET_PER_DEGREE_LONGITUDE * (getSheep(sheep).longitude - getSheep().longitude );
  float NS = FEET_PER_DEGREE_LATITUDE * (getSheep(sheep).latitude  - getSheep().latitude);
  float distance = sqrt(EW * EW + NS * NS);
  return distance;

}
boolean isClose(int sheep) {
  return distanceToSheep(sheep) < 45;
}

boolean isRecent(int sheep) {
  if (sheep == sheepNumber) return true;
  if (getSheep(sheep).time == 0) return false;
  if (getSheep(sheep).time + 5 * RADIO_EPOC > now()) return true;
  return false;
}
boolean isInFriendlyTerritory() {
  return true; // no longer at BRC
  if (!anyFix)
    return true;
  if (inCorral(sheepNumber))
    return true;
  for (int s = 1; s <= NUMBER_OF_SHEEP; s++)
    if (s != sheepNumber && isRecent(s) && isClose(s)) {
      myprintf(Serial, "%d feet from corral, but sheep %d is nearby\n",
               (int) distanceFromCorral(sheepNumber), s);
      return true;
    }
  myprintf(Serial, "Not in friendly terriorty, %d feet from corral\n",
           (int) distanceFromCorral(sheepNumber));
  int h = hour(BRC_now());
  int m = minute(BRC_now());
  if (h < 2) {
    myprintf(Serial, "Time is %d:%02d, assuming we are shepherded\n", h, m);
    return true;
  }
  if (h > 16) {
    myprintf(Serial, "Time is %d:%02d, assuming we are shepherded\n", h, m);
    return true;
  }



  return false;
};

// If there are x sheep immediately around me, returns x.
// If the rest of the sheep are 20 feet away, returns 1+(x-1)/3;
// In corral with 12 sheep, will likely return 6 or more
float howCrowded() {
  if (minutesPerSheep > 0)
    return 0.5;
  if (!anyFix)
    return 2.0;
  float answer = 1.0;
  for (int s = 1; s <= NUMBER_OF_SHEEP; s++)
    if (s != sheepNumber && isRecent(s)) {
      float distance = distanceToSheep(s);
      myprintf(Serial, "Sheep %d at distance ", s);
      Serial.println(distance);
      answer += 10.0 / (10.0 + distance);
    }
  return answer;
}


float distance_between_fixed_in_feet(int32_t lat1, int32_t long1, int32_t lat2, int32_t long2) {
  float EW = FEET_PER_DEGREE_LONGITUDE_FIXED * (long1 - long2);
  float NS = FEET_PER_DEGREE_LATITUDE_FIXED * (lat1 - lat1);
  return sqrt(EW * EW + NS * NS);
}

void updateRelativePosition() {
  float corralEW = FEET_PER_DEGREE_LONGITUDE_FIXED * (longitudeDegreesAvg - CORRAL_LONGITUDE_FIXED);
  float corralNS = FEET_PER_DEGREE_LATITUDE_FIXED * (latitudeDegreesAvg - CORRAL_LATITUDE_FIXED);
  distanceFromCoral = sqrt(corralEW * corralEW + corralNS * corralNS);
  angleFromCoral = atan2(corralEW, corralNS);

  float manEW = FEET_PER_DEGREE_LONGITUDE_FIXED * (longitudeDegreesAvg  - MAN_LONGITUDE_FIXED);
  float manNS = FEET_PER_DEGREE_LATITUDE_FIXED * (latitudeDegreesAvg - MAN_LATITUDE_FIXED);
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

time_t lastGPSTime = 0;

// millis value for when lastGPSTime was recorded.

unsigned long lastGPSTimeAt = 0;

boolean parseGPS(unsigned long now) {
  char * txt = GPS.lastNMEA();
  if (strstr(txt, "$PMTK") || false && strstr(txt, "GPGGA")) {
    if (GPS_DEBUG)
      Serial.println(txt);
    return true;
  }
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

  if (difference < 0)
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
  if (difference > 0)
    setTime(thisGPSTime);

  if (GPS.fix) {
    if (lastLatitudeDegreesFixed != -1000 && (abs( GPS.latitude_fixed  - lastLatitudeDegreesFixed) > 1000 ||
        abs( GPS.longitude_fixed  - lastLongitudeDegreesFixed) > 1000)) {
      // too much change
      if (GPS_DEBUG)  {

        myprintf(Serial,  "Bad lat/long change from GPS string: \"%s\"\n . ", (txt + 1));
        Serial.print(lastLatitudeDegreesFixed);
        Serial.print(", ");
        Serial.print(lastLongitudeDegreesFixed);
        Serial.print(" -> ");
        Serial.print(GPS.latitude_fixed);
        Serial.print(" ");
        Serial.print(GPS.latitude_fixed);
        Serial.println();
      }


      lastLatitudeDegreesFixed = GPS.latitude_fixed;
      lastLongitudeDegreesFixed = GPS.longitude_fixed;
      return false;
    }
    if ( GPS.latitudeDegrees < 38 ||  GPS.latitudeDegrees > 41 || GPS.longitudeDegrees < -120
         || GPS.longitudeDegrees > -76)  {
      if (GPS_DEBUG)
        myprintf(Serial, "Bad lat/long from GPS string: \"%s\"\n", (txt + 1));
      return false;
    }


    lastLatitudeDegreesFixed = GPS.latitude_fixed;
    lastLongitudeDegreesFixed = GPS.longitude_fixed;
    if (haveFix) {
      fixCount++;
      int32_t latDiff = (lastLatitudeDegreesFixed - latitudeDegreesAvg);
      int32_t longDiff = (lastLongitudeDegreesFixed - longitudeDegreesAvg);
      latitudeDegreesAvg += latDiff / 4;
      longitudeDegreesAvg += longDiff / 4;
    } else {
      Serial.println("Acquired fix");
      if (firstFix == 0)
        firstFix = now;
      fixCount = 0;
      haveFix = true;
      latitudeDegreesAvg = lastLatitudeDegreesFixed;
      longitudeDegreesAvg =  lastLongitudeDegreesFixed;
    }
    getSheep().latitude = latitudeDegreesAvg / FIXED_TO_FLOAT;
    getSheep().longitude = longitudeDegreesAvg / FIXED_TO_FLOAT;
    updateRelativePosition();
    long diff = now -  lastGPSReading;
    if (lastGPSReading != 0 && longestGPSvoid < diff)
      longestGPSvoid = diff;
    lastGPSReading = now;
    anyFix = true;
    totalFixes++;
    if (totalFixes % 1000 == 0) {
      uint16_t minutes =  (now - firstFix) / 1000 / 60;
      myprintf(Serial, "%d GPS fixes per minute\n", totalFixes / minutes);
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
  updateGPSLatency();
  while (true) {
    char c = GPS.read();
    if (!c) break;
    c = GPS.read();
    if (!c) break;
    c = GPS.read();
    if (!c) break;
  }
  read_gps_in_interrupt = oldValue;
}



boolean updateGPS() {
  unsigned long now = millis();

  quickGPSUpdate();
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

    } else {
      latitudeDegreesAvg = (3 * latitudeDegreesAvg + GPS.latitude_fixed) / 4;
      longitudeDegreesAvg = (3 * longitudeDegreesAvg + GPS.longitude_fixed) / 4;
      fixCount++;

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

        Serial.println();

      }
    }
  } else if (haveFix) {
    Serial.println("Lost fix");
    fixCount = 0;
  }
}
