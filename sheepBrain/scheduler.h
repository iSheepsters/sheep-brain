
typedef void (* activityPointer) ();

void addScheduledActivity(uint16_t periodMS, activityPointer func, const char * name);
void startSchedule();
void runScheduledActivities();
void listScheduledActivities();
extern volatile uint8_t currentActivity;
extern volatile unsigned long currentActivityStartedAt;
