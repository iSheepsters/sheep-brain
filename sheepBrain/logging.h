#include "util.h"

extern void setupLogging();
extern void updateLog(unsigned long now);
extern void logDistress(const char *fmt, ... );
extern void logRadioUpdate(uint8_t sheepNum, SheepInfo & info);
extern void logRadioDistress(uint8_t sheepNum, time_t when, SheepInfo & info, char* buf);
extern File logFile;
extern void optionalFlush();
extern void logBoot();
