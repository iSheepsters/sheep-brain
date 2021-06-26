#include "Arduino.h"
#include "util.h"
#include "state.h"
#include "printf.h"
#include "scheduler.h"
#include "time.h"
//#include <MemoryFree.h>
#include <Adafruit_SleepyDog.h>

const boolean USE_AMPLIFIER = true;

boolean rightSpeakerOn = false;
#include "sound.h"
#include "soundFile.h"


#include <Wire.h>
// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)


#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

const int RIGHT_SPEAKER_DECREASE = 5;

int getRightSpeakerSetting(int leftSpeakerSetting) {
  return constrain(leftSpeakerSetting + RIGHT_SPEAKER_DECREASE, 0, 0xfe);
}

// 0x4B is the default i2c address
#define MAX9744_I2CADDR 0x4B

// We'll track the volume level in this variable.
int8_t thevol = INITIAL_AMP_VOL;
uint8_t VS1053_volume = 0;
unsigned long lastSoundStarted = 0;
unsigned long lastSoundPlaying = 0;

boolean wasPlayingMusic;

// note: 0 is full volume
void musicPlayerSetVolume(uint8_t v) {

  VS1053_volume = v;
  if (currentSheepState->state == Bored && false) {
    musicPlayer.setVolume(v, 0xfe);
    rightSpeakerOn = false;
  } else {
    int right = getRightSpeakerSetting(v);
      myprintf(Serial, "VS1053_volume %d, %d\n", v, right);
    musicPlayer.setVolume(v, right);
    rightSpeakerOn = true;
  }
}
void musicPlayerFullVolume() {
  musicPlayerSetVolume(0);
}
void musicPlayerNoVolume() {
  Serial.println("musicPlayerNoVolume");
  VS1053_volume = 0;
  rightSpeakerOn = false;
  musicPlayer.setVolume(0xfe, 0xfe);
}

volatile boolean musicPlayerReady = false;
void setupSound() {


  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }

  Serial.println(F("VS1053 found"));

  musicPlayerFullVolume();
  myprintf(Serial, "VS1053_volume %d\n", VS1053_volume);

  musicPlayer.sineTest(0x44, 50);    // Make a tone to indicate VS1053 is working
  Serial.println(F("sineTest complete"));

  // musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  wasPlayingMusic = false;
  musicPlayerReady = true;
  if (playSound)
    addScheduledActivity(100, updateSound, "sound");
}

void updateSound() {
  if (musicPlayer.playingMusic) {
    unsigned long now = millis();
    wasPlayingMusic = true;
    if (currentSoundFile) {
      currentSoundFile->lastPlaying = now;
    }
    lastSoundPlaying = now;
  } else  if (wasPlayingMusic) {
    noteEndOfMusic();
  }
}

// Setting the volume is very simple! Just write the 6-bit
// volume to the i2c bus. That's it!




void slowlyStopMusic() {
  if (musicPlayer.playingMusic) {
    for (int i = VS1053_volume + 8; musicPlayer.playingMusic && i < 256; i += 8) {
      musicPlayerSetVolume(i);
      musicPlayer.feedBuffer();
      yield(10);
    }
    if (printInfo())
      Serial.println("Music slowly stopped");
    musicPlayer.stopPlaying();
  }
  noteEndOfMusic();
}



void setNextAmbientSound(unsigned long durationOfLastSound) {
  if (currentSheepState->state == Violated) {
    unsigned long result = + random(8000, 15000);
    nextAmbientSound = millis()  + result;
    if (printInfo())
      myprintf(Serial, "violated; next ambient sound in %d ms\n", result);

    return;
  }

  if (durationOfLastSound < 6000) durationOfLastSound = 6000;
  else if (durationOfLastSound > 30000) durationOfLastSound = 30000;
  


  unsigned long result = durationOfLastSound / 2 + (unsigned long)(random(20000, 40000) );
  if (printInfo()) myprintf(Serial, "next ambient sound in %d ms\n", result);
  nextAmbientSound = millis() + result;
}

void noteEndOfMusic() {
  unsigned long now = millis();
  if (musicPlayer.playingMusic) {
    if (printInfo())Serial.println("Music still playing");
    return;
  }
  if (!wasPlayingMusic)
    return;
  if (printInfo())Serial.println("note end of music");
  wasPlayingMusic = false;
  musicPlayerNoVolume();

  boolean isBaa = true;
  if (currentSoundFile) {
    currentSoundFile->lastPlaying = now;
    if (currentSoundFile->duration == 0) {
      currentSoundFile->duration = currentSoundFile->lastPlaying - currentSoundFile->lastStarted;
      if (printInfo()) myprintf(Serial, "%d ms for %s/%s\n",
                                  currentSoundFile->duration,
                                  currentSoundFile->collection->name,
                                  currentSoundFile->name);
      isBaa = currentSoundFile->collection == &baaSounds;
      currentSoundFile = NULL;
    }

  }
  if (!isBaa) {
    setNextAmbientSound(lastSoundPlaying - lastSoundStarted);
    nextBaa = now + random(10000, 20000) ;
    if (printInfo()) myprintf(Serial, "ambient ended, next sound in %d seconds\n",
                                (nextAmbientSound - now) / 1000);
  }
  nextBaa = millis() + random(10000, 20000) ;
  if (nextAmbientSound < now + 4000)
    nextAmbientSound = now + 4000;
  if (printInfo()) myprintf(Serial, "next baa in %d seconds, next ambient in %d\n",
                              (nextBaa - now) / 1000, (nextAmbientSound - now) / 1000);
}



boolean playFile(const char *fmt, ... ) {
  unsigned long start = micros();
  slowlyStopMusic();

  char buf[256]; // resulting string limited to 256 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, 256, fmt, args);
  va_end (args);
  if (printInfo()) {
    Serial.print("Playing ");
    Serial.println(buf);
  }
   

  lastSoundStarted = millis();
  boolean result = musicPlayer.startPlayingFile(buf);
  if (result)
    currentSoundPriority = 0;
  else
    SD.errorPrint(&Serial);

  unsigned long duration = micros() - start;
  if (duration > 10000 && printInfo())
    myprintf(Serial, "%5dus to start %s\n", duration, buf);
  return result;

}
