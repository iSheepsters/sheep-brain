extern void setupLogging();
extern void updateLog(unsigned long now);
extern void logDistress(const char *fmt, ... );
void logRadioUpdate(uint8_t sheepNum, SheepInfo & info);
