#define USE_OCTOWS2811
#include<OctoWS2811.h>
#include<FastLED.h>

#include "all.h"
#include "comm.h"
const int led = 13;

#define NUM_LEDS_PER_STRIP 500
#define NUM_STRIPS 8

CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  LEDS.addLeds<OCTOWS2811, RGB>(leds, NUM_LEDS_PER_STRIP);
  LEDS.show();
  Serial.begin(115200);
  // setup LEDS and turn them off
  for (int i = 0; i < 10; i++) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(50);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(50);
  }


  setupComm();
  LEDS.setBrightness(255);
  for (int i = 0; i < 10; i++) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(20);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(100);
  }
}

const  uint8_t HEAD_STRIP = 0;
const  uint8_t LEFT_STRIP = 1;
const  uint8_t RIGHT_STRIP = 2;

const  uint8_t HEAD_BOTTOM = 17;
const  uint8_t HEAD_EYES = 4;
const  uint8_t HEAD_TOP = 4;
const  uint8_t HEAD_HALF = 25;

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

void loop() {

  unsigned long now = millis();
  if (blinking) {
    if (nextBlinkEnds < now)
      blinking = false;
  }  else if (nextBlinkStarts < now) {
    startBlink(now);
  }

  if (nextUpdate < now) {
    Serial.println("sheepSlaveBrain");
    print_i2c_status();
    nextUpdate = now + 20000;
  }
  static uint8_t hue = 0;
  for (int i = 0; i < NUM_STRIPS; i++) {
    for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
      int pos = (i * NUM_LEDS_PER_STRIP) + j;
      uint8_t h = (32 * i) + hue + j;
      if (i == HEAD_STRIP && isEye(j)) {
        if (blinking)
          leds[pos] = 0;
        else
          leds[pos] = CRGB(255, 255, 255);
      } else {
        if (h >= 0)
          leds[(i * NUM_LEDS_PER_STRIP) + j] = CHSV(h, 255, 255);
        else  leds[(i * NUM_LEDS_PER_STRIP) + j]  = 0;
      }
    }
  }


  // Set the first n leds on each strip to show which strip it is
  if (false)
    for (int i = 0; i < NUM_STRIPS; i++) {
      for (int j = 0; j <= i; j++) {
        leds[(i * NUM_LEDS_PER_STRIP) + j] = CRGB::Red;
      }
    }

  hue++;

  LEDS.show();
  LEDS.delay(10);
}

