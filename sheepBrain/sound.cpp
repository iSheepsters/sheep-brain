#include "Arduino.h"
#include "all.h"



#include "sound.h"

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

// Feather ESP8266
#elif defined(ESP8266)
#define VS1053_CS      16     // VS1053 chip select pin (output)
#define VS1053_DCS     15     // VS1053 Data/command select pin (output)
#define CARDCS          2     // Card chip select pin
#define VS1053_DREQ     0     // VS1053 Data request, ideally an Interrupt pin

// Feather ESP32
#elif defined(ESP32)
#define VS1053_CS      32     // VS1053 chip select pin (output)
#define VS1053_DCS     33     // VS1053 Data/command select pin (output)
#define CARDCS         14     // Card chip select pin
#define VS1053_DREQ    15     // VS1053 Data request, ideally an Interrupt pin

// Feather Teensy3
#elif defined(TEENSYDUINO)
#define VS1053_CS       3     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          8     // Card chip select pin
#define VS1053_DREQ     4     // VS1053 Data request, ideally an Interrupt pin

// WICED feather
#elif defined(ARDUINO_STM32_FEATHER)
#define VS1053_CS       PC7     // VS1053 chip select pin (output)
#define VS1053_DCS      PB4     // VS1053 Data/command select pin (output)
#define CARDCS          PC5     // Card chip select pin
#define VS1053_DREQ     PA15    // VS1053 Data request, ideally an Interrupt pin

#elif defined(ARDUINO_FEATHER52)
#define VS1053_CS       30     // VS1053 chip select pin (output)
#define VS1053_DCS      11     // VS1053 Data/command select pin (output)
#define CARDCS          27     // Card chip select pin
#define VS1053_DREQ     31     // VS1053 Data request, ideally an Interrupt pin
#endif


Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);


// 0x4B is the default i2c address
#define MAX9744_I2CADDR 0x4B

// We'll track the volume level in this variable.
int8_t thevol = 48;

unsigned long lastSoundStarted = 0;


void playBored() {
  playFile("bored/bored-%d.mp3", random(1,10));
}
void playRiding() {
  playFile("riding/riding-%d.mp3", random(1,7));
}
void playWelcoming() {
  playFile("w/w-%d.mp3", random(1,7));
}

void setupSound() {
   if (! setVolume(0))
    Serial.println("Failed to set volume, MAX9744 not found!");
  else
    Serial.println("MAX9744 found");

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
  musicPlayer.sineTest(0x44, 1000);    // Make a tone to indicate VS1053 is working
  unsigned long end = millis();
  Serial.println(F("sineTest complete"));
  Serial.println(end - start);
  Serial.println();
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(0, 0);

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
  playFile("baa0.mp3");
}

void baa() {
  playFile("baa%d.mp3", 1 + random(8));
}

// Setting the volume is very simple! Just write the 6-bit
// volume to the i2c bus. That's it!
boolean setVolume(int8_t v) {

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
    musicPlayer.feedBuffer();
    delay(5);           // give IRQs a chance
  }
}



void slowlyStopMusic() {
  if (musicPlayer.playingMusic) {
    for (int i = 0; i < 256; i += 8) {
      musicPlayer.setVolume(i, i);
      delay(10);
    }
    Serial.println("Music slowly stopped");
    musicPlayer.stopPlaying();
    musicPlayer.setVolume(0, 0);
  }
}

void stopMusic() {
  if (musicPlayer.playingMusic) {
    musicPlayer.stopPlaying();
    Serial.println("Music stopped");
    delay(50);
  }

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
  return musicPlayer.startPlayingFile(buf);

}
