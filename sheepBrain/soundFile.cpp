#include <Arduino.h>
#include "soundFile.h"
#include "sound.h"
#include "printf.h"
#include "util.h"
#include <SPI.h>
#include <SD.h>

#define CARDCS          5     // Card chip select pin

int currentSoundPriority = 0;

const int PATH_BUFFER_LENGTH = 100;
char pathBuffer[PATH_BUFFER_LENGTH];

boolean isMusicFile(File f) {
  if (f.isDirectory()) return false;
  const char *name = f.name();
  const char *suffix = strrchr(name, '.');
  if (name[0] == '_')
    return false;
  if ( strcmp(suffix, ".MP3") == 0)
    return true;
  return false;
}

SoundCollection::SoundCollection(uint8_t pri) : priority (pri) {
  strncpy(name, "", 13);
  count = 0;
  files = NULL;
}

boolean SoundCollection::loadCommon(const char * s) {
  strncpy(name, s, 13);
  File dir = SD.open(name);
  common = true;
  return load(dir);
}

boolean SoundCollection::load(const char * s) {
  strncpy(name, s, 13);
  char buf[30];
  snprintf(buf, 30, "%d/%s", sheepNumber, name);
  myprintf(Serial, "opening %s\n", buf);
  File dir = SD.open(buf);
  return load(dir);
}
boolean SoundCollection::load(File dir) {
  count = 0;
  if (!dir.isDirectory()) {
    files = NULL;
    Serial.print(name);
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
      files[i].collection = this;
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
  } else {
    myprintf(Serial, "Found %d files for %s\n", count, name);
  }
  return true;
}

void SoundCollection::list() {

  for (int i = 0; i < count; i++) {
    Serial.println(files[i].name);
  }
}

uint16_t ago( unsigned long t,  unsigned long now) {
  return (now - t) / 1000;
}

void SoundCollection::verboseList(unsigned long now, boolean ambientSound) {


  Serial.println( "  duration   started   playing  eligable name");
  for (int i = 0; i < count; i++) {
    myprintf(Serial, "%10d%10d%10d     %s %s\n", files[i].duration, ago(files[i].lastStarted, now),
             ago(files[i].lastPlaying,  now),
             files[i].eligibleToPlay(now, ambientSound) ? " true" : "false",
             files[i].name);

  }
}

boolean soundPlayedRecently(unsigned long now) {
  return lastSoundPlaying + 10000L > now && now > 18000;
}

boolean SoundCollection::playSound(unsigned long now, boolean ambientSound) {
  if (ambientSound && soundPlayedRecently(now)) {
    Serial.print("Too soon for any sound from ");
    Serial.println(name);
    return false;
  }
  SoundFile *s  = chooseSound(now, ambientSound);
  if (s == NULL) {
    if (ambientSound)
      Serial.print("No ambient sound found for ");
    else
      Serial.print("No sound found for ");
    Serial.println(name);


    myprintf(Serial, "Last sound %d\n", ago(lastSoundPlaying, now));
    verboseList(now, ambientSound);
    return false;
  };
  slowlyStopMusic();

  boolean result = false;
  if (common)
    result = playFile("%s/%s", name, s->name );
  else
    result = playFile("%d/%s/%s", sheepNumber, name, s->name);
  if (!result) {
    myprintf(Serial, "Could not start %s/%s\n", name, s->name);
    return false;
  } else {
    myprintf(Serial, "Starting %s/%s at %d\n", name, s->name, now);
    s->lastStarted = s->lastPlaying = now;
    currentSoundFile = s;
    currentSoundPriority = priority;
    return true;
  }

}


// If ambientSound is false, always return something
SoundFile * SoundCollection::leastRecentlyPlayed(unsigned long now,
    boolean ambientSound) {

  myprintf(Serial, "finding least recently played %s\n", name);

  SoundFile * candidate = NULL;
  unsigned long bestTime = 0xfffffff;
  int offset = random(count);
  for (int i = 0; i < count; i++) {
    int index = (i + offset) % count;
    SoundFile *s = &(files[index]);
    if (s->lastPlaying == 0)
      return s;
    if (s->eligibleToPlay(now, ambientSound) && bestTime > s->lastPlaying) {
      candidate = s;
      bestTime =  s->lastPlaying;
    }
  }
  if (candidate != NULL || ambientSound)
    return candidate;
  bestTime = 0xfffffff;
  for (int i = 0; i < count; i++) {
    int index = (i + offset) % count;
    SoundFile *s = &(files[index]);
    if (bestTime > s->lastPlaying) {
      candidate = s;
      bestTime =  s->lastPlaying;
    }
  }
  return candidate;
}


SoundFile * SoundCollection::chooseSound(unsigned long now,  boolean ambientSound) {
  if (count == 0) return NULL;
  if (this == & violatedSounds)
    ambientSound = false;
  if (random(3) <= 1) {
    // return first eligable sound
    int firstChoice = random(count);
    if (count > 1)
      while (firstChoice == lastChoice)
        firstChoice = random(count);

    int i = firstChoice;
    while (true) {
      SoundFile *s = &(files[i]);
      lastChoice = i;
      if (s->eligibleToPlay(now, ambientSound))
        return s;

      i = (i + 1) % count;
      if (i == firstChoice) {
        break;
      }
    }
  }
  return leastRecentlyPlayed(now, ambientSound);
}

boolean SoundFile::eligibleToPlay(unsigned long now, boolean ambientSound) {
  if (now == 0) {
    myprintf(Serial, "%s eligable, not played before\n", name);
    return true;
  }
  uint32_t minimumQuietTime = 10000;
  if (ambientSound && lastSoundPlaying + minimumQuietTime > now)
    return false;

  unsigned long d = duration;
  if (d == 0) d = 5000;

  uint32_t minimumRepeatTime = d * 2 + 120000L;

  boolean result = lastPlaying == 0 || lastPlaying + minimumRepeatTime < now;
  if (result) {
    if (lastPlaying == 0)
      myprintf(Serial, "%s eligable: not played before\n",  name);
    else
      myprintf(Serial, "%s eligable: Last playing %d seconds ago; %d minimum\n",
               name, (now - lastPlaying) / 1000, minimumRepeatTime / 1000);
    if (ambientSound) myprintf(Serial, "  last sound %d seconds ago, minimum quiet %d seconds\n",
                                 (now - lastSoundPlaying) / 1000, minimumQuietTime / 1000);

  } else if (false) {
    myprintf(Serial, "%s not eligable: Last playing %d seconds ago; %d minimum\n",
             name, (now - lastPlaying) / 1000, minimumRepeatTime / 1000);
    if (ambientSound) myprintf(Serial, "  last sound %d seconds ago, minimum quiet %d seconds\n",
                                 (now - lastSoundPlaying) / 1000, minimumQuietTime / 1000);

  }
  return result;
}


SoundFile * currentSoundFile = NULL;

boolean setupSD() {
  if (!SD.begin(5 /* CARDCS */)) {
    Serial.println(F("SD failed, or not present"));
    return false;
  }
  Serial.println("SD OK!");


  // list files
  // printDirectory(SD.open("/"), 0);
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




