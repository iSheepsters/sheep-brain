#include <Adafruit_SleepyDog.h>
#include <Wire.h>
#include<FastLED.h>

#include "SdFat.h"

const uint8_t led = 13;
#define NUM_LEDS_PER_STRIP 400
#define NUM_STRIPS 8
uint8_t header[4];
uint8_t buf[3000];
CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

SdFatSdioEX sd;
FatFile file;
FatFile dirFile;
uint8_t frameRate;


const uint8_t GRID_HEIGHT = 25;
const uint8_t HALF_GRID_WIDTH = 16;


CRGB & getSheepLEDFor(uint8_t x, uint8_t y) {
  // 0 <= x < 2*HALF_GRID_WIDTH
  // 0 <= y < GRID_HEIGHT
  uint8_t strip;
  uint16_t pos;
  if (x >= HALF_GRID_WIDTH) {
    strip = 2;
    pos = (x - HALF_GRID_WIDTH) * GRID_HEIGHT;

  } else {
    strip = 1;
    pos = (HALF_GRID_WIDTH - 1 - x) * GRID_HEIGHT;
  }
  if (x % 2 == 1)
    pos += y;
  else
    pos += (GRID_HEIGHT - 1 - y);

  return leds[strip * NUM_LEDS_PER_STRIP + pos];
}


unsigned long started;
boolean ok = true;

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  LEDS.addLeds<WS2811_PORTD, NUM_STRIPS, RGB>(leds, NUM_LEDS_PER_STRIP)
  .setCorrection(TypicalLEDStrip);
  LEDS.setBrightness(90);
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
  if (!file.open(&dirFile, "a01.opc", O_READ)) {
    sd.errorHalt("a01.opc not found");
  }
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

 for(int r = 0; r  < 25; r++) 
      for(int c = 0; c < 32; c++) {
        int pos = 3*(r*32 + c);
        getSheepLEDFor(c,r) = CRGB(buf[pos], buf[pos+1], buf[pos+2]);
      
      }
  return true;

}

unsigned long nextUpdate = 0;
uint16_t frameCount = 0;

void loop() {
  unsigned long now = millis();

  frameCount++;

  unsigned long startFrame = micros();
  if (!readFrame()) {
    ok = false;

  }

 
  if (!ok) return;
  leds[0] = CRGB::Red;
  LEDS.show();
  unsigned long endFrame = micros();

  if (nextUpdate < now) {
    nextUpdate = now + 5000;
    Serial.print(frameCount);
    Serial.println(" frames");
    uint16_t seconds = (now - started) / 1000;
    Serial.print(seconds);
    Serial.println(" seconds");
    Serial.print(frameCount / seconds);
    Serial.println(" fps");
    Serial.print((endFrame - startFrame));
    Serial.println(" uSecs per frame");
  }
  delay(1000 / frameRate - (endFrame - startFrame) / 1000);


}
