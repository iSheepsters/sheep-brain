
#include "all.h"
#include "animations.h"
#include "comm.h"
#include <SdFat.h>
#include <Time.h>

double ANIMATION_FEET_PER_SECOND = 5.0;
const int ANIMATION_EPOC_SECONDS = 120;

const time_t BEFORE_BURN = 1535000000;

int animationEPOC;

double fractionalTime() {
  return (millis() % 1000) / 1000.0;
}

double lastManTime;
unsigned long lastManTimeAt = 0;
double manTime() {
  if (!receivedMsg) {
    return millis() / 1000.0;
  }
  unsigned long ms = millis();
  double result = (now() - BEFORE_BURN) + fractionalTime();
  result = result - commData.feetFromMan / ANIMATION_FEET_PER_SECOND;
  unsigned interval = ms - lastManTimeAt;
  if (lastManTimeAt != 0 && interval < 2000
      && commData.haveFix && ms > 10000) {
    double walkingDistance = interval / 100.0;
    double expected = lastManTime + interval / 1000.0;
    double expectedLow = expected - walkingDistance / ANIMATION_FEET_PER_SECOND - 1;
    double expectedHigh = expected + walkingDistance / ANIMATION_FEET_PER_SECOND + 1;
    if (result < expectedLow) {
      Serial.println("result < expectedLow");
      Serial.println(lastManTime);
      Serial.println(result);
      Serial.println(expectedLow);
      Serial.println(commData.feetFromMan);
      Serial.println(now() - BEFORE_BURN);

      result = (expectedLow + result) / 2;
    } else if (result > expectedHigh) {
      Serial.println("result > expectedHigh");
      Serial.println(lastManTime);
      Serial.println(result);
      Serial.println(expectedHigh);
      Serial.println(commData.feetFromMan);
      Serial.println(now());
      result = (result + expectedHigh) / 2;
    }
  }
  lastManTime = result;
  lastManTimeAt = ms;
  return result;
}

long millisToNextEpoc() {
  if (!scheduleSetUp) return ANIMATION_EPOC_SECONDS;
  double timeRemaining = (animationEPOC + 1)  * ANIMATION_EPOC_SECONDS
                         - manTime();
  if (true) {
    myprintf("current epoc started at: % 8d\n", animationEPOC * ANIMATION_EPOC_SECONDS);
    myprintf("next epoc starts at:     % 8d\n", (animationEPOC + 1) * ANIMATION_EPOC_SECONDS);
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
  if (remaining >= 0)
    return remaining;
  int oldEpoc = animationEPOC;
  while (remaining < 0) {
    animationEPOC++;

    remaining += 1000 * ANIMATION_EPOC_SECONDS;
  }
  myprintf("Advanced % d animations to animation epoc % d\n",
           animationEPOC - oldEpoc,
           animationEPOC);
  nextAnimation();
  return remaining;
};

boolean scheduleSetUp = false;
void setupSchedule() {
  Serial.println("Setting up schedule");
  myprintf("BRC time %d/%d %d:%02d:%02d\n", month(), day(), hour(), minute(), second());
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
  return i % 11 == 0;
}
boolean isCustom(int i) {
  return i % 15 == 0;
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

volatile boolean animationsSetUp = false;
void setupAnimations() {
  Serial.println("setupAnimations");
  sd.begin();
  if (!customAnimations.open(sd.vwd(), "custom", O_READ)) {
    opcOK = false;
    Serial.println("Open custom / failed, ignoring animations");
    return;
  }
  // customAnimations.ls(LS_SIZE);
  if (!dirFile.open(sd.vwd(), "opcFiles", O_READ)) {
    opcOK = false;
    Serial.println("Open opcFiles / failed, ignoring animations");
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
      //myprintf( "found %d %s\n", i, animationFile[i].name);
      i++;
    }
    file.close();
  }
  numberOfAnimations = countAnimations();
  myprintf(" %d opc files, %d total animations\n", numberOfOPCFiles,
           numberOfAnimations);
  nextAnimation();
  animationsSetUp = true;
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
    virtual void printName() {
      myprintf("Rainbow up animation %d\n", index);
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

RotateRainbowUp animationRotateRainbowUp;



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
      float mTime = manTime();
      uint8_t hue = (uint8_t) (mTime * 10);
      for (int y = 0; y < GRID_HEIGHT; y++) {
        uint16_t v = 150 + 150 * sin(
                       (-y + millis() / 200.0 ) * 5.0 * PI / GRID_HEIGHT);
        if (v < 0)
          v = 0;
        else if (v > 255)
          v = 255;
        uint8_t hue_y = hue - y;


        for (int x = 0; x < GRID_WIDTH; x++) {
          CRGB led = CHSV(hue_y, 255, v);

          getSheepLEDFor(x, y) = led;
        }
      }
    };
};




uint8_t header[4];
uint8_t buf[3000];
class ShowOPC : public Animation {

    FatFile file;
    int frameRate;
    int bytesRead = 0;
    boolean ok;
    boolean custom;
  public:
    ShowOPC()  {
      kind = OPC_KIND;
    };
    virtual void printName() {
      if (custom)
        myprintf("custom opc animation %d\n", index);
      else
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
      custom = true;
      index = idx;
      ok = false;
      bytesRead = 0;
      char buffer[20];
      snprintf(buffer, 20, "%d.opc", idx);
      myprintf("Opening custom/%s\n",  buffer);
      if (!file.open(&customAnimations, buffer , O_READ)) {
        myprintf("Could not open %s\n", buffer );
        return;
      }
      if (readFirstHeader()) {
        ok = true;
        myprintf("Read first header for %s\n", buffer);
      } else {
        myprintf("Failed to read first header for %s\n", buffer);
        file.close();
        initialize(idx);
      }

    }
    void initialize(int idx) {
      custom = false;
      index = numberOfOPCFiles == 0 || !receivedMsg ? 0 : (idx + commData.sheepNum) % numberOfOPCFiles;
      myprintf("Initialize opc. index = %d, sheepNum = %d, opc = %d\n", idx);
      ok = false;
      bytesRead = 0;
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

      if (!ok) {
        animationRotateRainbowUp.update(now);
        return;
      }

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
        myprintf(" %d frame bytes read, %d expected\n", count, length);

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
    case GPS_KIND :
      result = &animationGPSHue;
      result->initialize(kindIndex);
      Serial.println("initialized GPS animation");
      return result;

    case OPC_KIND :
      result = & animationShowOPC;
      myprintf("opc index = %d\n", kindIndex);
      result->initialize(kindIndex);
      return result;

    case CUSTOM_KIND :
      result =  &animationShowOPC;
      if (false)
        animationShowOPC.initializeCustom(5);
      else if (commData.sheepNum == 0)
        animationShowOPC.initializeCustom(1);
      else
        animationShowOPC.initializeCustom(commData.sheepNum);
      return result;
    case BASIC_KIND :
      result =  &animationRotateRainbowUp;
      result->initialize(kindIndex);
      return result;
    default:
      myprintf("Unknown kind %d\n", k);
      result =  &animationRotateRainbowUp;
      result->initialize(0);
      return result;

  }
  Serial.println("Should not have gotten here");
  return result;
}


void nextAnimation() {
  Serial.println("\nnext animation");

  if (currentAnimation != NULL)
    currentAnimation->close();

  currentAnimation = getAnimation();


}
