#include <Arduino.h>

#include "time.h"
#include "printf.h"
#include "util.h"

#include <math.h>


bool isDST() {
  time_t time = now() - 4;
  if (year(time) == 2018) return false;
  int m = month(time);
  if (m < 3) return false;
  if (m == 3) return day(time) >= 10;
  if (m > 3 && m < 11) return true;
  if (m == 11) return day(time) < 3;
  return false;
}

int adjustmentForEasternTime() {
  if (isDST()) return -4;
  return -5;
}

uint8_t adjustedHour() {
  int h = hour() + adjustmentForEasternTime() + timeZoneAdjustment;
  if (h < 0) h = h + 24;
  if (h > 23) h = h - 23;
  h = h % 24;
  return h;
}


time_t adjustedNow() {
  return now() + 60 * 60 * (timeZoneAdjustment + adjustmentForEasternTime());
}

time_t BRC_now() {
  return now() - 7 * 60 * 60;
}
