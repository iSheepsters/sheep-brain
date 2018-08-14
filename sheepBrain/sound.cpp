#include "Arduino.h"
#include "util.h"
#include <Adafruit_SleepyDog.h>

const boolean USE_AMPLIFIER = false;

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
int8_t thevol = 50;

unsigned long lastSoundStarted = 0;
unsigned long lastSound = 0;



void musicPlayerFullVolume() {
  musicPlayer.setVolume(0, 0);
}
void setupSound() {
  if (false) {
    if (! setVolume(0))
      Serial.println("Failed to set volume, MAX9744 not found!");
    else
      Serial.println("MAX9744 found");
  }

  Serial.println("\n\nAdafruit VS1053 Musicmaker Feather Test");

  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }

  Serial.println(F("VS1053 found"));
  if (! setVolume(thevol))
    Serial.println("Failed to set volume, MAX9744 not found!");
  else
    Serial.println("MAX9744 found");


  unsigned long start = millis();
  musicPlayer.sineTest(0x44, 200);    // Make a tone to indicate VS1053 is working
  unsigned long end = millis();
  Serial.println(F("sineTest complete"));
  Serial.println(end - start);
  Serial.println();


  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayerFullVolume();

#if defined(__AVR_ATmega32U4__)
  // Timer interrupts are not suggested, better to use DREQ interrupt!
  // but we don't have them on the 32u4 feather...
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int
#elif defined(ESP32)
  // no IRQ! doesn't work yet :/
#else
  // If DREQ is on an interrupt pin we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
#endif

}


void updateSound(unsigned long now) {
  if (musicPlayer.playingMusic) {
    if (currentSoundFile) {
      currentSoundFile->lastPlaying = now;

    }
    lastSound = now;
    musicPlayer.feedBuffer();
  } else if (currentSoundFile != NULL) {
    if (currentSoundFile->duration == 0) {
      currentSoundFile->duration = currentSoundFile->lastPlaying - currentSoundFile->lastStarted;
      Serial.print(currentSoundFile->duration);
      Serial.print("ms for  ");
      Serial.println(currentSoundFile->name);
    }
    currentSoundFile = NULL;
  }
}

// Setting the volume is very simple! Just write the 6-bit
// volume to the i2c bus. That's it!
boolean setVolume(int8_t v) {
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


void completeMusic() {
  while (musicPlayer.playingMusic) {
    // twiddle thumbs
    Watchdog.reset();
    musicPlayer.feedBuffer();
    yield(5);           // give IRQs a chance
  }
}

void slowlyStopMusic() {
  if (musicPlayer.playingMusic) {
    for (int i = 0; i < 256; i += 8) {
      musicPlayer.setVolume(i, i);
      yield(10);
    }
    Serial.println("Music slowly stopped");
    musicPlayer.stopPlaying();
    musicPlayerFullVolume();
  }
}

void stopMusic() {
  if (musicPlayer.playingMusic) {
    musicPlayer.stopPlaying();
    Serial.println("Music stopped");
    yield(50);
  }

}

boolean playFile(const char *fmt, ... ) {
  if (currentSoundFile) {
    currentSoundFile->lastPlaying = millis();
  }
  slowlyStopMusic();
  musicPlayerFullVolume();
  currentSoundFile = NULL;
  char buf[256]; // resulting string limited to 256 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, 256, fmt, args);
  va_end (args);
  Serial.print("Playing ");
  Serial.println(buf);

  lastSoundStarted = millis();
  return musicPlayer.startPlayingFile(buf);

}

