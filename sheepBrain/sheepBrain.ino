
#include <Wire.h>
#include "all.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"


unsigned long nextPettingReport;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Wire.begin();
  for (int i = 1; i < 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay( 20);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay( 20);
    Serial.println(i);
  }
  Serial.println("Setting up touch");
  setupTouch();
  //setupSound();




  //  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  //
  //  display.display();
  //  Serial.println("OLED begun");
  //  Serial.print("OLED status: " );
  //  Serial.println(display.ssd1306_status());

  // if you're using Bluefruit or LoRa/RFM Feather, disable the BLE interface
  //pinMode(8, INPUT_PULLUP);

  Serial.println("Ready");
  nextPettingReport = millis() + 2000;

}

unsigned long nextUpdate = 0;
unsigned long nextUpdate2 = 0;


uint16_t lasttouched = 0;
uint16_t currtouched = 0;



void loop() {
  unsigned long now = millis();
  updateTouchData(now);
  if (nextPettingReport < now) {
    nextPettingReport = now + 500;
    for (int i = 0; i < numPettingSensors; i++) {

      float confidence = 0.0;
      /// int i = 7;
      float hz = detectPetting(i, 128, &confidence);
      digitalWrite(LED_BUILTIN, hz > 0.0);
      Serial.print(hz);
      Serial.print(" ");
      //       Serial.print(confidence);
      //      Serial.print(" ");
    }
    Serial.println();
  }

}

void unused() {
  unsigned long now = millis();
  if (Serial && Serial.available()) {
    nextUpdate = now;
    nextUpdate2 = now + 5000;
    char c = Serial.read();
    if (c != '\n') {
      Serial.println();
      Serial.print("Got " );
      Serial.println(c);
      switch (c) {

        case 'b':
          stopMusic();
          playFile("baa % d.mp3", 1 + random(8));
          break;
        case '1':
          stopMusic();
          Serial.println(musicPlayer.playFullFile("baa1.mp3"));
          break;
        case '2':
          stopMusic();
          Serial.println(musicPlayer.playFullFile("baa2.mp3"));
          break;
        case '3':
          stopMusic();
          Serial.println(musicPlayer.playFullFile("baa3.mp3"));
          break;
        case '4':
          stopMusic();
          Serial.println(musicPlayer.playFullFile("baa4.mp3"));
          break;
        case 'h':
          stopMusic();
          Serial.println(musicPlayer.startPlayingFile("herd1.ogg"));
          break;
        case 'x':
          stopMusic();
          Serial.println(musicPlayer.startPlayingFile("scream1.mp3"));
          break;
        case 'z':
          stopMusic();
          Serial.println(musicPlayer.startPlayingFile("1000hz.wav"));
          break;
        case 'f':
          stopMusic();
          Serial.println(musicPlayer.startPlayingFile("feel.mp3"));
          break;
        case 'd':
          stopMusic();
          Serial.println(musicPlayer.startPlayingFile("drumbone.mp3"));
          break;
        case '5':
          stopMusic();
          Serial.println(musicPlayer.startPlayingFile("500hz.wav"));
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
  delay(50);
}
