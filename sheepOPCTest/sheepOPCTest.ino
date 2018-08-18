#include <Adafruit_SleepyDog.h>
#include <Wire.h>
#include<FastLED.h>

#include "SdFat.h"

const uint8_t led = 13;
#define NUM_LEDS_PER_STRIP 400
#define NUM_STRIPS 8
uint8_t header[4];
uint8_t buf[3000];
CRGB unusedLED;
CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

SdFatSdioEX sd;
FatFile file;
FatFile dirFile;
uint8_t frameRate;


const uint8_t GRID_HEIGHT = 25;
const uint8_t QUARTER_GRID_WIDTH = 8;
const uint8_t HALF_GRID_WIDTH = 16;
const uint8_t GRID_WIDTH = 2 * HALF_GRID_WIDTH;

const  uint8_t HEAD_STRIP = 0;
const  uint8_t LEFT_STRIP = 1;
const  uint8_t RIGHT_STRIP = 2;

const  uint8_t HEAD_BOTTOM = 17;
const  uint8_t HEAD_EYES = 4;
const  uint8_t HEAD_TOP = 4;
const  uint8_t HEAD_HALF = 25;


CRGB & getSheepLEDFor(uint8_t x, uint8_t y) {
  if (x >= GRID_WIDTH || y >= GRID_HEIGHT)
    return unusedLED;
  // 0 <= x < GRID_WIDTH
  // 0 <= y < GRID_HEIGHT
  uint8_t strip;
  uint16_t pos;
  if (x >= HALF_GRID_WIDTH) {
    strip = RIGHT_STRIP;
    pos = (x - HALF_GRID_WIDTH) * GRID_HEIGHT;

  } else {
    strip = LEFT_STRIP;
    pos = (HALF_GRID_WIDTH - 1 - x) * GRID_HEIGHT;
  }
  if ((x + strip) % 2 == 0)
    pos += y;
  else
    pos += (GRID_HEIGHT - 1 - y);

  return leds[strip * NUM_LEDS_PER_STRIP + pos];
}



boolean isEye(int led) {
  if (HEAD_BOTTOM <= led && led < HEAD_BOTTOM + HEAD_EYES)
    return true;
  if (HEAD_HALF + HEAD_TOP <= led && led < HEAD_HALF + HEAD_TOP + HEAD_EYES)
    return true;
  return false;
}

unsigned long nextUpdate = 0;
unsigned long nextBlinkEnds = 0;
unsigned long nextBlinkStarts = 0;
boolean blinking;
void startBlink(unsigned long now) {
  blinking = true;
  uint16_t blinkDuration = random(50, 100);
  nextBlinkEnds = now + blinkDuration;
  nextBlinkStarts = now + blinkDuration * 3 + 500 + random(1, 4000);
}


unsigned long started;
boolean ok = true;
const int firstFile = 1;
const int lastFile = 21;
int nextFileNumber = firstFile;
const int maxSeconds = 20;

void openNextFile() {
  char buf[256];
  snprintf(buf, 256, "top%02d.opc", nextFileNumber);
  if (!file.open(&dirFile, buf , O_READ)) {
    sd.errorHalt("file not found");
  }
  Serial.print("Update ");
  Serial.print(millis()/1000/60);
  Serial.print(" minutes, opening ");
  Serial.println(buf);
  nextFileNumber ++;
  if (nextFileNumber > lastFile)
    nextFileNumber = firstFile;
  int count = file.read(header, 4);
  if (count != 4) {
    Serial.print(count);
    Serial.println(" bytes read");
    return;
  }
  if (header[0] != 'O' || header[1] != 'P' || header[2] != 'C') {
    Serial.println("Not OPC file");
    return;
  }
  frameRate = header[3];
  Serial.print(frameRate);
  Serial.println(" frames per second");
}

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  LEDS.addLeds<WS2811_PORTD, NUM_STRIPS, RGB>(leds, NUM_LEDS_PER_STRIP)
  .setCorrection(TypicalLEDStrip);
  LEDS.show();
  LEDS.setBrightness(128);
  Serial.begin(115200);
  sd.begin();

  while (!Serial && millis() < 1000) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(100);
  }
  // put your setup code here, to run once:
  if (!dirFile.open("/", O_READ)) {
    sd.errorHalt("open root failed");
  }
  openNextFile();
  started = millis();
}

boolean readFrame() {
  if (!ok) return false;
  int count = file.read(header, 4);
  if (count != 4) {
    Serial.print(count);
    Serial.println(" header bytes read");
    return false;
  }
  uint16_t length = (header[2] << 8) |  header[3] ;
  uint8_t frame = header[0];
  count = file.read(buf, length);
  if (count != length) {
    Serial.print(count);
    Serial.println(" frame bytes read");
    Serial.print(length);
    Serial.println(" expected");
    return false;
  }

  for (int r = 0; r  < 25; r++)
    for (int c = 0; c < 32; c++) {
      int pos = 3 * (r * 32 + c);
      getSheepLEDFor(c, r) = CRGB(buf[pos], buf[pos + 1], buf[pos + 2]);

    }
  return true;
}

uint16_t frameCount = 0;


void loop() {
  unsigned long now = millis();

  if ((now / 1000) % 2 == 1)
    digitalWrite(led, HIGH);
  else
    digitalWrite(led, LOW);


  frameCount++;

  unsigned long startFrame = micros();
  if (frameCount / frameRate > maxSeconds ||  !readFrame()) {
    file.close();
    openNextFile();
    started = now;
    frameCount = 0;
    if (! readFrame())
      ok = false;
  }


  if (!ok) return;
  if (blinking) {
    if (nextBlinkEnds < now)
      blinking = false;
  }  else if (nextBlinkStarts < now) {
    startBlink(now);
  }
  for (int j = 0; j < 50; j++)
    if (isEye(j)) {
      if (blinking)
        leds[j] =  CRGB::Black;
      else
        leds[j] = CRGB::White;
    } else
      leds[j] = CRGB::Grey;

  LEDS.show();
  unsigned long endFrame = micros();

  if (nextUpdate < now) {
    nextUpdate = now + 10000;
    uint16_t seconds = (now - started) / 1000;
    if (seconds < 0) seconds = 1;
    //Serial.println(now);
    Serial.print(frameCount);
    Serial.print(" ");
    Serial.print(seconds);
    Serial.print(" ");
    Serial.print(frameCount / seconds);
    Serial.print(" ");
    Serial.println(endFrame - startFrame);

    if (false) printf("%4d %3d %3d %5d\n",
                        frameCount, seconds, frameCount / seconds,
                        endFrame - startFrame);

  }
  delay(1000 / frameRate - (endFrame - startFrame) / 1000);


}
