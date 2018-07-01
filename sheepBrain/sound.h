
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>

extern Adafruit_VS1053_FilePlayer musicPlayer;
extern void stopMusic();
extern void completeMusic();
extern void printDirectory(File dir, int numTabs);
extern boolean setvolume(int8_t v) ;
extern void setupSound();

extern boolean playFile(const char *fmt, ... );


