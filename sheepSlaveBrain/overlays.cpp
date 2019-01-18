
#include "all.h"
#include "printf.h"
#include "comm.h"

const int midSaddle = 8;
const  uint8_t HEAD_BOTTOM = 17;
const  uint8_t HEAD_EYES = 4;
const  uint8_t HEAD_TOP = 4;
const  uint8_t HEAD_HALF = 25;

uint16_t tracerFrame = 0;

boolean pettingOnly() {
  return (commData.state == NotInTheMood || commData.state == Violated);
}

uint8_t flash[GRID_WIDTH][GRID_HEIGHT];

const int numTracers = 8;
struct Tracer {
  Tracer() {
    placeRandomly();
  };
  float x, y;
  uint8_t dir;

  void placeRandomly() {
    x = random(GRID_WIDTH) + 0.5;
    y = random(GRID_HEIGHT) + 0.5;
    dir = random(4);
  }

  boolean inBounds() {
    if (x < 0 || y < 0) return false;
    if (x >= GRID_WIDTH) return false;
    if (y >= GRID_HEIGHT) return false;
    return true;
  }
  void placeRandomRadial();
  void moveTowardsCenter() {

    float dx = HALF_GRID_WIDTH - x;
    float dy =  midSaddle - y;
    float range = sqrt(dx * dx + dy * dy);
    if (range < 1.0) {
      placeRandomRadial();
      return;
    }
    x += dx / range;
    y += dy / range;
  }

  void move() {
    switch (dir) {
      case 0:
        x++;
        if (x >= GRID_WIDTH)
          x -= GRID_WIDTH;
        break;
      case 2:
        x--;
        if (x < 0)
          x += GRID_WIDTH;
        break;
      case 1:
        y++;
        if (y >= GRID_HEIGHT)
          y -= GRID_HEIGHT;
        break;
      case 3:
        y--;
        if (y < 0)
          y += GRID_HEIGHT;
    }
  }
};

void Tracer::placeRandomRadial() {
  float distance = GRID_HEIGHT - midSaddle;
  float angle = random(360) * PI / 180;
  x = HALF_GRID_WIDTH + 1 + cos(angle) * distance;
  y = midSaddle + 0.5 + sin(angle) * distance;
  while (!inBounds()) {
    moveTowardsCenter();
  }
}
Tracer tracers[numTracers];

boolean complainedAboutUnknownState = false;


void moveTracer(Tracer & t, int index) {
  switch (commData.state) {
    case Bored:
      if ((index + tracerFrame) % 4 == 0) {
        if (random(6) == 0)
          t.dir = (t.dir + 1 + random(2) * 2) % 4;
        t.move();
      }
      break;
    case Attentive:
      if ((index + tracerFrame) % 2 == 0) {
        if (random(5) == 0) {
          t.placeRandomly();
          t.dir = random(4);
        } else {
          t.move();
        }
      }
      break;
    case Riding:
      t.y++;
      if (t.y > GRID_HEIGHT) {
        t.y = random(4);
        t.x = random(GRID_WIDTH);
      }
      break;
    case ReadyToRide: {
        t.moveTowardsCenter();
      }
      break;
    case NotInTheMood:
      if (t.x < HALF_GRID_WIDTH )
        t.x--;
      else
        t.x++;
      if (t.x < 0 || t.x >= GRID_WIDTH) {
        t.y = random(GRID_HEIGHT);
        t.x = HALF_GRID_WIDTH - random(2);
      }
      break;
    case Violated:
      break;
    default :
      if (!complainedAboutUnknownState) {
        Serial.print("Unknown state: ");
        Serial.println(commData.state);
        complainedAboutUnknownState = true;
      }
  }
}
void updateTracers() {
  tracerFrame++;
  for (int i = 0; i < numTracers; i++) {
    Tracer & t = tracers[i];
    moveTracer(t, i);
    int x = ((int) t.x) % GRID_WIDTH;
    int y =  ((int) t.y) % GRID_HEIGHT;

    if (false &&  flash[x][y] == 255)
      moveTracer(t, i);
    flash[x][y] = 255;
  }
}


void applyAndFadeFlash() {
  int h = (millis() / 40) % 255;
  for (int x = 0; x < GRID_WIDTH; x++)
    for (int y = 0; y < GRID_HEIGHT; y++) {
      int v = flash[x][y];
      if (v == 0)
        continue;
      CRGB & led = getSheepLEDFor(x, y);
      CHSV hsv;
      if (commData.activated == Active) {
        hsv = rgb2hsv_approximate (led);
        int value = v + hsv.v;
        if (value <= 255) {
          hsv.v = value;
        } else {
          hsv.value = 255;
          value = hsv.s - (value - 255);
          if (value < 0)
            hsv.s = 0;
          else hsv.s = value;
        }
        hsv.s = hsv.s / 2;
      } else {
        // inactive, not showing animations, just colored tracers
        hsv = CHSV(h, 255, v);
      }
      led = hsv;

      if (commData.state == ReadyToRide)
        v = v * 0.95 - 2;
      else
        v = v * 0.9 - 3;
      if (v < 0)
        v = 0;
      flash[x][y] = v;
    }
}


boolean isEye(int led) {
  if (HEAD_BOTTOM <= led && led < HEAD_BOTTOM + HEAD_EYES)
    return true;
  if (HEAD_HALF + HEAD_TOP <= led && led < HEAD_HALF + HEAD_TOP + HEAD_EYES)
    return true;
  return false;
}

unsigned long nextBlinkEnds = 0;
unsigned long nextBlinkStarts = 0;
boolean blinking;
void startBlink(unsigned long now) {
  blinking = true;
  uint16_t blinkDuration = random(50, 100);
  nextBlinkEnds = now + blinkDuration;
  nextBlinkStarts = now + blinkDuration * 3 + 500 + random(1, 4000);
}



void privateLights() {
  for (int x = 0; x < GRID_WIDTH; x++)
    getSheepLEDFor(x, GRID_HEIGHT - 1) = CRGB::Red;
}

void rumpLights() {
  for (int x = HALF_GRID_WIDTH - 3; x < HALF_GRID_WIDTH + 3; x++)
    for (int y = GRID_HEIGHT - 4; y < GRID_HEIGHT - 1; y++)
      getSheepLEDFor(x, y) = pettingOnly() ? CRGB::Pink : CRGB::LightGrey;
}

void leftLights() {
  // left side
  for (int y = 4; y < 12; y++)
    getSheepLEDFor( QUARTER_GRID_WIDTH + 1, y)
      = pettingOnly() ? CRGB::Pink : CRGB::LightGrey;
}
void rightLights() {
  // right side
  for (int y = 4; y < 12; y++)
    getSheepLEDFor( QUARTER_GRID_WIDTH + HALF_GRID_WIDTH - 1, y)
      = pettingOnly() ? CRGB::Pink : CRGB::LightGrey;
}
int backLightPosOff = 0;
const int backLightSlowdown = 32;
const int backLightStart = 4;
const int backLightReadyToRide = midSaddle - 2;
void backLights() {
  // back side
  int q = commData.backTouchQuality + commData.headTouchQuality / 4;
  int start = commData.state == ReadyToRide
              ? backLightReadyToRide : backLightStart;
  //myprintf("Back lights %d\n", q);
  for (int y = -start; y <=  start; y++) {
    int brightness = 250 - abs(y) * 130 + q * 25;
    brightness = max(0, min(255, brightness));
    if (y == 0) brightness = 255;
    if (commData.state == ReadyToRide) {
      if (abs(y) == backLightPosOff / backLightSlowdown)
        brightness = 0;
      backLightPosOff = backLightPosOff - 1;
      if (backLightPosOff <= 0)
        backLightPosOff = backLightReadyToRide * backLightSlowdown;
    }
    for (int x = HALF_GRID_WIDTH - 1; x <= HALF_GRID_WIDTH; x++)
      getSheepLEDFor(x, y + midSaddle) = CRGB(brightness, brightness, brightness);
  }
}
void headLights() {
  // head
  boolean flash = (millis() / 500) % 2 == 1;
  if (false) {
    int q =  commData.backTouchQuality / 2;
    if (q > 7) q = 7;
    if (q > 0)
      myprintf("Setting %d extra head lights green\n", q);
    for (int j = HEAD_BOTTOM - q; j < HEAD_BOTTOM; j++)
      leds[ j] = CRGB::Green;
    int startOfLeft = HEAD_HALF + HEAD_TOP + HEAD_EYES;
    for (int j = startOfLeft;
         j < startOfLeft + q; j++)
      leds[ j] = CRGB::Green;
  }
  for (int j = HEAD_BOTTOM + HEAD_EYES; j < HEAD_BOTTOM + HEAD_EYES + 2 * HEAD_TOP; j++)

    leds[ j] = CRGB::Green;
  for (int x = HALF_GRID_WIDTH - 2; x <= HALF_GRID_WIDTH + 1; x++)
    getSheepLEDFor(x, 0) = CRGB::Green;
}

void violatedLights() {

  // HALF_GRID_WIDTH-1, midSaddle
  // HALF_GRID_WIDTH, midSaddle+1
  // HALF_GRID_WIDTH+1, midSaddle+2
  // x-y = HALF_GRID_WIDTH-1-midSaddle
  const int difference = HALF_GRID_WIDTH - 1 - midSaddle;

  // HALF_GRID_WIDTH, midSaddle
  // HALF_GRID_WIDTH-1, midSaddle+1
  // x+y = HALF_GRID_WIDTH+midSaddle

  const int sum = HALF_GRID_WIDTH + midSaddle;
  boolean flash = (millis() / 500) % 2 == 1;
  for (int x = 0; x < GRID_WIDTH; x++)
    for (int y = 0; y < GRID_HEIGHT; y++) {
      int offset1 = abs(x - y - difference);
      int offset2 = abs(x + y - sum);
      if (offset1 == 0 || offset2 == 0)
        getSheepLEDFor(x, y) = flash ? CRGB::Red : CRGB::Black;
      else if (offset1 == 1 || offset2 == 1)
        getSheepLEDFor(x, y) = CRGB::Black;
    }
}

void head() {
  unsigned long now = millis();
  if (blinking) {
    if (nextBlinkEnds < now)
      blinking = false;
  }  else if (nextBlinkStarts < now) {
    startBlink(now);
  }
  if (commData.sheepNum == 13) {
    float v1 = sin(now / 1500.0 * PI);
    if (!blinking) {
      for (int j = 0; j < 50; j++) {
        float v2 = sin((now + j * 717) / 500.0 * PI);
        int b = 15 + ((v1 + v2) + 4.0) * 40;
        if (b < 0) b = 0;
        if (b > 255) b = 255;
        leds[j] = CRGB(b, 0, 0);
      }
    }

  } else
    for (int j = 0; j < 50; j++)
      if (isEye(j)) {
        if (!blinking)
          leds[j] = CRGB::White;
      } else if (commData.activated == Active)
        leds[j] = CRGB::White;
}

void overlays(boolean receivedMsg) {

  head();
  if (receivedMsg) {
    updateTracers();
    applyAndFadeFlash();
    if (timeSinceLastMessage() > 10 * 1000) {
      for (int x = HALF_GRID_WIDTH - 2; x <= HALF_GRID_WIDTH + 1; x++)
        getSheepLEDFor(x, 2) = CRGB::Red;
    } else {
      if (commData.state == Violated) {
        violatedLights();
      }
      uint8_t touchData = commData.currTouched;

      if (true) {
        if (touchData & 0x1)
          privateLights();
        if (touchData & 0x2)
          rumpLights();
        if (touchData & 0x4)
          leftLights();
        if (touchData & 0x8)
          rightLights();
        if (touchData & 0x20)
          headLights();
      }
      if (touchData & 0x10 || touchData & 0x80)
        backLights();

      if (commData.state == Attentive) {
        for (int x = HALF_GRID_WIDTH - 1; x <= HALF_GRID_WIDTH + 0; x++)
          getSheepLEDFor(x, 2) = CRGB::White;
      }
    }
  }

}
