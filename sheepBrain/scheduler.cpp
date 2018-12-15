#include <Arduino.h>
#include "scheduler.h"
#include "printf.h"
#include "util.h"
#include "comm.h"


const int MAX_ACTIVITIES = 10;
const int ACTIVITY_NAME_LENGTH = 20;

struct ScheduledActivity {
  unsigned long lastStarted = 0;
  uint16_t periodMS;
  activityPointer func;
  char name[ACTIVITY_NAME_LENGTH];
  uint16_t lastDuration;
};

int numScheduledActivities = 0;

volatile uint8_t currentActivity = 255;
volatile unsigned long currentActivityStartedAt = 0;
ScheduledActivity scheduledActivities[MAX_ACTIVITIES];

void addScheduledActivity(uint16_t periodMS, void (*func)(), const char * name) {
  if (numScheduledActivities >= MAX_ACTIVITIES) {
    myprintf(Serial, "Already have %d activites scheduled, can't schedule %s\n",
             numScheduledActivities, name);
    return;
  }
  ScheduledActivity & s = scheduledActivities[numScheduledActivities++];
  s.periodMS = periodMS;
  s.func = func;
  strncpy(s.name, name, ACTIVITY_NAME_LENGTH);
  s.name[ACTIVITY_NAME_LENGTH - 1] = 0;
}

void startSchedule() {
  unsigned long now = millis();

  for (int i = 0; i < numScheduledActivities; i++) {
    ScheduledActivity & s = scheduledActivities[i];
    s.lastStarted = now;
  }
}
void listScheduledActivities() {
  myprintf(Serial, "%-2s %-5s %s\n", "#", "ms", "name");
  for (int i = 0; i < numScheduledActivities; i++) {
    ScheduledActivity & s = scheduledActivities[i];
    myprintf(Serial, "%2d %5d %s\n", i, s.periodMS, s.name);
  }
}

void runScheduledActivities() {
  unsigned long now = millis();
  for (int i = 0; i < numScheduledActivities; i++) {

    ScheduledActivity & s = scheduledActivities[i];
    if (s.lastStarted + s.periodMS < now) {
      uint16_t oldTotalYield = totalYield;
      boolean first = s.lastStarted == 0;
      s.lastStarted = now;
      if (useSlave)
        sendActivity(i);
      currentActivity  = i;
      currentActivityStartedAt = now;
      s.func();
      now = millis();
      unsigned long duration = now - s.lastStarted;
     if (printInfo()) if (first || duration > 15 || totalYield - oldTotalYield > 5) {
        myprintf(Serial, "%2dms, %2dms yield for %s\n", duration, totalYield - oldTotalYield, s.name);
      }
      s.lastDuration = now - s.lastStarted;
    }
  }
}
