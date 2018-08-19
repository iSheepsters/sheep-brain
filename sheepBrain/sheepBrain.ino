#include <Adafruit_SleepyDog.h>


#include <Wire.h>
#include <TimeLib.h>
#include "util.h"
#include "sound.h"
#include "printf.h"
#include "touchSensors.h"
#include "soundFile.h"
#include "slave.h"
#include "GPS.h"
#include "radio.h"
#include "logging.h"
#include "avdweb_SAMDtimer.h"


extern  void checkForCommand(unsigned long now);
const boolean useSound = true;
const boolean playSound = true;
const boolean useOLED = false;
const boolean useTouch = true;
const boolean useGPS = true;
const boolean useSlave = true;
const boolean useRadio = true;

uint8_t sheepNumber = 14;
SheepInfo infoOnSheep[NUMBER_OF_SHEEP];

SheepInfo & getSheep(int s) {
  if (s < 1 || s > NUMBER_OF_SHEEP) {
    myprintf(Serial, "invalid request for sheep %d\n", s);
    return infoOnSheep[0];
  }
  return infoOnSheep[s - 1];
}


//const uint8_t SHDN_MAX9744 = 10;

unsigned long nextPettingReport;

boolean debugTouch = false;
SoundCollection boredSounds(0);
SoundCollection ridingSounds(2);
SoundCollection welcomingSounds(1);
SoundCollection baaSounds(0);


const char * stateName(State s) {
  switch (s) {
    case Bored: return "Bored";
    case Welcoming: return "Welcoming";
    case Riding: return "Riding";
    default: return "Unknown";
  }
}
unsigned long setupFinished = 0;
uint16_t minutesUptime() {
  return ( millis() - setupFinished) / 1000 / 60;
}

unsigned long nextTestDistress = 0xffffffff; // disable test distreess
enum State currState = Bored;

void ISR_timer_1KHz(struct tc_module *const module_inst)
{
  while (Serial1.available())
    GPS.read();
}

SAMDtimer IRS_timer4 = SAMDtimer(4, ISR_timer_1KHz, 1000, 0);

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
      setupDelay(200);                     // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      setupDelay(200);
    }
  int countdownMS = Watchdog.enable(14000);
  myprintf(Serial, "Watchdog set, %d ms timeout\n", countdownMS);

  for (int i = 1; i < 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    setupDelay(20);                     // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    setupDelay(20);
    Serial.println(i);
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
  sheepNumber = configFile.parseInt();
  myprintf(Serial, "I am sheep %d\n", sheepNumber);
  configFile.close();

  setupLogging();
  myprintf(logFile, "Start of log for sheep %d\n", sheepNumber);

  if (useSound) {


    //    digitalWrite(SHDN_MAX9744, HIGH);
    //    setupDelay(10);
    //    pinMode(SHDN_MAX9744, INPUT);
    //    setupDelay(100);
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
  myprintf(Serial, "%dms set up time\n", setupFinished);
  myprintf(logFile, "sheep %d ready\n", sheepNumber);
  
  updateLog(setupFinished);
  read_gps_in_interrupt = true;
  IRS_timer4.enableInterrupt(1);
  nextPettingReport = setupFinished + 2000;

}

unsigned long nextRandomSound = 10000;


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
      if (useSound && playSound)
        ridingSounds.playSound(now, false);
      nextRandomSound = now + 12000 + random(1, 15000);
      currState = Riding;
      Serial.println("riding");
    }
  }
  else if (untouchDuration(BACK_SENSOR) > 10000
           && untouchDuration(HEAD_SENSOR) > 10000
           && untouchDuration(RUMP_SENSOR) > 10000) {
    if (currState != Bored) {
      if (useSound && playSound && !musicPlayer.playingMusic) {
        boredSounds.playSound(now, false);
        nextRandomSound = now + 12000 + random(1, 15000);
      }
      currState = Bored;
      Serial.println("bored");
    }
  }
  else if ( touchDuration(BACK_SENSOR) > 100 ||
            touchDuration(LEFT_SENSOR) > 100 || touchDuration(RIGHT_SENSOR) > 100
            ||  touchDuration(RUMP_SENSOR) > 100  ||  touchDuration(HEAD_SENSOR) > 100) {
    if (currState != Welcoming) {
      if (useSound && playSound && (!musicPlayer.playingMusic || currState == Bored)) {
        welcomingSounds.playSound(now, false);
        nextRandomSound = now + 12000 + random(1, 15000);
      }
      currState = Welcoming;
      Serial.println("welcoming");
    }

  }

}


unsigned long nextReport = 0;
unsigned long phase1 = 0;

unsigned long phase2 = 0;
unsigned long phase3 = 0;
unsigned long phase4 = 0;
const boolean TRACE = false;

unsigned long lastLoop = 0;
void loop() {
  Watchdog.reset();



  SheepInfo & me =  getSheep();
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


  if (nextTestDistress < now) {
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
  if (useRadio)
    updateRadio();
  if (nextReport < now ) {
    myprintf(Serial, "%d State %s, %f volts, %d minutes uptime, %d GPS fixes, %2d:%02d:%02d\n",
             now, stateName(currState),
             batteryVoltage(), minutesUptime(), fixCount,
             hour(), minute(), second());
    myprintf(Serial, "phase %d %d %d %d\n", phase1, phase2, phase3, phase4);
    time_t BRC_time = BRC_now();

    if (false)
      myprintf(Serial, "BRC time: %2d:%02d:%02d\n", hour(BRC_time), minute(BRC_time), second(BRC_time));

    nextReport = now + 5000;
    if (false) {
      if (TRACE)
        Serial.println("Updating log");
      updateLog(now);
      if (TRACE)
        Serial.println("Log updated");
    }
  }
  phase1 = millis() - now;

  if (useTouch) {
    

    if (TRACE)
      Serial.println("updateTouchData");
    unsigned long startTouch = micros();
    updateTouchData(now, false);
    //Serial.println(micros()-startTouch);
    if (debugTouch) {
      Serial.print("TD ");
      for (int i = 0; i < numTouchSensors; i++)
        myprintf(Serial, " %3d %3d ", stableValue[i], sensorValue((TouchSensor)i));
      Serial.println();
    }

    if (useSlave) {
      if (TRACE) Serial.println("sendSlave");
      uint8_t v = sendSlave(0, currTouched);
      if (v != 0) {
        Serial.print("Transmission error: " );
        Serial.println(v);
      }
    }
    //dumpTouchData();

  }
  phase2 = millis() - now - phase1;


  if (now > 10000 && useTouch) {
    if (TRACE) Serial.println("updateState");
    updateState(now);
    if (useSlave)
      sendSlave(1, (uint8_t) currState);


    for (int i = 0; i < numTouchSensors; i++) {
      if (((newTouched >> i) & 1 ) != 0) {
        myprintf(Serial, "touched %d - %d\n", i, sensorValue((enum TouchSensor)i));
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
  }
  phase3 = millis() - now - phase1 - phase2;

  if (useSound && playSound) {
    if (TRACE) Serial.println("checkForSound");
    checkForSound(now);
  }
  if (TRACE) Serial.println("checkForCommand");
  checkForCommand(now);
  phase4 = millis() - now - phase1 - phase2 - phase3;
  //yield(10);
}

void checkForSound(unsigned long now) {
  if (!useSound || !playSound)
    return;
  if (nextRandomSound < now && !musicPlayer.playingMusic && !soundPlayedRecently(now)) {
    if (random(3) == 0) {
      baaSounds.playSound(now, true);
      nextRandomSound = now + random(5000, 14000);
    } else {
      switch (currState) {
        case Bored:
          if (!boredSounds.playSound(now, true))
            baaSounds.playSound(now, true);
          break;
        case Welcoming:
          if (! welcomingSounds.playSound(now, true))
            baaSounds.playSound(now, true);
          break;
        case Riding:
          if (!ridingSounds.playSound(now, true))
            baaSounds.playSound(now, true);
          break;
      }
      nextRandomSound = now + random(12000, 30000);
    }
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

          setVolume(thevol);
          break;
      }
    }
  }
}

