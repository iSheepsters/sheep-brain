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
   for (int i = 0; i < 10; i++)
    leds[(0 * NUM_LEDS_PER_STRIP) + i] = CRGB::Blue;
 
  for (int i = 0; i < 10; i++)
    leds[(1 * NUM_LEDS_PER_STRIP) + i] = CRGB::Red;
  for (int i = 0; i < 20; i++)
    leds[(2 * NUM_LEDS_PER_STRIP) + i] = CRGB::Green;
  
  LEDS.show();
  for (int i = 0; i < 10; i++) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(20);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(100);
  }
}

unsigned long nextUpdate = 0;
void loop() {
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(500);
  LEDS.show();

}

