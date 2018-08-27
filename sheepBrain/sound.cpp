#include "Arduino.h"
#include "util.h"
#include "gps.h"
#include "state.h"
#include "printf.h"
#include <Adafruit_SleepyDog.h>

const boolean USE_AMPLIFIER = true;

#include "sound.h"
#include "soundFile.h"

#include <Wire.h>
// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)

// Feather M0 or 32u4
#if defined(__AVR__) || defined(ARDUINO_SAMD_FEATHER_M0)
#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

#endif

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);


// 0x4B is the default i2c address
#define MAX9744_I2CADDR 0x4B

// We'll track the volume level in this variable.
int8_t thevol = 55;
uint8_t VS1053_volume = 0;
unsigned long lastSoundStarted = 0;
unsigned long lastSoundPlaying = 0;
boolean ampOn = false;

boolean wasPlayingMusic;

// note: 0 is full volume
void musicPlayerSetVolume(uint8_t v) {
  ampOn = true;
  VS1053_volume = v;
  // right channel is always silent, we don't use it
  musicPlayer.setVolume(v, 0xfe);
  setAmpVolume(thevol);
}
void musicPlayerFullVolume() {
  ampOn = true;
  Serial.println("musicPlayerFullVolume");
  VS1053_volume = 0;
  musicPlayer.setVolume(0, 0xfe);
  setAmpVolume(thevol);
}
void musicPlayerNoVolume() {
  ampOn = false;
  Serial.println("musicPlayerNoVolume");
  VS1053_volume = 0;
  musicPlayer.setVolume(0xfe, 0xfe);
  setAmpVolume(0);
}

volatile boolean musicPlayerReady = false;
void setupSound() {
  if (true) {
    if (! setAmpVolume(0))
      Serial.println("Failed to set volume, MAX9744 not found!");
    else
      Serial.println("MAX9744 found");
  }


  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }

  Serial.println(F("VS1053 found"));
  if (! setAmpVolume(thevol))
    Serial.println("Failed to set volume, MAX9744 not found!");
  else
    Serial.println("MAX9744 found");


  musicPlayerFullVolume();
  if (ampOn)
    Serial.println("amp on");
  else
    Serial.println("amp off");
  myprintf(Serial, "VS1053_volume %d\n", VS1053_volume);

  musicPlayer.sineTest(0x44, 100);    // Make a tone to indicate VS1053 is working
  Serial.println(F("sineTest complete"));


  // musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  wasPlayingMusic = false;
  musicPlayerReady = true;
}



void updateSound(unsigned long now) {
  if (musicPlayer.playingMusic) {
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
boolean setAmpVolume(int8_t v) {
  if (!USE_AMPLIFIER) {
    Serial.println("Skipping amplifier");
    return false;
  }
  // cant be higher than 63 or lower than 0
  if (v > 63) v = 63;

  Serial.print("Setting volume to ");
  Serial.println(v);
  Wire.beginTransmission(MAX9744_I2CADDR);
  Wire.write(v);
  if (Wire.endTransmission() == 0)
    return true;
  else
    return false;
}


void slowlyStopMusic() {
  if (musicPlayer.playingMusic) {
    for (int i = VS1053_volume + 8; i < 256; i += 8) {
      musicPlayerSetVolume(i);
      yield(10);
    }
    Serial.println("Music slowly stopped");
    musicPlayer.stopPlaying();
  }
  noteEndOfMusic();
}



void setNextAmbientSound(unsigned long durationOfLastSound) {
  if (currentSheepState->state == Violated) {
    unsigned long result = + random(8000, 15000);
    nextAmbientSound = millis()  + result;
    myprintf(Serial, "violated; next ambient sound in %d ms\n", result);

    return;
  }

  if (durationOfLastSound < 6000) durationOfLastSound = 6000;
  else if (durationOfLastSound > 30000) durationOfLastSound = 30000;
  float crowded = howCrowded();
  Serial.print("Crowding: ");
  Serial.println(crowded);

  unsigned long result = durationOfLastSound / 2 + (unsigned long)(random(20000, 40000) * crowded);
  myprintf(Serial, "next ambient sound in %d ms\n", result);
  nextAmbientSound = millis() + result;
}

void noteEndOfMusic() {
  unsigned long now = millis();
  if (musicPlayer.playingMusic) {
    Serial.println("Music still playing");
    return;
  }
  if (!wasPlayingMusic)
    return;
  Serial.println("note end of music");
  wasPlayingMusic = false;
  musicPlayerNoVolume();

  boolean isBaa = true;
  if (currentSoundFile) {
    currentSoundFile->lastPlaying = now;
    if (currentSoundFile->duration == 0) {
      currentSoundFile->duration = currentSoundFile->lastPlaying - currentSoundFile->lastStarted;
      myprintf(Serial, "%d ms for %s/%s\n",
               currentSoundFile->duration,
               currentSoundFile->collection->name,
               currentSoundFile->name);
      isBaa = currentSoundFile->collection == &baaSounds;
      currentSoundFile = NULL;
    }

  }
  if (!isBaa) {
    setNextAmbientSound(lastSoundPlaying - lastSoundStarted);
    nextBaa = now + random(10000, 20000) * howCrowded();
    myprintf(Serial, "ambient ended, next sound in %d seconds\n",
             (nextAmbientSound - now) / 1000);
  }
  nextBaa = millis() + random(10000, 20000) * howCrowded();
  if (nextAmbientSound < now + 4000)
    nextAmbientSound = now + 4000;
  myprintf(Serial, "next baa in %d seconds, next ambient in %d\n",
           (nextBaa - now) / 1000, (nextAmbientSound - now) / 1000);
}



boolean playFile(const char *fmt, ... ) {
  slowlyStopMusic();

  char buf[256]; // resulting string limited to 256 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, 256, fmt, args);
  va_end (args);
  Serial.print("Playing ");
  Serial.println(buf);

  lastSoundStarted = millis();
  boolean result =  musicPlayer.startPlayingFile(buf);
  if (result)
    currentSoundPriority = 0;

  return result;

}


