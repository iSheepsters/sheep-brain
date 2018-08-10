
#include <SD.h>
#include <TimeLib.h>

#include "all.h"
#include "gps.h"
#include "printf.h"


File logFile;

void setupLogging() {
  char fileName[100];

  unsigned long now = millis();
  while (GPS.year == 0 && millis() < now + 2000) {
    updateGPS(millis());
  }
  if (GPS.year == 0) updateGPS(millis());
  if (GPS.year == 0) {
    SD.mkdir("log/nodate");
    int i = 0;
    while (true) {
      sprintf(fileName, "log/nodate/%04d.csv", i);
      if (!SD.exists(fileName)) {
        logFile = SD.open(fileName, FILE_WRITE);
        myprintf(Serial, "Using logfile %s\n", fileName);
        return;
      }
      i = i + 1;
    }
  }
  sprintf(fileName, "log/%04d%02d%02d", 2000 + GPS.year, GPS.month, GPS. day);
  SD.mkdir(fileName);
  sprintf(fileName, "log/%04d%02d%02d/%02d%02d%02d.csv", 2000 + GPS.year, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
  if (SD.exists(fileName))
    myprintf(Serial, "Overwriting %s\n", fileName);
  else
    myprintf(Serial, "Using logfile %s\n", fileName);
  logFile = SD.open(fileName, FILE_WRITE);
  if (!logFile) {
    Serial.println("Attempt to open log file failed");
  }
}

void updateLog(unsigned long timeNow) {
  noInterrupts();
  myprintf(logFile, "%d,%d, ", sheepNumber, InfoPacket);

  myprintf(logFile, "%d/%d/%d %d:%d:%d,%d, ",
           GPS.month, GPS.day, GPS.year, GPS.hour, GPS.minute, GPS.seconds,
           now());
  myprintf(logFile, "%d,%d,", timeNow / 1000 / 3600, batteryVoltageRaw());
  logFile.print(GPS.latitudeDegrees, 7);
  logFile.print(",");
  logFile.print(GPS.longitudeDegrees, 7);
  logFile.print(",");

  logFile.println();
  //logFile.flush();
  interrupts();


}



void updateLog(unsigned long timeNow) {
  noInterrupts();
  myprintf(logFile, "%d,%d, ", sheepNumber, InfoPacket);

  myprintf(logFile, "%d/%d/%d %d:%d:%d,%d, ",
           GPS.month, GPS.day, GPS.year, GPS.hour, GPS.minute, GPS.seconds,
           now());
  myprintf(logFile, "%d,%d,", timeNow / 1000 / 3600, batteryVoltageRaw());
  logFile.print(GPS.latitudeDegrees, 7);
  logFile.print(",");
  logFile.print(GPS.longitudeDegrees, 7);
  logFile.print(",");

  logFile.println();
  //logFile.flush();
  interrupts();
}


void logRadioUpdate(uint8_t sheepNum, SheepInfo & info) {
  noInterrupts();
  time_t when = info.time;
  
  myprintf(logFile, "%d,%d, ", sheepNum, RadioInfoPacket);

  myprintf(logFile, "%d/%d/%d %d:%d:%d,%d, ",
           month(when), day(when), year(when)%100,  hour(when), minute(when), seconds(when),
           when);
  myprintf(logFile, "%d,%d,", info.uptimeMinutes, info.batteryVoltageRaw);
  logFile.print(info.latitude, 7);
  logFile.print(",");
  logFile.print(info.longitude, 7);
  logFile.print(",");

  logFile.println();
  //logFile.flush();
  interrupts();
}
void logDistress(const char *fmt, ... ) {
  char buf[500]; // resulting string limited to 256 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, 500, fmt, args);
  va_end (args);
  Serial.print("Distress message ");
  Serial.println(buf);
  if (logFile) {
    noInterrupts();
    myprintf(logFile, "%d,%d, ", sheepNumber, DistressPacket);
    myprintf(logFile, "%d/%d/%d %d:%d:%d,%d,\"",
             GPS.month, GPS.day, GPS.year, GPS.hour, GPS.minute, GPS.seconds,
             now());
    logFile.print(buf);
    logFile.print("\"");
    //logFile.flush();
    interrupts();
    distressPacket(msg);
  }
}


