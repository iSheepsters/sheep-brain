#ifndef _UTIL_H
#define _UTIL_H


#include <SdFat.h>
extern uint8_t sheepNumber;
const uint8_t NUMBER_OF_SHEEP = 16;
const uint8_t PLACEHOLDER_SHEEP = 15;
const uint8_t INITIAL_AMP_VOL = 50;
extern File logFile;
extern uint16_t batteryVoltageRaw();
extern float batteryVoltage();
extern uint8_t batteryCharge();
extern uint16_t minutesUptime();
extern void writeSheepNumber(int s);
extern uint16_t totalYield;

extern void setupDelay(uint16_t ms);
extern void yield(uint16_t ms);

extern bool printInfo();

extern uint8_t millisToSecondsCapped(unsigned long ms);

const int minutesPerSheep = 0; // use 0 to not switch sheep
const boolean useSound = true;
const boolean playSound = true;
const boolean useTouch = true;

const boolean useSlave = true;

const boolean useLog = true;
const boolean useCommands = true;
const boolean doUpdateState = true;

extern boolean debugTouch;
extern boolean plotTouch;


class __attribute__ ((packed)) SheepInfo {
  public:
    SheepInfo() : time(0) {};
    time_t time;
    uint16_t uptimeMinutes;
    uint16_t batteryVoltageRaw;
    uint16_t errorCodes;
};


enum PacketKind {
  InfoPacket,
  DistressPacket,
  CommandPacket,
};


class TimeAdjustment {
  public:
  uint8_t hourStart;
  uint8_t hoursLong;
  uint8_t volume;
};

extern int8_t timeZoneAdjustment;
extern boolean privatesEnabled;
extern boolean allowRrated;
extern uint8_t numTimeAdjustments;
extern  TimeAdjustment* timeAdjustments;
extern SdFat SD;
extern SheepInfo & getSheep(int s);

extern SheepInfo & getSheep();

extern int sheepToSwitchTo(int s);

#endif
