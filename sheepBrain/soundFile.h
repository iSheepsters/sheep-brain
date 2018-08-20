#ifndef _SOUNDFILE_H
#define _SOUNDFILE_H
#include <SD.h>

extern boolean setupSD();
extern void printDirectory(File dir, int numTabs);
extern boolean soundPlayedRecently(unsigned long now);

class SoundFile {
  public:
    SoundFile() : lastStarted(0), lastPlaying(0), duration(0) {
      name[0] = '\0';
    };
    char name[13];
    uint32_t lastStarted;
    uint32_t lastPlaying;
    uint32_t duration;
    boolean eligibleToPlay(unsigned long now,  boolean ambientSound);
};

class SoundCollection {
  public:
    SoundCollection(uint8_t pri);
    char name[13];
    uint16_t count;
    const uint8_t priority; // higher priority sounds will preempt lower priority sounds
    SoundFile *files;


    void list();
    void verboseList(unsigned long now,  boolean ambientSound);

    boolean load(const char * s);
    SoundFile* chooseSound(unsigned long now,  boolean ambientSound);
    boolean playSound(unsigned long now,  boolean ambientSound);
};


extern SoundFile * currentSoundFile;

extern unsigned long nextRandomSound;

extern SoundCollection boredSounds, ridingSounds, welcomingSounds, baaSounds;

#endif

