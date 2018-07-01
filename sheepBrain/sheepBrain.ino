
#include <Wire.h>
#include "all.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include "Adafruit_ZeroFFT.h"

u
uint16_t touchData



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
  setupTouch();
  setupSound();




  //  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  //
  //  display.display();
  //  Serial.println("OLED begun");
  //  Serial.print("OLED status: " );
  //  Serial.println(display.ssd1306_status());

  // if you're using Bluefruit or LoRa/RFM Feather, disable the BLE interface
  //pinMode(8, INPUT_PULLUP);

  Serial.println("Ready");

}

unsigned long nextUpdate = 0;
unsigned long nextUpdate2 = 0;


uint16_t lasttouched = 0;
uint16_t currtouched = 0;

void loop() {
  unsigned long now = millis();
  if (true) {

    currtouched = cap.touched();
    uint16_t newTouch = currtouched & ~lasttouched;
    if (currtouched != 0) {
      Serial.print(currtouched, HEX);
      Serial.print(" ");
      Serial.println(newTouch, HEX);
    }
    lasttouched = currtouched;
    for (int i = 0; i < 13; i++) {
      if ((newTouch >> i) & 1 == 1) {
        Serial.print("touched ");
        Serial.println(i);
        playFile("%d.mp3", i);
        completeMusic();
        playFile("baa%d.mp3", i % 10);
      }
    }


  }

  
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
          playFile("baa%d.mp3", 1 + random(8));
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
