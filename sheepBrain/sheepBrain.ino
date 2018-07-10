#include <Adafruit_SleepyDog.h>

#include <Wire.h>
#include "all.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include "soundFile.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const boolean useSound = false;
const boolean useOLED = false;
const boolean useTouch = false;
Adafruit_SSD1306 display = Adafruit_SSD1306();
unsigned long nextPettingReport;

enum State {
  Bored,
  Welcoming,
  Riding
};

SoundCollection boredSounds;
SoundCollection ridingSounds;
SoundCollection welcomingSounds;


enum State currState = Bored;
void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  Serial.begin(115200);
  while (!Serial) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
  }


  for (int i = 1; i < 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(20);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(20);
    Serial.println(i);
  }


  setupSD();
  boredSounds.load("bored");
  ridingSounds.load("riding");
  welcomingSounds.load("w");
  myprintf(Serial, "%d bored files\n", boredSounds.count);

  myprintf(Serial, "%d riding files\n", ridingSounds.count);
  myprintf(Serial, "%d welcoming files\n", welcomingSounds.count);

  if (useSound)
    setupSound();
  if (useTouch) {
    Serial.println("Setting up touch");
    setupTouch();
  }



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
  int countdownMS = Watchdog.enable(4000);
  myprintf(Serial, "Watchdog set, %d ms timeout\n", countdownMS);
}

unsigned long nextBaa = 10000;


void updateState(unsigned long now) {
  if (touchDuration(BACK_SENSOR) > 1200 &&
      (touchDuration(LEFT_SENSOR) > 550 || touchDuration(RIGHT_SENSOR) > 550 )
      || touchDuration(BACK_SENSOR) > 9500)  {
    if (currState != Riding) {
      playRiding();
      nextBaa = now + 12000 + random(1, 15000);
      currState = Riding;
      Serial.println("riding");
    }


  }
  else if (untouchDuration(BACK_SENSOR) > 10000
           && untouchDuration(HEAD_SENSOR) > 10000
           && untouchDuration(RUMP_SENSOR) > 10000) {
    if (currState != Bored) {
      if (!musicPlayer.playingMusic) {
        playBored();
        nextBaa = now + 12000 + random(1, 15000);
      }
      currState = Bored;
      Serial.println("bored");
    }
  }
  else {
    if (currState != Welcoming) {
      if (!musicPlayer.playingMusic || currState == Bored) {
        playWelcoming();
        nextBaa = now + 12000 + random(1, 15000);
      }
      currState = Welcoming;
      Serial.println("welcoming");
    }

  }

}

void loop() {
  Watchdog.reset();
  
}
void loop0() {
  Watchdog.reset();
  unsigned long now = millis();
  updateTouchData(now);
  if (now > 10000)
    updateState(now);
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
    if (random(3) == 0) {
      baa();
      nextBaa = now + 4000 + random(1, 10000);
    } else {
      switch (currState) {
        case Bored:
          playBored();
          break;
        case Welcoming:
          playWelcoming();
          break;
        case Riding:
          playRiding();
          break;
      }
    }
    nextBaa = now + 12000 + random(1, 15000);
  }
  if (Serial && Serial.available()) {
    char c = Serial.read();
    if (c != '\n') {
      Serial.println();
      Serial.print("Got " );
      Serial.println(c);
      switch (c) {

        case 'a':
          playFile("baa%d.mp3", 1 + random(8));
          break;
        case 'w':
          playWelcoming();
          break;
        case 'r':
          playRiding();
          break;
        case 'b':
          playBored();
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

          setVolume(thevol);
          break;
      }
    }
  }

}
