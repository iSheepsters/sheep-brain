#include <SD.h>
extern uint8_t sheepNumber;
const uint8_t NUMBER_OF_SHEEP = 13;
extern File logFile;
extern uint16_t batteryVoltageRaw();
extern float batteryVoltage();
extern uint8_t batteryCharge();
extern uint16_t minutesUptime();

extern void setupDelay(uint16_t ms);
extern void yield(uint16_t ms);

enum State {
  Bored,
  Welcoming,
  Riding
};

extern enum State currState;

class __attribute__ ((packed)) SheepInfo {
  public:
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
};


extern SheepInfo & getSheep(int s);

extern SheepInfo & getSheep();
