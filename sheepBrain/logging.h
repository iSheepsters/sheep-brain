#include "util.h"

extern void setupLogging();
extern void updateLog(unsigned long now);
extern void logDistress(const char *fmt, ... );
extern File logFile;
extern void optionalFlush();
extern void logBoot();
