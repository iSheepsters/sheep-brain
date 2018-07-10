#include <Arduino.h>
#include "soundFile.h"
#include "printf.h"
#include <SPI.h>
#include <SD.h>

// Feather M0 or 32u4
#if defined(__AVR__) || defined(ARDUINO_SAMD_FEATHER_M0)
#define CARDCS          5     // Card chip select pin

// Feather ESP8266
#elif defined(ESP8266)
#define CARDCS          2     // Card chip select pin

// Feather ESP32
#elif defined(ESP32)
#define CARDCS         14     // Card chip select pin

// Feather Teensy3
#elif defined(TEENSYDUINO)
#define CARDCS          8     // Card chip select pin

// WICED feather
#elif defined(ARDUINO_STM32_FEATHER)
#define CARDCS          PC5     // Card chip select pin

#elif defined(ARDUINO_FEATHER52)
#define CARDCS          27     // Card chip select pin
#endif



boolean isMusicFile(File f) {
  if (f.isDirectory()) return false;
  const char *name = f.name();
  const char *suffix = strrchr(name, '.');
  if ( strcmp(suffix, ".MP3") == 0)
    return true;
  return false;
}

SoundCollection::SoundCollection() {
  strncpy(name, "", 13);
  count = 0;
  files = NULL;
}
boolean SoundCollection::load(const char * s) {
  strncpy(name, s, 13);
  File dir = SD.open(s);
  count = 0;
  if (!dir.isDirectory()) {
    files = NULL;
    return false;
  }
  while (true) {
    File entry =  dir.openNextFile();
    if (!entry) break;
    if (isMusicFile(entry)) {
      count++;
    }
  }
  dir.rewindDirectory();
  files = new SoundFile[count];
  uint16_t i = 0;
  while (true) {
    File entry =  dir.openNextFile();
    if (!entry) break;
    if (isMusicFile(entry)) {
      strncpy(files[i].name,  entry.name(), 13);
      files[i].lastPlaying = 0;
      files[i].lastStarted = 0;
    }
  }
  return true;
}

SoundFile * SoundCollection::chooseSound(uint32_t now) {
  if (count == 0) return NULL;
  for (int i = 0; i < 3; i++) {
    SoundFile *s = &(files[random(count)]);
    if (s->eligibleToPlay(now))
      return s;
  }
  return NULL;
}

boolean SoundFile::eligibleToPlay(uint32_t now) {
  if (lastStarted == 0) return true;
  uint32_t minimumQuietTime = duration * 3 + 120000L;
  return lastPlaying + minimumQuietTime < now;
}

SoundFile * currentSoundFile = NULL;

void setupSD() {
  if (!SD.begin(4 /* CARDCS */)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  Serial.println("SD OK!");

  // list files
  printDirectory(SD.open("/"), 0);
}



/// File listing helper
void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      //Serial.println("**nomorefiles**");
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}


