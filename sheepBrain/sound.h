

#include <Adafruit_VS1053.h>

extern Adafruit_VS1053_FilePlayer musicPlayer;


extern int8_t thevol;

extern boolean setVolume(int8_t v);
extern void setupSound();
extern unsigned long lastSoundPlaying;

extern void updateSound(unsigned long now);
extern void slowlyStopMusic();
extern unsigned long lastSoundStarted;
extern volatile boolean musicPlayerReady;

extern void noteEndOfMusic();

extern boolean playFile(const char *fmt, ... );



