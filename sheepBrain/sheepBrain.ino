#include <Adafruit_SleepyDog.h>


#include <Wire.h>
#include "all.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include "soundFile.h"
#include "comm.h"
#include "GPS.h"
#include "radio.h"
#include <RH_RF95.h>

extern  void checkForCommand(unsigned long now);
const boolean useSound = true;
const boolean useOLED = false;
const boolean useTouch = true;
const boolean useGPS = true;

const uint8_t SHDN_MAX9744 = 11;

unsigned long nextPettingReport;

enum State {
  Bored,
  Welcoming,
  Riding
};

SoundCollection boredSounds;
SoundCollection ridingSounds;
SoundCollection welcomingSounds;
SoundCollection baaSounds;

/* return voltage for 12v battery */
float batteryVoltage() {
  return analogRead(A2) * 3.3 * 5.7 / 1024;
}
unsigned long nextCalibration = 2000;
enum State currState = Bored;
void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SHDN_MAX9744, OUTPUT);
  digitalWrite(SHDN_MAX9744, LOW);
  Wire.begin();
  randomSeed(analogRead(0));
  Serial.begin(115200);
  while (!Serial && millis() < 10000) {
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


  if (useGPS) {
    if (setupGPS())
      Serial.println("GPS found");
    else
      Serial.println("GPS not found!");

  }

  if (setupRadio())
    Serial.println("radio found");
  else
    Serial.println("radio not found!");

  if (useSound) {
    pinMode(SHDN_MAX9744, INPUT);
    delay(100);
    setupSound();

    setupSD();

    boredSounds.load("bored");
    boredSounds.list();
    myprintf(Serial, "%d bored files\n", boredSounds.count);
    // boredSounds.playSound(0);

    ridingSounds.load("riding");
    myprintf(Serial, "%d riding files\n", ridingSounds.count);

    welcomingSounds.load("w");
    myprintf(Serial, "%d welcoming files\n", welcomingSounds.count);
    baaSounds.load("baa");


    myprintf(Serial, "%d baa files\n", baaSounds.count);
    if (musicPlayer.startPlayingFile("baa/baa0.mp3")) {
      Serial.println("Started baa0");
      while (musicPlayer.playingMusic) {
        Serial.println(millis());
        musicPlayer.feedBuffer();
        delay(100);
      }

    } else {
      Serial.println("Could not start baa0");
    }

    //baaSounds.playSound(0);

  }
  if (useTouch) {
    Serial.println("Setting up touch");
    setupTouch();

  }

  setupComm();

  Serial.println("Ready");
  nextPettingReport = millis() + 2000;

  int countdownMS = Watchdog.enable(4000);
  myprintf(Serial, "Watchdog set, %d ms timeout\n", countdownMS);

  nextCalibration = millis() + 1000;
}

unsigned long nextBaa = 10000;


void updateState(unsigned long now) {
  if (touchDuration(BACK_SENSOR) > 1200 &&
      (touchDuration(LEFT_SENSOR) > 550 || touchDuration(RIGHT_SENSOR) > 550 )
      || touchDuration(BACK_SENSOR) > 9500)  {
    if (currState != Riding) {
      ridingSounds.playSound(now, false);
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
        boredSounds.playSound(now, false);
        nextBaa = now + 12000 + random(1, 15000);
      }
      currState = Bored;
      Serial.println("bored");
    }
  }
  else {
    if (currState != Welcoming) {
      if (!musicPlayer.playingMusic || currState == Bored) {
        welcomingSounds.playSound(now, false);
        nextBaa = now + 12000 + random(1, 15000);
      }
      currState = Welcoming;
      Serial.println("welcoming");
    }

  }

}



void loop0() {
  Watchdog.reset();
  unsigned long now = millis();
  updateSound(now);
  updateGPS();
  if (nextCalibration < now) {
    Serial.println("Calibrate");
    calibrate();
    nextCalibration = 0x7fffffff;
  }
  updateTouchData(now);
  uint8_t t = 0;
  for (int i = 0; i < 6; i ++) {
    if (cap.filteredData(i) < cap.baselineData(i))
      t |= 1 << i;
  }

  uint8_t v = sendSlave(0, t);
  if (v != 0) {
    Serial.print("Transmission error: " );
    Serial.println(v);
  }
  //  Serial.print(t, HEX);
  //  Serial.print("  ");
  //  Serial.print(currTouched, HEX);
  //  Serial.print("  ");
  if (nextCalibration < 20000) {
    for (int i = 0; i < 6; i++) {
      Serial.print(cap.filteredData(i));
      Serial.print(" ");
      Serial.print(cap.baselineData(i));
      Serial.print("  ");
    }
  } else
    for (int i = 0; i < 6; i++) {
      Serial.print(cap.filteredData(i) - cap.baselineData(i));
      Serial.print("  ");

    }
  Serial.println();
  checkForCommand(now);
  delay(200);


}

unsigned long nextReport = 0;
void loop() {
  Watchdog.reset();

  //  Serial.println(millis());
  unsigned long now = millis();
  updateSound(now);
  if (nextReport < now ) {
    switch (currState) {
      case Bored:
        myprintf(Serial, "%d State Bored\n", now);
        break;
      case Welcoming:
        myprintf(Serial, "%d State Welcoming\n", now);
        break;
      case Riding:
        myprintf(Serial, "%d State Riding\n", now);
        break;
      default:
        myprintf(Serial, "%d State unknown\n", now);
        break;
    }
    nextReport = now + 5000;
  }
  updateGPS();

  if (nextCalibration < now) {
    Serial.println("Calibrate");
    calibrate();
    nextCalibration = 0x7fffffff;
  }

  updateTouchData(now);

  uint8_t v = sendSlave(0, currTouched);
  if (v != 0) {
    Serial.print("Transmission error: " );
    Serial.println(v);
  }
  if (false) {
    if (musicPlayer.playingMusic)
      Serial.print("M ");
    else
      Serial.print("- ");
    Serial.print(currTouched, HEX);
    Serial.print(" ");
    Serial.print(newTouched, HEX);
    Serial.print(" ");

    for (int i = 0; i < 6; i++) {
      Serial.print(cap.filteredData(i) - cap.baselineData(i));
      Serial.print("  ");

    }
    Serial.println();
  }


  if (now > 10000) {
    updateState(now);
    //sendSlave(1, (uint8_t) currState);
  }

  for (int i = 0; i < numTouchSensors; i++) {
    if (((newTouched >> i) & 1 ) != 0) {
      myprintf(Serial, "touched %d\n", i);
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
  checkForCommand(now);
  delay(100);
}

void checkForSound() {
  unsigned long now = millis();
  if (nextBaa < now && !musicPlayer.playingMusic) {
    if (random(3) == 0) {
      baaSounds.playSound(now, true);
      nextBaa = now + 4000 + random(1, 10000);
    } else {
      switch (currState) {
        case Bored:
          boredSounds.playSound(now, true);
          break;
        case Welcoming:
          welcomingSounds.playSound(now, true);
          break;
        case Riding:
          ridingSounds.playSound(now, true);
          break;
      }
    }
    nextBaa = now + 12000 + random(1, 15000);
  }
}
void checkForCommand(unsigned long now) {
  if (Serial && Serial.available()) {
    char c = Serial.read();
    if (c != '\n') {
      Serial.println();
      Serial.print("Got " );
      Serial.println(c);
      switch (c) {
        case 'c':
          Serial.println("Calibrate");
          calibrate();
          break;
        case 'a':
          playFile("baa%d.mp3", 1 + random(8));
          break;
        case 'w':
          welcomingSounds.playSound(now, false);
          break;
        case 'r':
          ridingSounds.playSound(now, false);
          break;
        case 'b':
          boredSounds.playSound(now, false);
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



