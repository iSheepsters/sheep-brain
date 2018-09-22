#ifndef _UTIL_H
#define _UTIL_H
#include <SD.h>
extern uint8_t sheepNumber;
const uint8_t NUMBER_OF_SHEEP = 16;
const uint8_t PLACEHOLDER_SHEEP = 15;
const uint8_t INITIAL_AMP_VOL = 47;
extern File logFile;
extern uint16_t batteryVoltageRaw();
extern float batteryVoltage();
extern uint8_t batteryCharge();
extern uint16_t minutesUptime();
extern void writeSheepNumber(int s);
extern uint16_t totalYield;

extern void setupDelay(uint16_t ms);
extern void yield(uint16_t ms);

extern uint8_t millisToSecondsCapped(unsigned long ms);

extern unsigned long updateGPSLatency();

const int minutesPerSheep = 0; // use 0 to not switch sheep
const boolean useSound = true;
const boolean playSound = true;
const boolean useTouch = true;
const boolean useGPS = true;
const boolean useGPSinterrupts = true;
const boolean useSlave = true;
const boolean useRadio = true;
const boolean useLog = true;
const boolean useCommands = false;
const boolean doUpdateState = true;
const boolean getGPSFixQuality = false;


class __attribute__ ((packed)) SheepInfo {
  public:
    SheepInfo() : time(0) {};
    float latitude, longitude;
    time_t time;
    uint16_t uptimeMinutes;
    uint16_t batteryVoltageRaw;
    uint16_t errorCodes;
};


enum PacketKind {
  InfoPacket,
  DistressPacket,
  RadioInfoPacket,
  RadioDistressPacket,
  CommandPacket,
};


extern SheepInfo & getSheep(int s);

extern SheepInfo & getSheep();

extern int sheepToSwitchTo(int s);

#endif
