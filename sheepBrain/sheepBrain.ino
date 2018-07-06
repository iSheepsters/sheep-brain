
#include <Wire.h>
#include "all.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const boolean useSound = true;
const boolean useOLED = false;
Adafruit_SSD1306 display = Adafruit_SSD1306();
unsigned long nextPettingReport;

enum State {
  Bored,
  Attentive,
  Accepting,
  Playtime,
  NotYet,
  Unhappy
};

enum State currState = Bored;
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  Serial.begin(115200);
  for (int i = 1; i < 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
    Serial.println(i);
  }
  Serial.println("Setting up touch");
  setupTouch();
  if (useSound)
    setupSound();


  if (useOLED) {

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)

    display.display();
    Serial.println("OLED begun");
    Serial.print("OLED status: " );
    Serial.println(display.ssd1306_status());
  }

  // if you're using Bluefruit or LoRa/RFM Feather, disable the BLE interface
  //pinMode(8, INPUT_PULLUP);

  Serial.println("Ready");
  nextPettingReport = millis() + 2000;
}

unsigned long nextBaa = 2000;


void updateState() {
  switch (currState) {
    case Bored:

    case Attentive:
    case Accepting:
    case Playtime:
    case NotYet:
    case Unhappy:
      Serial.println("state");

  }
}
void loop() {
  unsigned long now = millis();
  updateTouchData(now);

  for (int i = 0; i < numTouchSensors; i++) {
    if (((newTouched >> i) & 1 ) != 0) {
     // printf(Serial,"touched %d\n", i);
      //playFile("%d.mp3", i);
    }
  }
  
  if (false && nextPettingReport < now) {
    nextPettingReport = now + 250;
    for (int i = 0; i < numPettingSensors; i++) {

      float confidence = 0.0;
      /// int i = 7;
      float hz = detectPetting(i, 64, &confidence);
      digitalWrite(LED_BUILTIN, hz > 0.0);
      Serial.print(hz);
      Serial.print(" ");
      //       Serial.print(confidence);
      //      Serial.print(" ");
    }
    Serial.println();
  }
  if (useSound)
    checkForSound();
}

void checkForSound() {
  unsigned long now = millis();
  if (nextBaa < now && !musicPlayer.playingMusic) {
    baa();
    nextBaa = now + 4000 + random(1, 10000);
  }
  if (Serial && Serial.available()) {
    char c = Serial.read();
    if (c != '\n') {
      Serial.println();
      Serial.print("Got " );
      Serial.println(c);
      switch (c) {

        case 'b':
          playFile("baa%d.mp3", 1 + random(8));
          break;
        case '1':
          Serial.println(musicPlayer.playFullFile("baa1.mp3"));
          break;
        case '2':
          Serial.println(musicPlayer.playFullFile("baa2.mp3"));
          break;
        case '3':
          Serial.println(musicPlayer.playFullFile("baa3.mp3"));
          break;
        case '4':
          Serial.println(musicPlayer.playFullFile("baa4.mp3"));
          break;
        case 'h':
          Serial.println(musicPlayer.startPlayingFile("herd1.ogg"));
          break;
        case 'x':
          Serial.println(musicPlayer.startPlayingFile("scream1.mp3"));
          break;
        case 'd':
          Serial.println(musicPlayer.startPlayingFile("drumbone.mp3"));
          break;
        case 's':
          musicPlayer.stopPlaying();
          break;
        // if we get an 'p' on the serial console, pause/unpause!
        case 'p':
          if (! musicPlayer.paused()) {
            Serial.println("Paused");
            musicPlayer.pausePlaying(true);
          } else {
            Serial.println("Resumed");
            musicPlayer.pausePlaying(false);
          }
          break;

        case '+':
        case '-':
          if (c == '+')
            thevol++;
          else
            thevol--;
          if (thevol > 63) thevol = 63;
          if (thevol < 0) thevol = 0;

          setvolume(thevol);
          break;
      }
    }
  }

}
