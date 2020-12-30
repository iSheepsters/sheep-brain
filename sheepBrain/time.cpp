#include <Arduino.h>

#include "time.h"
#include "printf.h"
#include "util.h"

#include <math.h>

// in 2021  at 2:00 AM on Sunday, March 14
// and ends at 2:00 AM on Sunday, November 7

DateTime DSTstarts2021 = DateTime(2021, 3, 14, 2, 0, 0);
DateTime DSTends2021 = DateTime(2021, 11, 7, 2, 0, 0);

RTC_DS3231 rtc;

uint16_t year() { return rtc.now().year(); }
 uint8_t month()  { return rtc.now().month(); }
 uint8_t day()  { return rtc.now().day(); }
 uint8_t hour()  { return rtc.now().hour(); }
 uint8_t minute()  { return rtc.now().minute(); }
 uint8_t second()  { return rtc.now().second(); }


bool setupTime() {
   if (rtc.begin()) {
    return true;
   }
    Serial.println("Couldn't find RTC");
    Serial.flush();
    return false;

}

 time_t now() {
  return  rtc.now().unixtime();
 }

bool isDST() {
DateTime now = rtc.now();   
return DSTstarts2021 <= now && now <= DSTends2021;
}

int adjustmentForEasternTime() {
  if (isDST()) return +1;
  return 0;
}

uint8_t adjustedHour() {
  int h = hour() + adjustmentForEasternTime() + timeZoneAdjustment;
  if (h < 0) h = h + 24;
  if (h > 23) h = h - 23;
  h = h % 24;
  return h;
}


time_t adjustedNow() {
  return time_t() + 60 * 60 * (timeZoneAdjustment + adjustmentForEasternTime());
}
