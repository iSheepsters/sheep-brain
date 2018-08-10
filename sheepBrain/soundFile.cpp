#include <Arduino.h>
#include "soundFile.h"
#include "sound.h"
#include "printf.h"
#include <SPI.h>
#include <SD.h>

#define CARDCS          5     // Card chip select pin


const int PATH_BUFFER_LENGTH = 100;
char pathBuffer[PATH_BUFFER_LENGTH];

boolean isMusicFile(File f) {
  if (f.isDirectory()) return false;
  const char *name = f.name();
  const char *suffix = strrchr(name, '.');
  if ( strcmp(suffix, ".MP3") == 0)
    return true;
  return false;
}

SoundCollection::SoundCollection(uint8_t pri) : priority (pri) {
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
    Serial.print(s);
    Serial.println(" is not a directory");
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
  while (i < count) {
    File entry =  dir.openNextFile();
    if (!entry)
      break;
    if (isMusicFile(entry)) {
      strncpy(files[i].name,  entry.name(), 13);
      i++;
    }
  }
  if (i < count) {
    Serial.println("Ran short of music files!");
    myprintf(Serial, "Expected %d, found %d\n", count, i);
    for (; i < count; i++) {
      strncpy(files[i].name,  "", 13);
      files[i].lastPlaying = 0;
      files[i].lastStarted = 0;
    }
  }
  return true;
}

void SoundCollection::list() {
  Serial.println((uint32_t)(void *)files, HEX);

  for (int i = 0; i < count; i++) {
    Serial.println(files[i].name);
  }
}

void SoundCollection::playSound(uint32_t now, boolean quietTime) {
  SoundFile *s  = chooseSound(now, quietTime);
  if (s == NULL) {
    Serial.print("No sound found for ");
    Serial.println(name);
    return;
  };
  slowlyStopMusic();

  if (!playFile("%s/%s", name, s->name )) {
    Serial.println("Could not start ");
  } else {
    s->lastStarted = s->lastPlaying = now;
    currentSoundFile = s;
  }

}


SoundFile * SoundCollection::chooseSound(uint32_t now,  boolean quietTime) {
  if (count == 0) return NULL;
  for (int i = 0; i < 3; i++) {
    SoundFile *s = &(files[random(count)]);
    if (s->eligibleToPlay(now, quietTime))
      return s;
  }
  return NULL;
}

boolean SoundFile::eligibleToPlay(uint32_t now, boolean quietTime) {
  if (lastStarted == 0 || now == 0) return true;
  unsigned long d = max(3000, duration);
  
  uint32_t minimumQuietTime = duration  + 15000L;
  if (quietTime && lastSound + minimumQuietTime > now)
    return false;

  uint32_t minimumRepeatTime = duration * 3 + 120000L;

  return lastPlaying + minimumQuietTime < now;
}


SoundFile * currentSoundFile = NULL;

boolean setupSD() {
  if (!SD.begin(5 /* CARDCS */)) {
    Serial.println(F("SD failed, or not present"));
    return false;
  }
  Serial.println("SD OK!");

  

  // list files
  //  printDirectory(SD.open("/"), 0);
  return true;
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



