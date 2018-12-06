#include <Adafruit_SleepyDog.h>
#include <SPI.h>
#include <RH_RF95.h>
#include "secret.h"

/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_HX8357.h>
#include <Adafruit_STMPE610.h>
#include <Fonts/FreeSansBold18pt7b.h>

#ifdef ARDUINO_SAMD_FEATHER_M0
#define STMPE_CS 6
#define TFT_CS   9
#define TFT_DC   10
#define SD_CS    5
#endif


#define TFT_RST -1

Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 100
#define TS_MAXX 3800
#define TS_MINY 100
#define TS_MAXY 3750

#define BOXSIZE 106


// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

typedef uint16_t hash_t;

struct  __attribute__ ((packed)) CommandInfo {
  int8_t packetKind = 4;
  hash_t hashValue = SECRET_SALT;
  uint8_t ampVol;
  boolean reboot;
};


void centerText(int x, int y, const char * s) {
  int16_t  x1, y1;
  uint16_t w, h;
  tft.getTextBounds(s, x, y, &x1, &y1, &w, &h);

  tft.setCursor(x - w / 2, y + h / 2);
  tft.println(s);
}
void eraseButton(int x, int y) {
  tft.fillRect(x * BOXSIZE, y * BOXSIZE, BOXSIZE, BOXSIZE, HX8357_BLACK);
}
void drawButton(int x, int y, const char * s) {
  tft.drawRect(x * BOXSIZE, y * BOXSIZE, BOXSIZE, BOXSIZE, HX8357_WHITE);
  tft.setTextColor(HX8357_WHITE);
  centerText(x * BOXSIZE + BOXSIZE / 2, y * BOXSIZE + BOXSIZE / 2, s);
}
void drawFilledButton(int x, int y, const char * s) {
  tft.fillRect(x * BOXSIZE, y * BOXSIZE, BOXSIZE, BOXSIZE, HX8357_RED);
  tft.setTextColor(HX8357_BLACK);
  centerText(x * BOXSIZE + BOXSIZE / 2, y * BOXSIZE + BOXSIZE / 2, s);
}
void drawFilledRect(int x, int y, const char * s) {
  tft.fillRect(x * BOXSIZE, y * BOXSIZE, BOXSIZE, BOXSIZE, HX8357_BLACK);
  tft.setTextColor(HX8357_WHITE);
  centerText(x * BOXSIZE + BOXSIZE / 2, y * BOXSIZE + BOXSIZE / 2, s);
}
int vol = 20;
int state = 0;
  char buf[30];
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
  }

  delay(100);
  int countdownMS = Watchdog.enable(8000);

  Serial.println("Sheep remote control");
  Serial.println("Watchdog set");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // change to < Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
  rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512);
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }

  Serial.println("Touchscreen started");
  tft.begin(HX8357D);
  tft.setTextColor(HX8357_WHITE);
  tft.setTextSize(1);

  tft.setFont(&FreeSansBold18pt7b);

  tft.fillScreen(HX8357_BLACK);
  drawButton(0, 0, "-5");
  drawButton(0, 1, "-1");
  drawButton(2, 1, "+1");
  drawButton(2, 0, "+5");
  drawButton(0, 3, "r");
  drawButton(2, 3, "s");
  drawButton(1, 3, "g");
itoa(vol, buf, 10);
    drawFilledRect(1, 2, buf);

}

void changeState(int newState) {
  switch (newState) {
    case 1:
      drawFilledButton(0, 3, "r");
      break;
    case 2:
      drawFilledButton(2, 3, "s");
      break;
    case 3:
      drawFilledButton(1, 3, "g");
      break;
    case 0:
      eraseButton(0, 3);
      eraseButton(2, 3);
      eraseButton(1, 3);
      drawButton(0, 3, "r");
      drawButton(2, 3, "s");
      drawButton(1, 3, "g");
      break;

  }
  state = newState;

}


void sendCommand() {
  CommandInfo commandInfo;
  commandInfo.ampVol = vol;
  commandInfo.reboot = false;

  Serial.print("Sending command to sheep, vol: ");
  Serial.println(vol);

  unsigned long t1 = millis();
  rf95.waitCAD();
  unsigned long t2 = millis();
  rf95.send((uint8_t *)&commandInfo, sizeof(commandInfo));

  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  unsigned long t3 = millis();
  if (t2 - t1 > 1) {
    Serial.print("CAD  delay: ");
    Serial.print(t2 - t1);
    Serial.println("ms");
  }
  Serial.print("Send delay: ");
  Serial.print(t3 - t2);
  Serial.println("ms");
}
int prevVol = -1;

void loop() {
      Watchdog.reset();


  if (ts.bufferEmpty()) {
    delay(10);
    return;
  }

  TS_Point p;
  while (!ts.bufferEmpty())
    p = ts.getPoint();
  Serial.print(p.x);
  Serial.print(" ");
  Serial.println(p.y);

  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  Serial.print(p.x);
  Serial.print(" ");
  Serial.println(p.y);
  int bx = p.x / BOXSIZE;
  int by = p.y / BOXSIZE;
  Serial.print(bx);
  Serial.print(" ");
  Serial.println(by);
  if (bx == 0 && by == 0)
    vol -= 5;
  else if (bx == 0 && by == 1)
    vol -= 1;
  else if (bx == 2 && by == 0)
    vol += 5;
  else if (bx == 2 && by == 1)
    vol += 1;
  else if (bx == 0 && by == 3 && state == 0)
    changeState(1);
  else if (bx == 2 && by == 3)
    if (state == 1)
      changeState(2);
    else changeState(0);
  else if (bx == 1 && by == 3) {
    if (state == 2) {
      changeState(3);
      sendCommand();
    }
    changeState(0);
  }
  if (vol < 0) vol = 0;
  else if (vol > 63) vol = 63;
  if (vol != prevVol) {
    itoa(vol, buf, 10);
    drawFilledRect(1, 2, buf);
    prevVol = vol;
  }

  unsigned long debounce = millis() + 100;
  while (debounce > millis()) {
    Watchdog.reset();
    while (!ts.bufferEmpty()) {
      ts.getPoint();
    }
    delay(10);
    if (ts.touched()) debounce = millis() + 100;
  }

}
