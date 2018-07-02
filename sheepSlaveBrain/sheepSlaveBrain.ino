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

unsigned long nextUpdate = 0;
void loop() {
  unsigned long now = millis();
  if (nextUpdate < now) {
    Serial.println("sheepSlaveBrain");
    print_i2c_status();
    nextUpdate = now + 20000;
  }
  static uint8_t hue = 0;
  for (int i = 0; i < NUM_STRIPS; i++) {
    for (int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
      uint8_t h = (32 * i) + hue + j;
      if (h > 20)
        leds[(i * NUM_LEDS_PER_STRIP) + j] = CHSV(h, 192, 255);
      else  leds[(i * NUM_LEDS_PER_STRIP) + j]  = 0;
    }
  }

  // Set the first n leds on each strip to show which strip it is
  if (true)

    for (int i = 0; i < NUM_STRIPS; i++) {
      leds[(i * NUM_LEDS_PER_STRIP) + 0] = CRGB::Red;
      leds[(i * NUM_LEDS_PER_STRIP) + 1] = CRGB::Green;
      leds[(i * NUM_LEDS_PER_STRIP) + 2] = CRGB::Blue;
      for (int j = 0; j <= i; j++) {
        leds[(i * NUM_LEDS_PER_STRIP) + j + 3] = CRGB::Red;
      }
    }

  hue++;

  LEDS.show();
  LEDS.delay(10);
}

