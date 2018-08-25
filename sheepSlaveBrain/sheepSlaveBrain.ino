#include <Adafruit_SleepyDog.h>


#include<FastLED.h>

#include "all.h"
#include "comm.h"
#include "hsv2rgb.h"
#include "animations.h"
#include "overlays.h"
#include <TimeLib.h>

const int led = 13;

const uint16_t NUM_LEDS_PER_STRIP  = 400;
const uint8_t NUM_STRIPS  = 8;

const  uint8_t HEAD_STRIP = 0;
const  uint8_t LEFT_STRIP = 1;
const  uint8_t RIGHT_STRIP = 2;

CRGB unusedLED;
CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

CRGB & getSheepLEDFor(uint8_t x, uint8_t y) {
  if (x >= GRID_WIDTH || y >= GRID_HEIGHT)
    return unusedLED;
  // 0 <= x < GRID_WIDTH
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
  if ((x + strip) % 2 == 0)
    pos += y;
  else
    pos += (GRID_HEIGHT - 1 - y);

  return leds[strip * NUM_LEDS_PER_STRIP + pos];
}

void copyLEDs() {
  uint16_t halfWay = 4 * NUM_LEDS_PER_STRIP;
  for (int i = 0; i < halfWay; i++) {
    leds[i + halfWay] =  leds[i];
  }
}

const uint8_t BRIGHTNESS_NORMAL = 210;
const uint8_t BRIGHTNESS_BORED = 100;


void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  LEDS.addLeds<WS2811_PORTD, NUM_STRIPS, RGB>(leds, NUM_LEDS_PER_STRIP)
  .setCorrection(TypicalLEDStrip);
  LEDS.show();
  Serial.begin(115200);

  if (true) {
    int countdownMS = Watchdog.enable(4000);
    Serial.print("Enabled the watchdog with max countdown of ");
    Serial.print(countdownMS, DEC);
    Serial.println(" milliseconds!");
    Serial.println();
  }
  // setup LEDS and turn them off
  for (int i = 0; i < 10; i++) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(50);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(50);
    Serial.println(i);
  }

  setupComm();
  Serial.println("comm set up");
  setupAnimations();
  Serial.println("animations set up");

  LEDS.setBrightness(BRIGHTNESS_BORED);

}

unsigned long nextUpdate = 0;

int counter = 0;

void loop() {
  Watchdog.reset();
  unsigned long now = millis();

  State currState = commData.state;
  digitalWrite(led, (now / 1000) % 2 == 1);
  switch (currState) {
    case   Bored:
    case Violated:
      LEDS.setBrightness(BRIGHTNESS_BORED);
      break;
    default :
      LEDS.setBrightness(BRIGHTNESS_NORMAL);
      break;
  }

  if (nextUpdate < now) {
    unsigned long millisToChange =  updateAnimation(now);

    myprintf( "sheepSlaveBrain state %d, %d:%02d:%02d\n",
              currState, hour(), minute(), second());
    myprintf(" currentEpoc %d, %d feet to the man, %dms to next epoc\n", animationEPOC,
             (int) commData.feetFromMan, millisToChange);
    nextUpdate = now + 5000;
  }

  LEDS.clear();

  currentAnimation->update(now);

  overlays();

  copyLEDs();
  LEDS.show();
  unsigned long timeUsed = millis() - now;

  long timeToWait = 33 - timeUsed;
  if (timeToWait > 0)
    delay(timeToWait);
}


