#include <Adafruit_SleepyDog.h>


#include <Wire.h>
#include <TimeLib.h>
#include "all.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include "soundFile.h"
#include "slave.h"
#include "GPS.h"
#include "radio.h"
#include "logging.h"


extern  void checkForCommand(unsigned long now);
const boolean useSound = true;
const boolean useOLED = false;
const boolean useTouch = true;
const boolean useGPS = true;
const boolean useSlave = true;
const boolean useRadio = true;

uint8_t sheepNumber = 14;
SheepInfo infoOnSheep[NUMBER_OF_SHEEP];

//const uint8_t SHDN_MAX9744 = 10;

unsigned long nextPettingReport;


const char * stateName(State s) {
  switch (s) {
    case Bored: return "Bored";
    case Welcoming: return "Welcoming";
    case Riding: return "Riding";
    default: return "Unknown";
  }
}
SoundCollection boredSounds(0);
SoundCollection ridingSounds(2);
SoundCollection welcomingSounds(1);
SoundCollection baaSounds(0);

/* return voltage for 12v battery */
uint16_t batteryVoltageRaw() {
  return analogRead(A2);
}


SheepInfo & getSheep() {
  return getSheep(sheepNumber);
}

SheepInfo & getSheep(int s) {
  if (s < 1 || s > NUMBER_OF_SHEEP) {
    myprintf(Serial, "invalid request for sheep %d\n", s);
    return infoOnSheep[0];
  }
  return infoOnSheep[s - 1];
}

float batteryVoltage() {
  return batteryVoltageRaw() * 3.3 * 5.7 / 1024;
}
uint8_t batteryCharge() {
  // https://www.energymatters.com.au/components/battery-voltage-discharge/
  float v = batteryVoltage();
  v = 50 + (v - 12.2) / 0.2 * 25;
  if (v > 100) v = 100;
  if (v < 0) v = 0;
  return (int) v;
}
unsigned long nextCalibration = 6000;
enum State currState = Bored;
void setup() {
  sheepNumber = 1;
  memset(&infoOnSheep, 0, sizeof (infoOnSheep));
  pinMode(LED_BUILTIN, OUTPUT);
  //  pinMode(SHDN_MAX9744, OUTPUT);
  //  digitalWrite(SHDN_MAX9744, LOW);
  Wire.begin();
  randomSeed(analogRead(0));
  Serial.begin(115200);
  if (true) while (!Serial && millis() < 10000) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(200);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      delay(200);
    }
  int countdownMS = Watchdog.enable(14000);
  myprintf(Serial, "Watchdog set, %d ms timeout\n", countdownMS);

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
      logDistress("GPS not found!");

  } else
    Serial.println("Skipping GPS");



  if (setupRadio())
    Serial.println("radio found");
  else
    Serial.println("radio not found!");
  if (!setupSD()) {
    logDistress("SD card not found");
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(1000);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      delay(1000);

    }
  }
  File configFile = SD.open("config.txt");
  sheepNumber = configFile.parseInt();
  myprintf(Serial, "I am sheep %d\n", sheepNumber);
  configFile.close();

  setupLogging();
  myprintf(logFile, "Start of log for sheep %d\n", sheepNumber);

  if (useSound) {


    //    digitalWrite(SHDN_MAX9744, HIGH);
    //    delay(10);
    //    pinMode(SHDN_MAX9744, INPUT);
    //    delay(100);
    setupSound();
    Serial.println("Sound set up");


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
      logDistress("Could not start baa0");
    }

    //baaSounds.playSound(0);

  }  else  Serial.println("Skipping sound");
  if (useTouch) {
    Serial.println("Setting up touch");
    setupTouch();
  } else
    Serial.println("Skipping touch");
  setupComm();

  Serial.println("Ready");
  myprintf(logFile, "sheep %d ready\n", sheepNumber);
  updateLog(millis());
  nextPettingReport = millis() + 2000;


  nextCalibration = millis() + 5000;
}

unsigned long nextBaa = 10000;


void updateState(unsigned long now) {
  if (false) myprintf(Serial, "state %d: %6d %6d %6d %6d %6d\n", currState,
                        combinedTouchDuration(HEAD_SENSOR),
                        combinedTouchDuration(BACK_SENSOR),
                        combinedTouchDuration(LEFT_SENSOR),
                        combinedTouchDuration(RIGHT_SENSOR),
                        combinedTouchDuration(RUMP_SENSOR));

  if (touchDuration(BACK_SENSOR) > 1200 &&
      (touchDuration(LEFT_SENSOR) > 550 || touchDuration(RIGHT_SENSOR) > 550 ||  touchDuration(RUMP_SENSOR) > 550 )
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
  else if ( touchDuration(BACK_SENSOR) > 100 ||
            touchDuration(LEFT_SENSOR) > 100 || touchDuration(RIGHT_SENSOR) > 100
            ||  touchDuration(RUMP_SENSOR) > 100  ||  touchDuration(HEAD_SENSOR) > 100) {
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


unsigned long nextReport = 0;

const boolean TRACE = false;
void loop() {
  Watchdog.reset();

  SheepInfo & me =  getSheep();
  me.time = now();
  me.uptimeMinutes = millis() / 1000 / 60;
  me.batteryVoltageRaw =  batteryVoltageRaw();
  me.errorCodes = 0;
  unsigned long now = millis(); 
  if (useSound)
    updateSound(now);
  if (useGPS)
    updateGPS(now);
  if (useRadio)
    updateRadio();
  if (nextReport < now ) {
    myprintf(Serial, "%d State %s, %f volts, %d GPS fixes, %2d:%02d:%02d\n", now, stateName(currState),
             batteryVoltage(), fixCount,
             GPS.hour, GPS.minute, GPS.seconds);
    time_t BRC_time = BRC_now();

    myprintf(Serial, "BRC time: %2d:%02d:%02d\n", hour(BRC_time), minute(BRC_time), second(BRC_time));

    nextReport = now + 5000;
    if (TRACE)
      Serial.println("Updating log");
    updateLog(now);
    if (TRACE)
      Serial.println("Log updated");
  }

  if (useTouch) {
    if (nextCalibration < now) {
      Serial.println("Calibrate");
      calibrate();
      nextCalibration = 0x7fffffff;
    }

    if (TRACE) Serial.println("updateTouchData");
    updateTouchData(now, false);

    if (useSlave) {
      if (TRACE) Serial.println("sendSlave");
      uint8_t v = sendSlave(0, currTouched);
      if (v != 0) {
        Serial.print("Transmission error: " );
        Serial.println(v);
      }
    }
    //dumpTouchData();

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
  }


  if (now > 10000) {
    if (TRACE) Serial.println("updateSlave");
    updateState(now);
    if (useSlave)
      sendSlave(1, (uint8_t) currState);
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

  if (useSound) {
    if (TRACE) Serial.println("checkForSound");
    checkForSound();
  }
  if (TRACE) Serial.println("checkForCommand");
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




void loop0() {
  Watchdog.reset();
  unsigned long now = millis();
  updateSound(now);
  updateGPS(now);
  if (nextCalibration < now) {
    Serial.println("Calibrate");
    calibrate();
    nextCalibration = 0x7fffffff;
  }
  updateTouchData(now, false);
  uint8_t t = 0;
  for (int i = 0; i < 6; i ++) {
    if (cap.filteredData(i) < cap.baselineData(i))
      t |= 1 << i;
  }

  if (useSlave) {
    uint8_t v = sendSlave(0, t);
    if (v != 0) {
      Serial.print("Transmission error: " );
      Serial.println(v);
    }
  }
  //  Serial.print(t, HEX);
  //  Serial.print("  ");
  //  Serial.print(currTouched, HEX);
  //  Serial.print("  ");
  dumpTouchData();
  checkForCommand(now);
  delay(200);


}


