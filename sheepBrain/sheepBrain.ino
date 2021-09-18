#include <Adafruit_SleepyDog.h>
// #include <Adafruit_ASFcore.h>
//#include <MemoryFree.h>
const boolean WAIT_FOR_SERIAL = false;
#include <Adafruit_ZeroTimer.h>

#include <Adafruit_ZeroDMA.h>
#include <Wire.h>
#include <SdFat.h>
#include "util.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include "soundFile.h"
#include "comm.h"
#include "state.h"
#include "time.h"


#include "logging.h"
#include "scheduler.h"




extern  void checkForCommand();

// If the initial sheepNumber is anything other than 15, it will be
// rewritten by what is on the SD card. 15 is a placeholder, there
// shouldn't be a real sheep with that number.
// sheepNumber 14 is for shepherd

const int WATCHDOG_TIMEOUT = 10000;
uint8_t sheepNumber = 15;
SheepInfo infoOnSheep[NUMBER_OF_SHEEP];

SheepInfo & getSheep(int s) {
  if (s < 1 || s > NUMBER_OF_SHEEP) {
    myprintf(Serial, "invalid request for sheep %d\n", s);
    return infoOnSheep[0];
  }
  return infoOnSheep[s - 1];
}


SoundCollection generalSounds(0);
SoundCollection boredSounds(0);
SoundCollection firstTouchSounds(2);
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


unsigned long nextActivityReport = 0;
volatile uint16_t periodicTicks = 0;
// execute this code at 20Hz

void periodicTask(void) {
  periodicTicks++;
  unsigned long start = micros();
  if (false) {
    unsigned long ms = millis();
    if (currentActivityStartedAt + 500 < ms && nextActivityReport < ms) {
      myprintf(Serial, "activity %d.%d, started %dms ago\n",
               currentActivity, currentSubActivity,
               ms - currentActivityStartedAt);
      nextActivityReport = ms + 1000;
    }
  }
  
  if (musicPlayerReady && musicPlayer.playingMusic)
    musicPlayer.feedBuffer();
  unsigned long end = micros();
  long diff = end - start;
  if (maxInterruptTime < diff)
    maxInterruptTime = diff;
}


// timer tester
Adafruit_ZeroTimer zerotimer = Adafruit_ZeroTimer(4);

void setupPeriodTask() {
  Serial.println("setup periodic task");
  int freq = 20;
  uint8_t  divider = 64;
  tc_clock_prescaler prescaler = TC_CLOCK_PRESCALER_DIV64;
  uint16_t   compare = (48000000 / 64) / freq;
  zerotimer.enable(false);
  zerotimer.configure(prescaler,       // prescaler
                      TC_COUNTER_SIZE_16BIT,       // bit width of timer/counter
                      TC_WAVE_GENERATION_MATCH_PWM // frequency or PWM mode
                     );

  zerotimer.setCompare(0, compare);
  zerotimer.setCallback(true, TC_CALLBACK_CC_CHANNEL0, periodicTask);
  zerotimer.enable(true);
}

void testSwapLeftRight(uint8_t touched) {
  myprintf(Serial, "testSwapLeftRight %02x -> %02x\n",
           touched, swapLeftRightSensors(touched));
}

void setup() {
  // disable the radio module until we are ready for it
  pinMode(8, INPUT_PULLUP);
  // disable VS1053 until we are ready for it
  pinMode(6, INPUT_PULLUP);
  // disable SdCard until we are ready for it
  pinMode(5, INPUT_PULLUP);

  delay(300);
  memset(&infoOnSheep, 0, sizeof (infoOnSheep));
  pinMode(LED_BUILTIN, OUTPUT);

  delay(100);
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    Serial.println(i);
    delay(100);
  }
  int countdownMS = Watchdog.enable(WATCHDOG_TIMEOUT);
  myprintf(Serial, "Watchdog set, %d ms timeout\n", countdownMS);

  int clearBus = I2C_ClearBus();
  if (clearBus != 0)
    while (true) {
      Serial.println(F("I2C bus error. Could not clear"));
      if (clearBus == 1) {
        Serial.println(F("SCL clock line held low"));
      } else if (clearBus == 2) {
        Serial.println(F("SCL clock line held low by slave clock stretch"));
      } else if (clearBus == 3) {
        Serial.println(F("SDA data line held low"));
      } else
        Serial.println(F("Unknown return value from I2C_ClearBus"));
      delay(500);
    }

  Wire.begin();

  if (WAIT_FOR_SERIAL) while (!Serial && millis() < 8000) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      setupDelay(200);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      setupDelay(200);
    }


  randomSeed(analogRead(0));
  myprintf(Serial, "\n\n\n\nCompiled %s, %s\n", F(__DATE__), F(__TIME__));

  countdownMS = Watchdog.enable(WATCHDOG_TIMEOUT);
  myprintf(Serial, "Watchdog set, %d ms timeout\n", countdownMS);

  // myprintf(Serial, "Free memory = %d\n", freeMemory());



  if (useSlave) {
    Serial.println("Setting up comm");
    setupComm();
    sendBoot();
  } else
    Serial.println("Skipping comm");

 
  Watchdog.reset();
  if (useSound) {
    Serial.println("Setting up sound");
    setupSound();
    Serial.println("Sound setup");
  } else
    Serial.println("Skipping sound");

    Serial.println("Setting up time");
    setupTime();





  Watchdog.reset();
  if (!setupSD()) {
    Serial.println("setupSD failed");
    Serial.println("huh");
    logDistress("SD card not found");
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      setupDelay(1000);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      setupDelay(1000);
      Serial.print("setupSD failed, time: ");
      Serial.println(millis());
    }
  }
  Watchdog.reset();
  // myprintf(Serial, "Free memory = %d\n", freeMemory());
  int s = 1;
  File configFile = SD.open("config.txt");
  if (!configFile) {

    logDistress("no config.txt file");

  } else {
    Watchdog.reset();
    s = configFile.parseInt();
    myprintf(Serial, "config file contains %d\n", s);

    int v = configFile.parseInt();
    if (v != 0) {
      STABLE_VALUE = v;
      myprintf(Serial, "stable sensor value set to %d\n", v);
    } else {
      myprintf(Serial, "stable sensor value not set\n", v);
    }
    v = configFile.parseInt();
    if (v != 0) {
      privatesEnabled = true;
      Serial.println("Privates enabled");
    } else Serial.println("Privates disabled");
    v = configFile.parseInt();
    if (v != 0) {
      allowRrated = true;
      Serial.println("R rated sounds enabled");
    } else
      Serial.println("R rated sounds disabled");
    v = configFile.parseInt();

    if (v > 0 && v <= 63) {
      thevol = v;
      myprintf(Serial, "default volume set to %d\n", v);
    } else
      myprintf(Serial, "default volume not set, using %d\n", thevol);

    v = configFile.parseInt();
    if (v != 0) {
      myprintf(Serial, "min time to next sound: %d secs\n", v);

      msToNextSoundMin = v * 1000;
    }
    v = configFile.parseInt();
    if (v != 0) {
      myprintf(Serial, "max time to next sound: %d secs\n", v);
      msToNextSoundMax = v * 1000;
    }

    configFile.close();
  }
  Watchdog.reset();
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

  setupLogging();
  myprintf(logFile, "Start of log for sheep %d\n", sheepNumber);
  Watchdog.reset();
  if (useSound) {

   // myprintf(Serial, "Free memory = %d\n", freeMemory());
    Serial.println("Sound set up, examining sound files");

    loadPerSheepSounds();

    if (minutesPerSheep > 0)
      changeSounds.loadCommon("change");
    baaSounds.loadCommon("baa");
    baaSounds.playSound(millis(), false);
   // myprintf(Serial, "Free memory = %d\n", freeMemory());
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
  logTouchConfiguration();
  updateLog(setupFinished);
  
  


  if (useSlave) {
    Serial.println("Getting last boot from teensy");
    logBoot();
  }

  Serial.println();
  sendActivity(101);
  setupFinished = millis();
  myprintf(Serial, " %dms set up time\n", setupFinished);
  myprintf(logFile, "sheep %d ready, ", sheepNumber);
  myprintf(logFile, "Compiled %s, %s\n", F(__DATE__), F(__TIME__));
  listScheduledActivities();

  // myprintf(Serial, "  Free memory = %d\n", freeMemory());
  myprintf(Serial, "setup complete, sheep %d\n",
           sheepNumber);
  myprintf(Serial, "Compiled %s, %s\n", F(__DATE__), F(__TIME__));
  randomSeed(analogRead(0));
  startSchedule();

}


const boolean TRACE = false;

unsigned long lastReport = 0;

void generateReport() {
  if (plotTouch) return;
  unsigned long now = millis();
  myprintf(Serial, "%d/%d State %s, %f volts,  %d minutes uptime, %2d:%02d:%02d\n",
           sheepNumber, now, currentSheepState->name,
           batteryVoltage(),  minutesUptime(), 
           hour(), minute(), second());
  myprintf(Serial, "  untouch duration %d\n", untouchDuration());
  if (!isActive()) {
     myprintf(Serial, "not active");
  }

  myprintf(Serial, "  local time %2d:%02d:%02d\n",

           adjustedHour(), minute(), second() );
  myprintf(Serial, "  %dms interrupt interval, %dus max interrupt time, %d max avail\n",
           longestInterval, maxInterruptTime, maxAvail);
  // myprintf(Serial, "  Free memory = %d\n", freeMemory());
  myprintf(Serial, "  periodic ticks: %d\n", periodicTicks);

  if (thevol != INITIAL_AMP_VOL)
    myprintf(Serial, "  amplifier volume %d\n", thevol);
  if (currentSoundFile != NULL)
    myprintf(Serial, "  playing %s/%s\n", currentSoundFile->collection->name,
             currentSoundFile -> name);
  else if (musicPlayer.playingMusic)
    Serial.println("  Playing unknown sound");
  else {
    myprintf(Serial, "next ambient sound in %d ms\n", nextAmbientSound - now);
    myprintf(Serial, "next baa in %d ms\n", nextBaa - now);
  }

  

 

  longestInterval = 0;
  maxAvail = 0;
  maxInterruptTime = 0;
 
  lastReport = now;
  sendSubActivity(10);
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

void considerReboot() {
  
}
void loop() {
   periodicTask();
  considerReboot();
  Watchdog.reset();
  interrupts();
  totalYield = 0;
  SheepInfo & me = getSheep();
  me.time = now();
  me.uptimeMinutes = minutesUptime();
  me.batteryVoltageRaw = batteryVoltageRaw();
  me.errorCodes = 0;
  unsigned long now = millis();
  unsigned long nowMicros = micros();

  if ((now / 500) % 2 == 1)
    digitalWrite(LED_BUILTIN, HIGH);
  else
    digitalWrite(LED_BUILTIN, LOW);


  


  if (totalYield > 20 && printInfo()) {
    myprintf(Serial, "total yield %dms\n", totalYield);
  }

  runScheduledActivities();

  yield(10);
}

const int DELAY_BETWEEN_SOUNDS = 5000;
void checkForNextAmbientSound() {
  unsigned long now = millis();
  if (!useSound || !playSound)
    return;

  if (musicPlayer.playingMusic || soundPlayedRecently(now)) {
    return;
  }
  if (now > nextAmbientSound) {
    if (TRACE) Serial.println("play ambient sound");
   
    if (currentSheepState -> playAmbientSound(now)) {
      unsigned long delay = DELAY_BETWEEN_SOUNDS;
      if (currentSoundFile -> duration > 0 && currentSoundFile -> duration / 2 < delay)
        delay = currentSoundFile -> duration / 2 ;
      delay = (delay + random(msToNextSoundMin, msToNextSoundMax)) ;
      if (printInfo())
        myprintf(Serial, "Started playing ambient sound, next in %dms\n", delay);
      nextAmbientSound = now + delay;
      return;
    } else  if (baaSounds.playSound(now, true)) {

      if (printInfo()) myprintf(Serial, "No ambient sound available, baa-ing\n");

      unsigned long delay = random(msToNextSoundMin, msToNextSoundMax) ;
      nextAmbientSound = now + delay;
      nextBaa = now + delay + DELAY_BETWEEN_SOUNDS / 2;
    } else {
      myprintf(Serial, "No ambient or baa sound available\n");
      nextAmbientSound = now + DELAY_BETWEEN_SOUNDS / 4;
    }
  }
  if (now > nextBaa && nextBaa + 4000 < nextAmbientSound) {
    if (TRACE) Serial.println("play baa sound");
    if (baaSounds.playSound(now, true)) {
      unsigned long delay = random(msToNextSoundMin, msToNextSoundMax) ;
      if (printInfo()) myprintf(Serial, "Started playing baa sound, next in %dms\n", delay);
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
      if (printInfo()) {
      Serial.println();
      Serial.print("Got " );
      Serial.println(c);
      }
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
          dumpConfiguration();
          dumpTouchData();
          debugTouch = !debugTouch;
          break;
          
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         if (plotTouch) {
          sensorToPlot = c - '0';
         }
         break;

        case 'p':
          dumpConfiguration();
          dumpTouchData();
          plotTouch = !plotTouch;
          if (plotTouch) {
            sensorToPlot = numTouchSensors;
          }
          break;
      }
    }
  }
}
