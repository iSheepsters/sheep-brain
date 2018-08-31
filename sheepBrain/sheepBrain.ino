#include <Adafruit_SleepyDog.h>

const char * VERSION = "version as of Friday, 10am";
const boolean WAIT_FOR_SERIAL = false;

#include <Wire.h>
#include <TimeLib.h>
#include "util.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include "soundFile.h"
#include "slave.h"
#include "state.h"
#include "GPS.h"
#include "radio.h"
#include "logging.h"
#include "avdweb_SAMDtimer.h"


extern  void checkForCommand(unsigned long now);

// If the initial sheepNumber is anything other than 15, it will be
// rewritten by what is on the SD card. 15 is a placeholder, there
// shouldn't be a real sheep with that number.
// sheepNumber 14 is for shepherd

uint8_t sheepNumber = 15;
SheepInfo infoOnSheep[NUMBER_OF_SHEEP];

SheepInfo & getSheep(int s) {
  if (s < 1 || s > NUMBER_OF_SHEEP) {
    myprintf(Serial, "invalid request for sheep %d\n", s);
    return infoOnSheep[0];
  }
  return infoOnSheep[s - 1];
}


unsigned long nextPettingReport;

boolean debugTouch = false;
SoundCollection boredSounds(0);
SoundCollection ridingSounds(2);
SoundCollection readyToRideSounds(2);

SoundCollection attentiveSounds(1);
SoundCollection notInTheMoodSounds(1);
SoundCollection violatedSounds(1);
SoundCollection baaSounds(0);

// not a state
SoundCollection endOfRideSounds(3);
SoundCollection inappropriateTouchSounds(4);
SoundCollection seperatedSounds(1);




unsigned long setupFinished = 0;
uint16_t minutesUptime() {
  return ( millis() - setupFinished) / 1000 / 60;
}

unsigned long nextTestDistress = 0xffffffff; // disable test distreess

volatile unsigned long lastInterrupt = 0;
volatile  long longestInterval = 0;
volatile unsigned int maxAvail = 0;
volatile int maxInterruptTime = 0;

unsigned long updateGPSLatency() {
  unsigned long now_us = millis();
  if (lastInterrupt != 0) {
    long diff = now_us - lastInterrupt;
    if (longestInterval < diff) {
      longestInterval = diff;
    }
  }
  lastInterrupt = now_us;
  int a = Serial1.available();
  if (maxAvail < a)
    maxAvail = a;
  return now_us;
}
void ISR_GPS(struct tc_module *const module_inst)
{
  unsigned long start = micros();
  if (read_gps_in_interrupt) {

    updateGPSLatency();
    while (true) {
      char c = GPS.read();
      if (!c) break;
      c = GPS.read();
      if (!c) break;
      c = GPS.read();
      if (!c) break;
    }
  }
  if (musicPlayerReady)
    musicPlayer.feedBuffer();
  unsigned long end = micros();
  long diff = end - start;
  if (maxInterruptTime < diff)
    maxInterruptTime = diff;

}

SAMDtimer IRS_timer5 = SAMDtimer(5, ISR_GPS, 55000, 0);

void testSwapLeftRight(uint8_t touched) {
  myprintf(Serial, "testSwapLeftRight %02x -> %02x\n",
           touched, swapLeftRightSensors(touched));
}
void setup() {
  memset(&infoOnSheep, 0, sizeof (infoOnSheep));
  pinMode(LED_BUILTIN, OUTPUT);
  //  pinMode(SHDN_MAX9744, OUTPUT);
  //  digitalWrite(SHDN_MAX9744, LOW);
  Wire.begin();
  randomSeed(analogRead(0));
  delay(100);
  Serial.begin(115200);
  if (WAIT_FOR_SERIAL) while (!Serial && millis() < 15000) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      setupDelay(200);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      setupDelay(200);
    }
  int countdownMS = Watchdog.enable(14000);
  myprintf(Serial, "Watchdog set, %d ms timeout\n", countdownMS);

  if (false) {
    testSwapLeftRight(0);
    testSwapLeftRight(0x04);
    testSwapLeftRight(0x08);
    testSwapLeftRight(0x0f);
    testSwapLeftRight(0x37);
    testSwapLeftRight(0x3B);
  }


  if (setupRadio())
    Serial.println("radio found");
  else
    Serial.println("radio not found!");

  if (useGPS) {
    if (setupGPS())
      Serial.println("GPS found");
    else
      logDistress("GPS not found!");
  } else
    Serial.println("Skipping GPS");



  if (!setupSD()) {
    Serial.println("setupSD failed");
    Serial.println("huh");
    logDistress("SD card not found");
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      setupDelay(1000);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      setupDelay(1000);

    }
  }

  File configFile = SD.open("config.txt");
  if (!configFile) {
    while (true) {
      logDistress("no config.txt file");
      while (true) {
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        setupDelay(1000);                     // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        setupDelay(1000);
      }
    }
  }
  int s = configFile.parseInt();
  configFile.close();
  myprintf(Serial, "config file contains %d\n", s);
  if (s == 9) {
    // On-playa swap: Larry replaced by Baarak
    myprintf(Serial, "on playa swap; Larry replaced by Baarak");
    s = 10;
  }
  if (sheepNumber == PLACEHOLDER_SHEEP) {
    sheepNumber = s;
    myprintf(Serial, "I am sheep %d\n", sheepNumber);
  } else {
    myprintf(Serial,
             "Sheep number hard coded as %d, ignoring value of %d on sd card\n",
             sheepNumber, s);
  }


  setupLogging();
  myprintf(logFile, "Start of log for sheep %d\n", sheepNumber);

  if (useSound) {


    //    digitalWrite(SHDN_MAX9744, HIGH);
    //    setupDelay(10);
    //    pinMode(SHDN_MAX9744, INPUT);
    //    setupDelay(100);
    setupSound();
    Serial.println("Sound set up, examining sound files");

    boredSounds.load("BORED");
    ridingSounds.load("RIDNG");
    readyToRideSounds.load("RDRID");
    endOfRideSounds.load("EORID");
    attentiveSounds.load("ATTNT");
    notInTheMoodSounds.load("NMOOD");
    violatedSounds.load("VIOLT");
    inappropriateTouchSounds.load("INAPP");
    seperatedSounds.load("SEPRT");

    baaSounds.loadCommon("baa");
    baaSounds.playSound(millis(), false);

  } else
    Serial.println("Skipping sound");

  if (useTouch) {
    Serial.println("Setting up touch");
    setupTouch();
    Serial.println("touch set up");
  } else
    Serial.println("Skipping touch");

  if (useSlave) {
    Serial.println("Setting up comm");
    setupComm();
  } else
    Serial.println("Skipping comm");

  //discardGPS();
  setupFinished = millis();
  myprintf(Serial, " %dms set up time\n", setupFinished);
  myprintf(logFile, "sheep %d ready\n", sheepNumber);

  updateLog(setupFinished);
  quickGPSUpdate();
  if (useGPSinterrupts) {
    lastInterrupt = setupFinished;
    read_gps_in_interrupt = true;
    IRS_timer5.enableInterrupt(1);
  }
  nextPettingReport = setupFinished + 2000;
  myprintf(Serial, "setup complete, sheep %d\n",
  sheepNumber);
  Serial.println(VERSION);
  randomSeed(analogRead(0));

}


unsigned long lastReport = 0;


const boolean TRACE = false;

unsigned long lastLoop = 0;

int32_t prevLat = -1;
int32_t prevLong = -1;
const uint16_t REPORT_INTERVAL = 15000;
void loop() {
  Watchdog.reset();
  interrupts();
  SheepInfo & me = getSheep();
  me.time = now();
  if (totalYield > 0) {
    myprintf(Serial, "total yield %dms\n", totalYield);
    totalYield = 0;
  }

  me.uptimeMinutes = minutesUptime();
  me.batteryVoltageRaw =  batteryVoltageRaw();
  me.errorCodes = 0;
  unsigned long now = millis();
  if ((now / 500) % 2 == 1)
    digitalWrite(LED_BUILTIN, HIGH);
  else
    digitalWrite(LED_BUILTIN, LOW);


  if (false && nextTestDistress < now) {
    nextTestDistress = now + random(150000, 170000);
    logDistress("This is a test distress message from sheep %d, next test in %d seconds",
                sheepNumber, (nextTestDistress - now) / 1000);
  }
  if (useGPS) {
    quickGPSUpdate();
    updateGPS(now);
  }

  if (useSound && playSound)
    updateSound(now);

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }
  if (useRadio)
    updateRadio();

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }
  if (lastReport + REPORT_INTERVAL < now ) {
    myprintf(Serial, "%d State %s, %f volts,  %d minutes uptime, %d GPS fixes, %2d:%02d:%02d\n",
             now, currentSheepState->name,
             batteryVoltage(),  minutesUptime(), fixCount,
             hour(), minute(), second());
    myprintf(Serial, "  %dms interrupt interval, %dus max interrupt time, %d max avail\n",
             longestInterval, maxInterruptTime, maxAvail);


    if (currentSoundFile != NULL)
      myprintf(Serial, "  playing %s\n", currentSoundFile -> name);
    else if (musicPlayer.playingMusic)
      Serial.println("  Playing unknown sound");
    else {
      myprintf(Serial, "next ambient sound in %d ms\n", nextAmbientSound - now);
      myprintf(Serial, "next baa in %d ms\n", nextBaa - now);
    }
    if (ampOn)
      Serial.println("  Amplifier on");

    myprintf(Serial, "  GPS readings %d good, %d bad\n", total_good_GPS, total_bad_GPS);
    if (getGPSFixQuality)
      myprintf(Serial, "  Fix quality = %d, Satellites = %d\n", GPS.fixquality, GPS.satellites);
    myprintf(Serial, "  Location = %d, %d; longest gps void = %dms,  Feet from man = ",
             latitudeDegreesAvg, longitudeDegreesAvg, longestGPSvoid);
    Serial.println(distanceFromMan);
    if (now < 10000)
      longestGPSvoid = 0;


    if (prevLat != -1) {
      float speed = 1000 * distance_between_fixed_in_feet(latitudeDegreesAvg, longitudeDegreesAvg, prevLat, prevLong)
                    / (now - lastReport);
      Serial.print("  Speed : ");
      Serial.print(speed, 2);
      Serial.println(" ft / sec");
    }
    prevLat = latitudeDegreesAvg;
    prevLong = longitudeDegreesAvg;


    longestInterval = 0;
    maxAvail = 0;
    maxInterruptTime = 0;
    time_t BRC_time = BRC_now();

    if (false)
      myprintf(Serial, "BRC time: %2d:%02d:%02d\n", hour(BRC_time), minute(BRC_time), second(BRC_time));

    lastReport = now;
    if (useLog) {
      if (TRACE)
        Serial.println("Updating log");
      updateLog(now);
      if (TRACE)
        Serial.println("Log updated");
    }
  }


  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }
  if (useTouch) {

    if (TRACE)
      Serial.println("updateTouchData");
    unsigned long startTouch = micros();
    updateTouchData(now, false);
    //Serial.println(micros()-startTouch);
    if (debugTouch) {
      Serial.print("TD ");
      for (int i = 0; i < numTouchSensors; i++)
        myprintf(Serial, " %3d %3d %3d  ", stableValue[i],  currentValue[i], sensorValue((TouchSensor)i));
      Serial.println();
    }
  }

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }

  if (doUpdateState && now > 10000 && useTouch) {
    if (TRACE) Serial.println("updateState");
    updateState(now);
    if (useSlave)
      sendComm(now);


    for (int i = 0; i < numTouchSensors; i++) {
      if (((newTouched >> i) & 1 ) != 0) {
        myprintf(Serial, "touched %d - %d\n", i, sensorValue((enum TouchSensor)i));
        //playFile(" %d.mp3", i);
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
  }

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }

  if (useSound && playSound) {
    if (TRACE) Serial.println("checkForNextAmbientSound");
    checkForNextAmbientSound(now);
  }

  if (useGPS && !useGPSinterrupts) {
    quickGPSUpdate();
  }
  if (TRACE) Serial.println("checkForCommand");
  checkForCommand(now);

  //yield(10);
}

void checkForNextAmbientSound(unsigned long now) {
  if (!useSound || !playSound)
    return;

  if (musicPlayer.playingMusic || soundPlayedRecently(now)) {
    return;
  }
  if (now > nextAmbientSound) {
    if (!isInFriendlyTerritory() && random(2) == 0 && seperatedSounds.playSound(now, true))  {
      unsigned long delay = 20000;
      if (currentSoundFile -> duration > 0 && currentSoundFile -> duration < delay)
        delay = currentSoundFile -> duration ;
      delay = (delay + random(20000)) * howCrowded();
      myprintf(Serial, "Started playing seperated sound, next in %dms\n", delay);
      nextAmbientSound = now + delay;
      return;
    }
    if (currentSheepState -> playSound(now, true)) {
      unsigned long delay = 20000;
      if (currentSoundFile -> duration > 0 && currentSoundFile -> duration < delay)
        delay = currentSoundFile -> duration ;
      delay = (delay + random(20000)) * howCrowded();
      myprintf(Serial, "Started playing ambient sound, next in %dms\n", delay);
      nextAmbientSound = now + delay;
      return;
    } else  if (baaSounds.playSound(now, true)) {
      myprintf(Serial, "No ambient sound available, baa-ing\n");

      unsigned long delay = random(10000, 20000) * howCrowded();
      nextAmbientSound = now + delay;
      nextBaa = now + delay + 10000;
    } else {
      myprintf(Serial, "No ambient or baa sound available\n");
      nextAmbientSound = now + 5000;
    }
  }
  if (now > nextBaa && nextBaa + 8000 < nextAmbientSound) {
    if (baaSounds.playSound(now, true)) {
      unsigned long delay = random(10000, 20000) * howCrowded();
      Serial.println("Started playing baa");
      myprintf(Serial, "Started playing baa sound, next in %dms\n", delay);
      nextBaa = now + delay;
      return;
    } else
      nextBaa = now + 3000 + random(1, 5000);
  }

}

unsigned long nextCommandCheck = 0;
void checkForCommand(unsigned long now) {
  if (nextCommandCheck > now)
    return;
  nextCommandCheck = now + 200;
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
          attentiveSounds.playSound(now, false);
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
        case 't':
          dumpTouchData();
          debugTouch = !debugTouch;
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

          turnAmpOn();
          break;
      }
    }
  }
}


