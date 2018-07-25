#include <Adafruit_SleepyDog.h>
#include <Wire.h>

#include<FastLED.h>

#include "all.h"
#include "comm.h"
const int led = 13;

const uint16_t NUM_LEDS_PER_STRIP  = 400;
const uint8_t NUM_STRIPS  = 8;

const uint8_t GRID_HEIGHT = 25;
const uint8_t QUARTER_GRID_WIDTH = 8;
const uint8_t HALF_GRID_WIDTH = 16;
const uint8_t GRID_WIDTH = 2 * HALF_GRID_WIDTH;

enum State {
  Bored,
  Welcoming,
  Riding
};

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
class Animation {
    // row = 0..24
    // col = 0..15

    virtual void initialize() = 0;
    virtual void update(unsigned long now) = 0;
};


void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  LEDS.addLeds<WS2811_PORTD, NUM_STRIPS, RGB>(leds, NUM_LEDS_PER_STRIP)
  .setCorrection(TypicalLEDStrip);
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
  Serial.println("comm set up");
  LEDS.setBrightness(255);
  for (int i = 0; i < 10; i++) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(20);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(100);
  }
  if (true) {
    int countdownMS = Watchdog.enable(4000);
    Serial.print("Enabled the watchdog with max countdown of ");
    Serial.print(countdownMS, DEC);
    Serial.println(" milliseconds!");
    Serial.println();
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

int counter = 0;


void privateLights() {
  for (int x = 0; x < GRID_WIDTH; x++)
    getSheepLEDFor(x, GRID_HEIGHT - 1) = CRGB::Red;
}

void rumpLights() {
  for (int x = HALF_GRID_WIDTH - 3; x < HALF_GRID_WIDTH + 3; x++)
    for (int y = GRID_HEIGHT - 4; y < GRID_HEIGHT - 1; y++)
      getSheepLEDFor(x, y) = CRGB::White;
}

void leftLights() {

  // left side
  for (int y = 5; y < 12; y++)
    getSheepLEDFor( QUARTER_GRID_WIDTH, y) = CRGB::White;
}
void rightLights() {
  // right side
  for (int y = 5; y < 12; y++)
    getSheepLEDFor( QUARTER_GRID_WIDTH + HALF_GRID_WIDTH, y) = CRGB::White;
}
void backLights() {
  // back side
  for (int x = HALF_GRID_WIDTH - 1; x <= HALF_GRID_WIDTH; x++)
    for (int y = 5; y < 12; y++)
      getSheepLEDFor( x, y) = CRGB::White;
}
void headLights() {
  // head
  for (int j = HEAD_BOTTOM + HEAD_EYES; j < HEAD_BOTTOM + HEAD_EYES + 2 * HEAD_TOP; j++)
    leds[ j] = CRGB::Green;
}


void loop() {
  if (true)
    Watchdog.reset();
  State currState = (State) mem[1];
  if (false) {
    //LEDS.clear();
    for (int x = 0; x < GRID_WIDTH; x++)
      getSheepLEDFor(x, counter) = CHSV(x * 10, 255, 255);
    privateLights();

    rumpLights();

    leftLights();

    rightLights();

    backLights();

    headLights();
    LEDS.show();

    for (int x = 0; x < GRID_WIDTH; x++)
      getSheepLEDFor(x, counter) = CRGB::Black;
    counter++;
    if (counter >= GRID_HEIGHT) counter = 0;


    delay(100);
  }

  if (false) {
    for (int i = counter; i < counter + 20 && i < 400; i++) {
      leds[(1 * NUM_LEDS_PER_STRIP) + i] = CRGB::Blue;
      leds[(2 * NUM_LEDS_PER_STRIP) + i] = CRGB::Green;
    }

    LEDS.show();

    for (int i = counter; i < counter + 20 && i < 400; i++) {
      leds[(1 * NUM_LEDS_PER_STRIP) + i] = CRGB::Black;
      leds[(2 * NUM_LEDS_PER_STRIP) + i] = CRGB::Black;
    }
    counter++;
    if (counter >= 400)
      counter = 0;
    delay(10);
  }



  unsigned long now = millis();
  if (blinking) {
    if (nextBlinkEnds < now)
      blinking = false;
  }  else if (nextBlinkStarts < now) {
    startBlink(now);
  }

  if (nextUpdate < now) {
    myprintf(Serial, "sheepSlaveBrain %d\n", currState);
    nextUpdate = now + 20000;
  }

  LEDS.clear();
  for (int x = 0; x < GRID_WIDTH; x++)
    for (int y = 0; y < GRID_HEIGHT; y++)
      getSheepLEDFor(x, y) = CRGB::Black;


  static uint8_t hue = 0;


  if (true) {
    for (int x = 0; x < HALF_GRID_WIDTH; x++)
      for (int y = 0; y < GRID_HEIGHT; y++) {
        uint8_t v = 255;
        switch (currState) {
          case Welcoming:
            v = (x * 137 + 179 * y + now / 10);
            if (v < 128) {
              v = 128;
            } else 
              v = (v+190)/2;
            break;
          case Bored:
            v = 128;
            break;
          case Riding:
            float r = (-y + now / 100.0) * 2.0 * PI * 5 / GRID_HEIGHT;
            v = sin(r) * 64 + 128 + 64;
            break;

        }
        getSheepLEDFor(HALF_GRID_WIDTH + x, y) = CHSV(x * 8 + hue, 255, v);
        getSheepLEDFor(HALF_GRID_WIDTH - 1 - x, y) = CHSV(x * 8 + hue, 255, v);
      }
  }

  const uint8_t TRACER_LENGTH = 10;
  static int8_t tracerX = 1 - TRACER_LENGTH;
  static int8_t tracerY = 0;


  for (int j = 0; j < 50; j++)
    if (isEye(j)) {
      if (blinking)
        leds[j] =  CRGB::Black;
      else
        leds[j] = CRGB::White;
    } else
      leds[j] = CRGB::White;

  uint8_t touchData = mem[0];

  if (touchData & 0x1)
    privateLights();
  if (touchData & 0x2)
    rumpLights();
  if (touchData & 0x4)
    leftLights();
  if (touchData & 0x8)
    rightLights();
  if (touchData & 0x10)
    backLights();
  if (touchData & 0x20)
    headLights();


  // Set the first n leds on each strip to show which strip it is
  if (true)
    for (int i = 0; i < NUM_STRIPS; i++) {
      for (int j = 0; j <= i; j++) {
        leds[(i * NUM_LEDS_PER_STRIP) + j] = CRGB::Red;
      }
    }

  if (true) {
    for (int x = tracerX; x <= tracerX + TRACER_LENGTH && x < GRID_WIDTH; x++)
      getSheepLEDFor(x, tracerY) = CRGB::White;
    tracerX ++;
    if (tracerX >= GRID_WIDTH) {
      tracerX = 1 - TRACER_LENGTH;
      tracerY++;

      if (tracerY >= GRID_HEIGHT)
        tracerY = 0;
    }
  }
  hue++;
  copyLEDs();
  LEDS.show();

  delay(10);
}

