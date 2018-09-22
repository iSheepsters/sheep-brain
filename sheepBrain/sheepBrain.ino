#include <Adafruit_SleepyDog.h>
#include <MemoryFree.h>
const char * VERSION = "version as of 9/21, 9:30pm";
const boolean WAIT_FOR_SERIAL = true;

#include <Wire.h>
#include <TimeLib.h>
#include "util.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include "soundFile.h"
#include "comm.h"
#include "state.h"
#include "GPS.h"
#include "radio.h"
#include "logging.h"
#include "avdweb_SAMDtimer.h"
#include "scheduler.h"


extern  void checkForCommand();

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
SoundCollection changeSounds(0);




unsigned long setupFinished = 0;
uint16_t minutesUptime() {
  return ( millis() - setupFinished) / 1000 / 60;
}

unsigned long nextTestDistress = 0xffffffff; // disable test distreess

volatile unsigned long lastInterrupt = 0;
volatile  long longestInterval = 0;
volatile unsigned int maxAvail = 0;
volatile int maxInterruptTime = 0;

void checkForNextAmbientSound();

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

unsigned long nextActivityReport = 0;
void ISR_GPS(struct tc_module *const module_inst)
{
  unsigned long start = micros();
  unsigned long ms = millis();
  if (currentActivityStartedAt + 500 < ms && nextActivityReport < ms) {
    myprintf(Serial, "activity %d.%d, started %dms ago\n",
             currentActivity, currentSubActivity,
             ms-currentActivityStartedAt);
    nextActivityReport = ms + 1000;
  }
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
  Wire.begin();
  randomSeed(analogRead(0));
  delay(100);
  Serial.begin(115200);
  if (WAIT_FOR_SERIAL) while (!Serial && millis() < 10000) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      setupDelay(200);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      setupDelay(200);
    }
  int countdownMS = Watchdog.enable(20000);
  myprintf(Serial, "Watchdog set, %d ms timeout\n", countdownMS);

  myprintf(Serial, "Free memory = %d\n", freeMemory());
  if (useSlave) {
    Serial.println("Setting up comm");
    setupComm();
    sendBoot();
  } else
    Serial.println("Skipping comm");


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

  if (useSound) {
    Serial.println("Setting up sound");
    setupSound();
    Serial.println("Sound setup");
  } else
    Serial.println("Skipping sound");

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
  myprintf(Serial, "Free memory = %d\n", freeMemory());
  int s = 1;
  File configFile = SD.open("config.txt");
  if (!configFile) {

    logDistress("no config.txt file");

  } else {
    s = configFile.parseInt();
    myprintf(Serial, "config file contains %d\n", s);
    configFile.close();
  }

  if (s < 0 || s >= NUMBER_OF_SHEEP)
    s = 1;

  if (sheepNumber == PLACEHOLDER_SHEEP) {
    sheepNumber = s;
    myprintf(Serial, "I am sheep %d\n", sheepNumber);

  } else {

    myprintf(Serial,
             "Sheep number hard coded as %d, ignoring value of %d on sd card\n",
             sheepNumber, s);
  }

  writeSheepNumber(sheepToSwitchTo(sheepNumber));
  setupLogging();
  myprintf(logFile, "Start of log for sheep %d\n", sheepNumber);

  if (useSound) {


    //    digitalWrite(SHDN_MAX9744, HIGH);
    //    setupDelay(10);
    //    pinMode(SHDN_MAX9744, INPUT);
    //    setupDelay(100);

    myprintf(Serial, "Free memory = %d\n", freeMemory());
    Serial.println("Sound set up, examining sound files");

    loadPerSheepSounds();

    changeSounds.loadCommon("change");
    baaSounds.loadCommon("baa");
    baaSounds.playSound(millis(), false);
    myprintf(Serial, "Free memory = %d\n", freeMemory());
  } else
    Serial.println("Skipping sound");

  if (useTouch) {
    Serial.println("Setting up touch");
    setupTouch();
    Serial.println("touch set up");
  } else
    Serial.println("Skipping touch");



  addScheduledActivity(10000, generateReport, "report");
  if (useSound && playSound)
    addScheduledActivity(500, checkForNextAmbientSound, "ambient sound");
  setupState();
  if (useCommands) {
    addScheduledActivity(500, checkForCommand, "commands");
  }
  updateLog(setupFinished);
  quickGPSUpdate();
  if (useGPSinterrupts) {
    lastInterrupt = setupFinished;
    read_gps_in_interrupt = true;
    IRS_timer5.enableInterrupt(1);
  }


  if (useSlave) {
    Serial.println("Getting last boot from teensy");
    logBoot();
  }

  Serial.println();
  sendActivity(101);
  setupFinished = millis();
  myprintf(Serial, " %dms set up time\n", setupFinished);
  myprintf(logFile, "sheep %d ready\n", sheepNumber);
  listScheduledActivities();

  myprintf(Serial, "  Free memory = %d\n", freeMemory());
  myprintf(Serial, "setup complete, sheep %d\n",
           sheepNumber);
  Serial.println(VERSION);
  randomSeed(analogRead(0));
  startSchedule();

}


const boolean TRACE = false;

unsigned long lastReport = 0;

int32_t prevLat = -1;
int32_t prevLong = -1;

void generateReport() {
  unsigned long now = millis();
  myprintf(Serial, "%d/%d State %s, %f volts,  %d minutes uptime, %d GPS fixes, %2d:%02d:%02d\n",
           sheepNumber, now, currentSheepState->name,
           batteryVoltage(),  minutesUptime(), fixCount,
           hour(), minute(), second());
  myprintf(Serial, "  %dms interrupt interval, %dus max interrupt time, %d max avail\n",
           longestInterval, maxInterruptTime, maxAvail);

  myprintf(Serial, "  Free memory = %d\n", freeMemory());

  if (thevol != INITIAL_AMP_VOL)
    myprintf(Serial, "  amplifier volume %d\n", thevol);
  if (currentSoundFile != NULL)
    myprintf(Serial, "  playing %s/%d.mp3\n", currentSoundFile->collection->name, currentSoundFile -> num);
  else if (musicPlayer.playingMusic)
    Serial.println("  Playing unknown sound");
  else {
    myprintf(Serial, "next ambient sound in %d ms\n", nextAmbientSound - now);
    myprintf(Serial, "next baa in %d ms\n", nextBaa - now);
  }

  myprintf(Serial, "  GPS readings %d good, %d bad\n", total_good_GPS, total_bad_GPS);
  if (getGPSFixQuality)
    myprintf(Serial, "  Fix quality = %d, Satellites = %d\n", GPS.fixquality, GPS.satellites);
  myprintf(Serial, "  Location = %d, %d; longest gps void = %dms,  Feet from man = ",
           latitudeDegreesAvg, longitudeDegreesAvg, longestGPSvoid);
  Serial.println(distanceFromMan);
  if (now < 10000)
    longestGPSvoid = 0;


  if (false && prevLat != -1) {
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




unsigned long lastLoop = 0;


const uint16_t REPORT_INTERVAL = 10000;
void loop() {
  Watchdog.reset();
  interrupts();
  SheepInfo & me = getSheep();
  me.time = now();
  if (totalYield > 20) {
    myprintf(Serial, "total yield %dms\n", totalYield);

  }
  totalYield = 0;
  me.uptimeMinutes = minutesUptime();
  me.batteryVoltageRaw =  batteryVoltageRaw();
  me.errorCodes = 0;
  unsigned long now = millis();
  unsigned long nowMicros = micros();
  if ((now / 500) % 2 == 1)
    digitalWrite(LED_BUILTIN, HIGH);
  else
    digitalWrite(LED_BUILTIN, LOW);

  runScheduledActivities();

  yield(10);
}

void checkForNextAmbientSound() {
  unsigned long now = millis();
  if (!useSound || !playSound)
    return;

  if (musicPlayer.playingMusic || soundPlayedRecently(now)) {
    return;
  }
  if (now > nextAmbientSound) {
    if (TRACE) Serial.println("play ambient sound");
    if (!isInFriendlyTerritory() && random(2) == 0 && seperatedSounds.playSound(now, true))  {
      unsigned long delay = 20000;
      if (false && currentSoundFile -> duration > 0 && currentSoundFile -> duration < delay)
        delay = currentSoundFile -> duration;
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
    if (TRACE) Serial.println("play baa sound");
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
void checkForCommand() {
  unsigned long now = millis();
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
          changeAmpVol(thevol + 1);
          break;
        case '-':
          changeAmpVol(thevol - 1);
          break;
      }
    }
  }
}
