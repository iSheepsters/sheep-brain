

#include <Adafruit_VS1053.h>

extern Adafruit_VS1053_FilePlayer musicPlayer;

extern int8_t thevol;
extern void stopMusic();
extern void completeMusic();

extern boolean setVolume(int8_t v);
extern void setupSound();
extern unsigned long lastSound;

extern void updateSound(unsigned long now);
extern void slowlyStopMusic();
extern unsigned long lastSoundStarted;

extern boolean playFile(const char *fmt, ... );


