#include "util.h"
#include <TimeLib.h>
#include "GPS.h"
#include "printf.h"

bool isOpen(bool debug) {

  time_t time = adjustedNow();
  tmElements_t openAt;
  tmElements_t closeAt;
  breakTime(time, openAt);
  breakTime(time, closeAt);
  openAt.Minute = 0;
  openAt.Second = 0;
  closeAt.Minute = 0;
  closeAt.Second = 0;
  int thisMonth = month(time);
  int thisDay = day(time);
  if (debug) {
    myprintf(Serial, "isOpen %d %d\n", thisMonth, thisDay);
  }
  if (thisMonth == 1 and thisDay == 1) {
    openAt.Hour = 10; closeAt.Hour = 6;
  } else if (thisMonth == 12)
    switch (thisDay) {
      case 15: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 10;
          break;
        }
      case 16: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 10;
          break;
        }
      case 17: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 10;
          break;
        }
      case 18: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 10;
          break;
        }
      case 19: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 20: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 21: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 22: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 23: {
          openAt.Hour = 9;
          closeAt.Hour = 12 + 11;
          break;
        }
      case 24: {
          openAt.Hour = 8;
          closeAt.Hour = 12 + 5;
          break;
        }
      case 25: {
          return false;
        }
      case 26: {
          openAt.Hour = 8;
          closeAt.Hour = 12 + 9;
          closeAt.Minute = 30;
          break;
        }
      case 27: {
          openAt.Hour = 10;
          closeAt.Hour = 12 + 9;
          closeAt.Minute = 30;
          break;
        }
      case 28: {
          openAt.Hour = 10;
          closeAt.Hour = 12 + 9;
          closeAt.Minute = 30;
          break;
        }
      case 29: {
          openAt.Hour = 10;
          closeAt.Hour = 12 + 9;
          closeAt.Minute = 30;
          break;
        }
      case 30: {
          openAt.Hour = 11;
          closeAt.Hour = 12 + 7;
          break;
        }
      case 31: {
          openAt.Hour = 10;
          closeAt.Hour = 12 + 6;
          break;
        }
      default : {
          if (weekday(time) == 1) {
            // Sunday
            openAt.Hour = 11; closeAt.Hour = 12 + 7;
          } else {
            openAt.Hour = 10; closeAt.Hour = 12 + 9; closeAt.Minute = 30;
          }
          break;
        }
    }
  else {
    if (weekday(time) == 1) {
      // Sunday
      openAt.Hour = 11; closeAt.Hour = 12 + 7;
    } else {
      openAt.Hour = 10; closeAt.Hour = 12 + 9; closeAt.Minute = 30;
    }
  }
  time_t oTime = makeTime(openAt);
  time_t cTime = makeTime(closeAt);

  if (debug) {
    myprintf(Serial, "open at: %d\n", oTime);
    myprintf(Serial, "close at: %d\n", cTime);
    myprintf(Serial, "now: %d\n", time);
    myprintf(Serial, "Date: %d/%d\n", thisMonth, thisDay);

    if (oTime  > time)

      myprintf(Serial, "open in %d minutes\n", (oTime  - time) / 60);

    else
      myprintf(Serial, "opened %d minutes ago\n", -(oTime  - time) / 60);
    if (cTime  > time)

      myprintf(Serial, "close in %d minutes\n", (cTime - time) / 60);

    else
      myprintf(Serial, "closed %d minutes ago\n", -(cTime - time) / 60);
  }
  const time_t onBeforeOpening = 0 * 60;
  const time_t onAfterClosing = 0 * 60;
  if (oTime - onBeforeOpening <= time && time <= cTime + onAfterClosing)
    return true;
  return false;

}
