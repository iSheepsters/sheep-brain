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
  if (false) while (!Serial)
      delay(20);

  if (true) {
    int countdownMS = Watchdog.enable(14000);
    Serial.print("Enabled the watchdog with max countdown of ");
    Serial.print(countdownMS, DEC);
    Serial.println(" milliseconds!");
    Serial.println();
  }
  for (int i = 0; i < 5; i++) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(50);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(50);
    Serial.println(i);
  }

  setupAnimations();
  Serial.println("animations set up");
  setupComm();

  Serial.println("comm set up");
  for (int i = 0; !receivedMsg && i < 100; i++) {
    delay(50);
    Serial.println(i);
  }


  LEDS.setBrightness(BRIGHTNESS_BORED);


}

unsigned long nextUpdate = 0;

int counter = 0;

boolean isDaytime() {
  if (!receivedMsg)
    return false;
  int h = hour();
  return h >= 7 && h <= 18;
}

boolean isPreview() {
  if (!receivedMsg)
    return true;
  int d = day();
  return d <= 27;
}

void loop() {
  Watchdog.reset();
  unsigned long now = millis();

  State currState = commData.state;
  digitalWrite(led, (now / 1000) % 2 == 1);

  if (isDaytime())
    LEDS.setBrightness(255);
  else
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

    myprintf( "sheepSlaveBrain  sheep %d, state %d, %d/%d, %d:%02d:%02d\n",
              commData.sheepNum,
              currState, month(), day(), hour(), minute(), second());
    if (!receivedMsg)
      Serial.println("Have not received any messages");
    if (isPreview())
      Serial.println("is preview");
    if (isDaytime())
      Serial.println("is daytime");
    myprintf(" currentEpoc %d, %dms to next epoc, %d feet to the man", animationEPOC,
             millisToChange,  (int) commData.feetFromMan);
    if (commData.haveFix)
      Serial.println(", gps fix current");
    else
      Serial.println();
    Serial.print("  animation: ");
    currentAnimation->printName();
    myprintf(" touched %02x, quality head = %d, quality back = %d\n",
             commData.currTouched, commData.headTouchQuality, commData.backTouchQuality);
    nextUpdate = now + 5000;
  }

  LEDS.clear();

  if (false) Serial.println("updating animation");
  if (isPreview() ||  !isDaytime()) {
    currentAnimation->update(now);
    overlays();

    copyLEDs();
    LEDS.show();
  } else {
    Serial.println("daytime; LEDs off");
    for (int x = 0; x < GRID_WIDTH; x++)
      for (int y = 0; y < GRID_HEIGHT; y++)
        getSheepLEDFor(x, y) = CRGB::Black;
    LEDS.show();
    delay(10000);
  }

  if (false) Serial.println("updated animation");

  unsigned long timeUsed = millis() - now;

  long timeToWait = 33 - timeUsed;
  if (timeToWait > 0)
    delay(timeToWait);
}


