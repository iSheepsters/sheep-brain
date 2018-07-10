
#include <SD.h>

extern void setupSD();
extern void printDirectory(File dir, int numTabs);

class SoundFile {
  public:
    char name[13];
    uint32_t lastStarted;
    uint32_t lastPlaying;
    uint32_t duration;
    boolean eligibleToPlay(uint32_t);
};

class SoundCollection {
  public:
    SoundCollection();
    char name[13];
    uint16_t count;
    SoundFile *files;

    boolean load(const char * s);
    SoundFile* chooseSound(uint32_t now);
};


extern SoundFile * currentSoundFile;
