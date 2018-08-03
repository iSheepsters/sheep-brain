
#include <SD.h>

extern void setupSD();
extern void printDirectory(File dir, int numTabs);

class SoundFile {
  public:
    SoundFile() : lastStarted(0), lastPlaying(0), duration(0) {
      name[0] = '\0';
    };
    char name[13];
    uint32_t lastStarted;
    uint32_t lastPlaying;
    uint32_t duration;
    boolean eligibleToPlay(uint32_t,  boolean quietTime);
};

class SoundCollection {
  public:
    SoundCollection();
    char name[13];
    uint16_t count;
    SoundFile *files;


    void list();

    boolean load(const char * s);
    SoundFile* chooseSound(uint32_t now,  boolean quietTime);
    void playSound(uint32_t now,  boolean quietTime);
};


extern SoundFile * currentSoundFile;

