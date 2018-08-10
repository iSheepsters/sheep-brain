#include <SD.h>
extern uint8_t sheepNumber;
const uint8_t NUMBER_OF_SHEEP = 13;
extern File logFile;
extern uint16_t batteryVoltageRaw();

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
};


extern SheepInfo & getSheep(int s);

extern SheepInfo & getSheep();
