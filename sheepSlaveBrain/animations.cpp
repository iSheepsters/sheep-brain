
#include "all.h"
#include "animations.h"
#include "comm.h"
#include <SdFat.h>
#include <Time.h>

double ANIMATION_FEET_PER_SECOND = 5.0;
const int ANIMATION_EPOC_SECONDS = 30;

const time_t BEFORE_BURN = 1535000000;

int animationEPOC;

double fractionalTime() {
  return (millis() % 1000) / 1000.0;
}

double manTime() {
  double result = (now() - BEFORE_BURN) + fractionalTime();
  return result - commData.feetFromMan / ANIMATION_FEET_PER_SECOND;
}

long millisToNextEpoc() {
  if (!scheduleSetUp) return ANIMATION_EPOC_SECONDS;
  double timeRemaining = (animationEPOC + 1)  * ANIMATION_EPOC_SECONDS
                         - manTime();
  if (false) {
    myprintf("current epoc started at: %8d\n", animationEPOC * ANIMATION_EPOC_SECONDS);
    myprintf("next epoc starts at:     %8d\n", (animationEPOC + 1) * ANIMATION_EPOC_SECONDS);
    Serial.print("distance to man: ");
    Serial.println(commData.feetFromMan);
    Serial.print("now time: ");
    Serial.println(now() - BEFORE_BURN);
    Serial.print("man time: ");
    Serial.println(manTime());
    Serial.print("time remaining: ");
    Serial.println (timeRemaining);
  }
  long result = (long)(1000 * timeRemaining);
  return result;
}



unsigned long updateAnimation(unsigned long now) {
  long remaining = millisToNextEpoc();
  while (remaining < 0) {

    animationEPOC++;
    myprintf("Advancing to animation epoc %d\n", animationEPOC);
    nextAnimation();
    remaining += 1000 * ANIMATION_EPOC_SECONDS;
  }
  return remaining;
};

boolean scheduleSetUp = false;
void setupSchedule() {
  Serial.println("Setting up schedule");
  myprintf("BRC time %d:%02d:%02d\n", hour(), minute(), second());
  Serial.print("distance to man: ");
  Serial.println(commData.feetFromMan);
  myprintf("Man time: ");
  Serial.println(manTime());

  animationEPOC = ((unsigned long)manTime()) / ANIMATION_EPOC_SECONDS;
  myprintf("animation epoc %d\n", animationEPOC);
  scheduleSetUp = true;
  nextAnimation();
};



SdFatSdioEX sd;

FatFile dirFile;
FatFile customAnimations;

const size_t MAX_FILE_NAME_LENGTH = 30;

boolean opcOK = true;
class AnimationFile {
  public:
    AnimationFile() {
      name[0] = '\0';
    };
    char name[MAX_FILE_NAME_LENGTH];
};
int numberOfAnimations = 0;
int numberOfOPCFiles = 0;

boolean isGPS(int i) {
  return false;
  return i % 5 == 0;
}
boolean isCustom(int i) {
  return false;
  return i % 8 == 0;
}
boolean isBasic(int i) {
  return false;
  return i % 11 == 0;
}
boolean isOPC(int i) {
  return !isGPS(i) && !isCustom(i) && !isBasic(i);
}

enum AnimationKind getKind(int i) {
  if (isGPS(i))
    return GPS_KIND;
  if (isCustom(i))
    return CUSTOM_KIND;
  if (isBasic(i))
    return BASIC_KIND;
  return OPC_KIND;
}

int getKindIndex(int animationIndex) {
  enum AnimationKind k = getKind(animationIndex);
  int kindIndex = 0;
  for (int i = 0; i < animationIndex; i++)
    if (getKind(i) == k) kindIndex++;
  return kindIndex;
}

int countAnimations() {
  int num = 0;
  int fileIndex = 0;
  for (; fileIndex < numberOfOPCFiles; num++)
    if (isOPC(num)) fileIndex++;
  return num;
}

AnimationFile * animationFile;

boolean isOPC(FatFile f) {
  char name[MAX_FILE_NAME_LENGTH];
  f.getName(name, MAX_FILE_NAME_LENGTH);

  const char *suffix = strrchr(name, '.');

  if ( strcmp(suffix, ".opc") == 0)
    return true;
  return false;
}

void setupAnimations() {
  Serial.println("setupAnimations");
  sd.begin();
  if (!customAnimations.open(sd.vwd(), "custom", O_READ)) {
    opcOK = false;
    Serial.println("Open custom/ failed, ignoring animations");
    return;
  }
  if (!dirFile.open(sd.vwd(), "opcFiles", O_READ)) {
    opcOK = false;
    Serial.println("Open opcFiles/ failed, ignoring animations");
    return;
  }
  FatFile file;
  while (file.openNext(&dirFile, O_READ)) {
    if (isOPC(file))
      numberOfOPCFiles++;
    file.close();
  }
  dirFile.rewind();
  myprintf("Got %d animations\n", numberOfOPCFiles);
  if (numberOfOPCFiles == 0) return;

  animationFile = new AnimationFile[numberOfOPCFiles];
  int i = 0;
  while (file.openNext(&dirFile, O_READ)) {
    if (isOPC(file)) {

      file.getName(animationFile[i].name, MAX_FILE_NAME_LENGTH);
      i++;
    }
    file.close();
  }
  numberOfAnimations = countAnimations();
  myprintf("%d opc files, %d total animations\n", numberOfOPCFiles,
           numberOfAnimations);

}

class RotateRainbowUp : public Animation {

    uint8_t hue = 0;
  public:
    RotateRainbowUp()  {
      kind = BASIC_KIND;
    };
    void initialize(int idx) {
      index = idx;
    }
    virtual void update(unsigned long now) {
      for (int x = 0; x < HALF_GRID_WIDTH; x++)
        for (int y = 0; y < GRID_HEIGHT; y++) {
          uint8_t v = 220;
          CRGB led = CHSV(x * 8 + hue, 255, v);
          getSheepLEDFor(HALF_GRID_WIDTH + x, y) = led;
          getSheepLEDFor(HALF_GRID_WIDTH - 1 - x, y) = led;
        }
      hue++;
    }
};




class GPSHue : public Animation {
  public:
    GPSHue() {
      kind = GPS_KIND;
    };
    void initialize(int idx) {
      index = idx;
    };

    virtual void printName() {
      myprintf("gps hue animation %d\n", index);
    }
    virtual void update(unsigned long timeMillis) {
      float hue = now() + (timeMillis % 1000) / 1000.0 - commData.feetFromMan / ANIMATION_FEET_PER_SECOND;
      for (int x = 0; x < HALF_GRID_WIDTH; x++)
        for (int y = 0; y < GRID_HEIGHT; y++) {
          uint8_t v = 220;
          CRGB led = CHSV(x + hue, 255, v);
          getSheepLEDFor(HALF_GRID_WIDTH + x, y) = led;
          getSheepLEDFor(HALF_GRID_WIDTH - 1 - x, y) = led;
        }
      hue++;
    }
};




uint8_t header[4];
uint8_t buf[3000];
class ShowOPC : public Animation {

    FatFile file;
    int frameRate;
    int bytesRead = 0;
    boolean ok;
  public:
    ShowOPC()  {
      kind = OPC_KIND;
    };
    virtual void printName() {
      myprintf("Opc animation %d\n", index);
    }
    boolean readFirstHeader() {
      int count = file.read(header, 4);

      if (count != 4) {
        Serial.print(count);
        Serial.println(" bytes read");

        return false;
      }
      if (header[0] != 'O' || header[1] != 'P' || header[2] != 'C') {
        Serial.println("Not OPC file");
        return false;
      }
      frameRate = header[3];
      if (frameRate == 0)
        frameRate = 30;
      bytesRead += 4;
      myprintf("set up opc animation\n");
      return true;
    }
    void initializeCustom(int idx) {
      myprintf("Initialize custom %d\n", idx);
      index = idx;
      ok = false;
      char buffer[20];
      snprintf(buffer, 20, "%d.opc", idx);
      myprintf("Opening custom/%s\n",  buffer);
      if (!file.open(&customAnimations, buffer , O_READ)) {
        myprintf("Could not open %s\n", buffer );
        return;
      }
      if (readFirstHeader())
        ok = true;
    }
    void initialize(int idx) {
      int index = (idx+commData.sheepNum)%numberOfOPCFiles;
      myprintf("Initialize opc. index= %d, sheepNum = %d, opc=%d\n", idx);
      ok = false;
      AnimationFile thisAnimation = animationFile[index];
      myprintf("Opening %s\n",  thisAnimation.name );
      if (!file.open(&dirFile, thisAnimation.name , O_READ)) {
        myprintf("Could not open %s\n", thisAnimation.name );
        return;
      }
      if (readFirstHeader())
        ok = true;
    }
    void close() {
      file.close();
    }
    boolean isOK()  {
      return ok;
    }
    void update(unsigned long now) {

      if (!ok)
        return ;
      int count = file.read(header, 4);
      if (count == 0 && bytesRead > 10000) {
        myprintf("Rewinding after reading %d bytes\n", bytesRead);
        file.rewind();
        bytesRead = 0;
        readFirstHeader();
        count = file.read(header, 4);
      }
      if (count != 4) {
        Serial.print(count);
        Serial.println(" header bytes read");
        ok = false;
        return ;
      }
      bytesRead += count;
      uint16_t length = (header[2] << 8) |  header[3] ;
      count = file.read(buf, length);
      bytesRead += count;
      if (count != length) {
        myprintf("%d frame bytes read, %d expected\n", count, length);

        ok = false;
        return ;
      }

      for (int r = 0; r  < GRID_HEIGHT; r++)
        for (int c = 0; c < GRID_WIDTH; c++) {
          int pos = 3 * (r * GRID_WIDTH + c);
          getSheepLEDFor(c, r) = CRGB(buf[pos], buf[pos + 1], buf[pos + 2]);

        }
      return ;
    }

};

int nextOPC = 0;
RotateRainbowUp animationRotateRainbowUp;
GPSHue animationGPSHue;
ShowOPC animationShowOPC;

Animation * currentAnimation = &animationRotateRainbowUp;


Animation * getAnimation() {

  int index = animationEPOC % numberOfAnimations;
  enum AnimationKind k = getKind(index);
  int kindIndex = getKindIndex(index);
  myprintf("get animation %d %d %d %d %d\n", numberOfAnimations, animationEPOC, index, k, kindIndex);
  Animation * result = NULL;
  switch (k) {
    case GPS_KIND : result = &animationGPSHue;
      result->initialize(kindIndex);
      break;
    case OPC_KIND : result = & animationShowOPC;
      result->initialize(kindIndex);
      break;
    case CUSTOM_KIND : result =  &animationShowOPC;
      animationShowOPC.initializeCustom(commData.sheepNum);
      break;
    case BASIC_KIND : result =  &animationRotateRainbowUp;
      result->initialize(kindIndex);
      break;
    default:
      myprintf("Unknown kind %d\n", k);
      result =  &animationRotateRainbowUp;
      result->initialize(0);
      break;
  }
  return result;
}


void nextAnimation() {
  Serial.println("\nnext animation");

  if (currentAnimation != NULL)
    currentAnimation->close();


  currentAnimation = getAnimation();

}



