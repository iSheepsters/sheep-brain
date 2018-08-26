#ifndef _SOUNDFILE_H
#define _SOUNDFILE_H
#include <SD.h>

extern boolean setupSD();
extern void printDirectory(File dir, int numTabs);
extern boolean soundPlayedRecently(unsigned long now);

 class SoundCollection;

class SoundFile {
  public:
    SoundFile() : lastStarted(0), lastPlaying(0), duration(0) {
      name[0] = '\0';
    };
    char name[13];
    uint32_t lastStarted;
    uint32_t lastPlaying;
    uint32_t duration;
    SoundCollection * collection;
    boolean eligibleToPlay(unsigned long now,  boolean ambientSound);
};

class SoundCollection {
  public:
    SoundCollection(uint8_t pri);
    char name[13];
    uint16_t count;
    uint16_t lastChoice;
    boolean common = false;
    const uint8_t priority; // higher priority sounds will preempt lower priority sounds
    SoundFile *files;


    void list();
    void verboseList(unsigned long now,  boolean ambientSound);

    boolean load(const char * s);
    boolean loadCommon(const char * s);
    boolean load(File f);
    SoundFile* chooseSound(unsigned long now,  boolean ambientSound);
    boolean playSound(unsigned long now,  boolean ambientSound);
};


extern SoundFile * currentSoundFile;
extern int currentSoundPriority;

extern unsigned long nextAmbientSound;

extern SoundCollection boredSounds;
extern SoundCollection ridingSounds;
extern SoundCollection readyToRideSounds;
extern SoundCollection endOfRideSounds;
extern SoundCollection attentiveSounds;
extern SoundCollection notInTheMoodSounds;
extern SoundCollection violatedSounds;
extern SoundCollection inappropriateTouchSounds;
extern SoundCollection seperatedSounds;
extern SoundCollection baaSounds;

#endif

