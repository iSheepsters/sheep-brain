

#include <Adafruit_VS1053.h>

extern Adafruit_VS1053_FilePlayer musicPlayer;

extern int8_t thevol;
extern void stopMusic();
extern void completeMusic();

extern boolean setVolume(int8_t v);
extern void setupSound();
extern unsigned long lastSound = 0;

extern void updateSound(unsligned long now);
extern unsigned long lastSoundStarted;

extern boolean playFile(const char *fmt, ... );


