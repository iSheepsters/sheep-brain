#include <i2c_t3.h>
#include "printf.h"

void setupComm();
extern unsigned long timeSinceLastMessage();

extern volatile boolean receivedMsg;
extern unsigned long lastMsg;
extern unsigned long activityReports;
