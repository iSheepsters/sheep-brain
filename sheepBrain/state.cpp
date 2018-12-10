#include "util.h"
#include "touchSensors.h"
#include "sound.h"
#include "soundFile.h"
#include "scheduler.h"
#include "state.h"
#include "printf.h"
#include "tysons.h"

unsigned long nextAmbientSound = 10000;
unsigned long nextBaa = 30000;

unsigned long timeEnteredCurrentState = 0;
unsigned long lastReadyToRide = 0;

unsigned long notInTheMoodUntil = 0;

BoredState boredState;
AttentiveState attentiveState;
ReadyToRideState readyToRideState;
RidingState ridingState;
NotInTheMoodState notInTheMoodState;
ViolatedState violatedState;

SheepState * currentSheepState = &boredState;


unsigned long privateTouchDisabledUntil = 0;
int privateTouchLoad = 0;
unsigned long privateTouchFade = 0;

void setupState() {
  addScheduledActivity(100, updateState, "state");
}

boolean privateSensorEnabled() {
  if (MALL_SHEEP) return false;
  return privatesEnabled && millis() > privateTouchDisabledUntil;
}

int32_t untouchDurationPrivates() {
  if (privateSensorEnabled())
    return untouchDuration(PRIVATES_SENSOR);
  return 5 * 60 * 1000;
}


uint16_t secondsSinceEnteredCurrentState() {
  return (millis() - timeEnteredCurrentState) / 1000;
}
boolean maybeRiding() {
  if (MALL_SHEEP)
    return false;
  int extra = 0;
  if (touchDuration(LEFT_SENSOR) > 1550)
    extra ++;
  if (touchDuration(RIGHT_SENSOR) > 1550)
    extra ++;
  if (touchDuration(RUMP_SENSOR) > 1550)
    extra ++;
  return touchDuration(BACK_SENSOR) > 8200 && extra >= 2
         ||  touchDuration(BACK_SENSOR) > 6200 && extra == 3;

}

boolean definitivelyRiding() {
  if (MALL_SHEEP)
    return false;
  int extra = 0;
  if (touchDuration(LEFT_SENSOR) > 3550)
    extra ++;
  if (touchDuration(RIGHT_SENSOR) > 3550)
    extra ++;
  if (touchDuration(RUMP_SENSOR) > 3550)
    extra ++;
  return touchDuration(BACK_SENSOR) > 15200 && extra >= 2
         || touchDuration(BACK_SENSOR) > 11000 && extra == 3;

}


boolean isTouched() {
  if (MALL_SHEEP) {
    return  touchDuration(WHOLE_BODY_SENSOR) > 400;
  }
  boolean result =  touchDuration(BACK_SENSOR) > 400
                    ||  touchDuration(RUMP_SENSOR) > 400  ||  touchDuration(HEAD_SENSOR) > 400;
  if (false && result) {
    myprintf(Serial, "touched: %4d %4d %4d\n", touchDuration(BACK_SENSOR) , touchDuration(RUMP_SENSOR) , touchDuration(HEAD_SENSOR));

    myprintf(Serial, "recent: %4d %4d %4d\n", recentTouchDuration(BACK_SENSOR) ,
             recentTouchDuration(RUMP_SENSOR) , recentTouchDuration(HEAD_SENSOR));
    myprintf(Serial, "isTouchedThisInterval: %x\n", touchedThisInterval);

  }
  return result;
}

boolean qualityTouch() {
  if (MALL_SHEEP) {
    return  qualityTime(WHOLE_BODY_SENSOR) > 15000;
  }
  return  qualityTime(BACK_SENSOR) > 15000
          ||  qualityTime(HEAD_SENSOR) > 12000
          ||  qualityTime(BACK_SENSOR) + qualityTime(RUMP_SENSOR) / 2
          +  2 * qualityTime(HEAD_SENSOR) > 30000;
}

boolean isIgnored() {
  if (qualityTouch() || isTouched())
    return false;
  if (MALL_SHEEP)
    return untouchDuration(WHOLE_BODY_SENSOR) > 20000;

  return untouchDuration(BACK_SENSOR) > 15000
         && untouchDuration(HEAD_SENSOR) > 15000
         && untouchDuration(RUMP_SENSOR) > 10000
         && untouchDuration(LEFT_SENSOR) > 10000
         && untouchDuration(RIGHT_SENSOR) > 10000
         && untouchDurationPrivates() > 10000;
}

int privateTouches = 0;
unsigned long lastPrivateTouch = 0;
void becomeViolated() {
  notInTheMoodUntil = max(notInTheMoodUntil, millis() + 1000 * (30 + random(30, 60) + random(20, 100)));
}

boolean wouldInterrupt() {
  unsigned long now = millis();

  if (now - lastSoundStarted < 30000)
    return false; // last started more than 30 seconds ago.

  if (musicPlayer.playingMusic)
    return true;

  if (now - lastSoundPlaying < 5000)
    return true;  // been silent for less than 5 seconds

  return false;
}

unsigned long next_sheep_switch = minutesPerSheep * 60 * 1000L;

void updateState() {

  unsigned long ms = millis();

  if (privateTouchFade < ms) {
    if (privateTouchLoad > 0) {
      privateTouchLoad--;
      myprintf(Serial, "reducing private touch load to %d\n", privateTouchLoad);

    }
    privateTouchFade = ms + 2 * 60 * 1000;
  }

  if (privateSensorEnabled() && touchDuration(PRIVATES_SENSOR) > 1000 && ms > lastPrivateTouch + 3000
      && !(currentSoundPriority == 4 && currentSoundFile != NULL)) {
    privateTouchLoad++;
    myprintf(Serial, "new private touch, current load = %d\n", privateTouchLoad);
    if (privateTouchLoad > 6) {
      privateTouchDisabledUntil = ms + 10 * 60 * 1000;
      Serial.println("private touch disabled");
    } else {
      lastPrivateTouch = ms;
      privateTouches++;

      myprintf(Serial, "inappropriate touch duration %d, num inappropriate touches = %d\n",
               touchDuration(PRIVATES_SENSOR), privateTouches);
      myprintf(Serial, "current value = %d, stable value = %d\n",
               cap.filteredData((uint8_t) PRIVATES_SENSOR), stableValue[PRIVATES_SENSOR]);
      inappropriateTouchSounds.playSound(ms, false);

      if (privateTouches >= 2 && currentSheepState != &violatedState) {
        becomeViolated();
        currentSheepState = &violatedState;
        timeEnteredCurrentState = ms;
        return;
      }
    }
  }

  if (secondsSinceEnteredCurrentState() < 2)
    return;

  SheepState * newState = currentSheepState->update();
  if (newState != currentSheepState) {
    // got an update

    myprintf(Serial, "State changed from %s to %s\n", currentSheepState->name, newState->name);
    if (MALL_SHEEP) {
      myprintf(Serial, "   %d secs, %6d\n",
               secondsSinceEnteredCurrentState(),
               combinedTouchDuration(WHOLE_BODY_SENSOR));
      if (isIgnored()) Serial.println("is ignored");
      if (qualityTouch()) Serial.println("quality touched");
      else if (isTouched()) Serial.println("is touched");

    } else {
      myprintf(Serial, "   %d secs, %6d %6d %6d %6d %6d\n",
               secondsSinceEnteredCurrentState(),
               combinedTouchDuration(HEAD_SENSOR),
               combinedTouchDuration(BACK_SENSOR),
               combinedTouchDuration(LEFT_SENSOR),
               combinedTouchDuration(RIGHT_SENSOR),
               combinedTouchDuration(RUMP_SENSOR));
      if (maybeRiding()) Serial.println("maybe riding");
      if (definitivelyRiding()) Serial.println("definitely riding");
      if (isIgnored()) Serial.println("is ignored");
      if (qualityTouch()) Serial.println("quality touched");
      else if (isTouched()) Serial.println("is touched");
      if (privateTouches > 0)
        myprintf(Serial, "%d private touches\n", privateTouches);
    }


    if (!musicPlayer.playingMusic ||
        newState->getInitialSounds().priority >= currentSoundPriority) {
      newState->getInitialSounds().playSound(ms, false);
      if (newState == &violatedState)
        nextAmbientSound = ms + 12000;
      else
        nextAmbientSound = ms + 12000 + random(1, 15000);
    }
    currentSheepState = newState;
    timeEnteredCurrentState = ms;
  } else if (minutesPerSheep > 0 && next_sheep_switch < ms && !musicPlayer.playingMusic && !wouldInterrupt()) {
    sheepNumber = sheepToSwitchTo(sheepNumber);
    next_sheep_switch = ms + minutesPerSheep * 60 * 1000;

    loadPerSheepSounds();
    myprintf(Serial, "switching to sheep %d\n", sheepNumber);
    myprintf(logFile, "Switching to sheep %d\n", sheepNumber);
    writeSheepNumber(sheepNumber);
    myprintf(Serial, "Wrote %d to config.txt\n", sheepNumber);

    changeSounds.playSound(ms, false);

  }
  //Serial.println("update state finished");
}

boolean SheepState::playAmbientSound(unsigned long now) {
  if (random(2) == 0) {
    if (ambientSounds.playSound(now, true))
      return true;
    return generalSounds.playSound(now, true);
  }
  if (generalSounds.playSound(now, true))
    return true;
  return ambientSounds.playSound(now, true);
}

SheepState * BoredState::update() {
  if (secondsSinceEnteredCurrentState() > 20)
    privateTouches = 0;
  if (privateTouches == 0 && maybeRiding() && millis() - lastReadyToRide < 8000)
    return &ridingState;
  if (isTouched()) {
    if (random(100) < 20 && secondsSinceEnteredCurrentState() > 30) {
      int notInTheMood = (random(30, 60) + random(20, 100));
      myprintf(Serial, "not in the mood for %d seconds\n", notInTheMood);
      notInTheMoodUntil = millis() + 1000 * notInTheMood;
    }
    return &attentiveState;
  }
  return this;
}


SheepState * AttentiveState::update() {

  if (isIgnored())
    return &boredState;
  if (privateTouches == 0 && maybeRiding() && millis() - lastReadyToRide < 8000)
    return &ridingState;

  if (definitivelyRiding()) {
    becomeViolated();
    return &violatedState;
  }
  if (MALL_SHEEP)
    return this;
  if (secondsSinceEnteredCurrentState() > 40 && !wouldInterrupt()
      && millis() < notInTheMoodUntil)
    return &notInTheMoodState;
  if (qualityTouch() && secondsSinceEnteredCurrentState() > 12 && !wouldInterrupt()) {
    if (millis() < notInTheMoodUntil)
      return &notInTheMoodState;
    return &readyToRideState;
  }
  return this;
}


SheepState * ReadyToRideState::update() {
  lastReadyToRide = millis();
  if (wouldInterrupt())
    return this;
  if (maybeRiding() && secondsSinceEnteredCurrentState() > 10 )
    return &ridingState;

  if (!qualityTouch() && secondsSinceEnteredCurrentState() > 20) {
    if (isTouched())
      return &attentiveState;
    return &boredState;
  }

  return this;
}

SheepState * RidingState::update() {
  lastReadyToRide = millis();
  if (!maybeRiding() && secondsSinceEnteredCurrentState() > 20 && !wouldInterrupt()) {

    endOfRideSounds.playSound(millis(), false);

    if (qualityTouch())
      return &readyToRideState;

    if (isTouched())
      return &attentiveState;

    return &boredState;
  }
  return this;
}


SheepState * NotInTheMoodState::update() {
  if (definitivelyRiding()) {
    becomeViolated();
    return &violatedState;
  }
  if (wouldInterrupt())
    return this;

  if (millis() > notInTheMoodUntil && secondsSinceEnteredCurrentState() > 30) {
    if (qualityTouch()) {
      if (notInTheMoodUntil > 20000)
        notInTheMoodUntil -= 20000;
      return &attentiveState;
    }
    if (isTouched())
      return &attentiveState;
    return &boredState;
  }
  return this;
}

const boolean debug_violation = true;
unsigned long nextViolationDebug = 0;

SheepState * ViolatedState::update() {
  boolean debug = false;
  if (debug_violation && nextViolationDebug < millis()) {
    debug = true;
    nextViolationDebug = nextViolationDebug + 2000;
  }
  int t = untouchDurationPrivates();
  if (t < 10000) {
    if (debug)
      myprintf(Serial, "untouch duration %d, remain violated\n", t);
    return this;
  }


  if (definitivelyRiding()) {
    if (debug)
      Serial.println("definitely riding, remain violated");
    return this;
  }
  if (wouldInterrupt()) {
    if (debug)
      Serial.println("would interrupt, remain violated");
    return this;
  }
  if (secondsSinceEnteredCurrentState() < 20) {
    if (debug)
      myprintf(Serial, "only %d seconds since violated, remain violated\n", secondsSinceEnteredCurrentState()  );
    return this;
  }
  if (isTouched())
    return &attentiveState;
  else return &boredState;
}
