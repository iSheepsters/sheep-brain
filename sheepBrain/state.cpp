#include "util.h"
#include "touchSensors.h"
#include "sound.h"
#include "soundFile.h"
#include "state.h"
#include "printf.h"


unsigned long nextRandomSound = 10000;
unsigned long timeEnteredCurrentState = 0;

unsigned long notInTheMoodUntil = 0;

BoredState boredState;
AttentiveState attentiveState;
ReadyToRideState readyToRideState;
RidingState ridingState;
NotInTheMoodState notInTheMoodState;
ViolatedState violatedState;

SheepState * currentSheepState = &boredState;


uint16_t secondsSinceEnteredCurrentState() {
  return (millis() - timeEnteredCurrentState) / 1000;
}
boolean areRiding() {

  return touchDuration(BACK_SENSOR) > 4200
         && (touchDuration(LEFT_SENSOR) > 550 || touchDuration(RIGHT_SENSOR) > 550 ||  touchDuration(RUMP_SENSOR) > 550 )
         || touchDuration(RUMP_SENSOR) > 9500;
}

boolean isIgnored() {
  return untouchDuration(BACK_SENSOR) > 10000
         && untouchDuration(HEAD_SENSOR) > 10000
         && untouchDuration(RUMP_SENSOR) > 10000;
}

boolean isTouched() {
  return  touchDuration(BACK_SENSOR) > 100
          ||  touchDuration(RUMP_SENSOR) > 100  ||  touchDuration(HEAD_SENSOR) > 100;
}

boolean qualityTouch() {
  return  touchDuration(BACK_SENSOR) > 7000
          ||  touchDuration(RUMP_SENSOR) > 7000  ||  touchDuration(HEAD_SENSOR) > 7000
          ||  touchDuration(BACK_SENSOR) + touchDuration(RUMP_SENSOR) +  touchDuration(HEAD_SENSOR) > 4000;
}

int privateTouches = 0;
unsigned long lastPrivateTouch = 0;

void becomeViolated() {
  notInTheMoodUntil = max(notInTheMoodUntil, millis() + 1000 * (30 + random(30, 60) + random(20, 100)));
}

void updateState(unsigned long now) {
  if (touchDuration(PRIVATES_SENSOR) > 2000 && now > lastPrivateTouch + 4000
      && !(currentSoundPriority == 4 && currentSoundFile != NULL)) {
    lastPrivateTouch = now;
    inappropriateTouchSounds.playSound(now, false);
    privateTouches++;
    if (privateTouches >= 2 && currentSheepState != &violatedState) {
      becomeViolated();
      currentSheepState = &violatedState;
      timeEnteredCurrentState = now;
      return;
    }
  }

  if (secondsSinceEnteredCurrentState() < 2)
    return;

  SheepState * newState = currentSheepState->update();
  if (newState != currentSheepState) {
    // got an update

    myprintf(Serial, "State changed from %s to %s\n", currentSheepState->name, newState->name);
    myprintf(Serial, "   %d secs, %6d %6d %6d %6d %6d\n",
             secondsSinceEnteredCurrentState(),
             combinedTouchDuration(HEAD_SENSOR),
             combinedTouchDuration(BACK_SENSOR),
             combinedTouchDuration(LEFT_SENSOR),
             combinedTouchDuration(RIGHT_SENSOR),
             combinedTouchDuration(RUMP_SENSOR));
    if (!musicPlayer.playingMusic ||
        newState->sounds.priority >= currentSoundPriority) {
      newState->sounds.playSound(now, false);
      nextRandomSound = now + 12000 + random(1, 15000);
    }
    currentSheepState = newState;
    timeEnteredCurrentState = now;
  }
  //Serial.println("update state finished");
}


boolean SheepState::playSound(unsigned long now, boolean ambientNoise) {
  if (sounds.playSound(now, ambientNoise)) {
    return true;
  }
  return baaSounds.playSound(now, ambientNoise);
}

SheepState * BoredState::update() {
  if (secondsSinceEnteredCurrentState() > 10)
    privateTouches = 0;
  if (isTouched()) {
    if (random(100) < 20 || true)
      notInTheMoodUntil = millis() + 1000 * (random(30, 60) + random(20, 100));

    return &attentiveState;
  }
  return this;
}


SheepState * AttentiveState::update() {
  if (isIgnored())
    return &boredState;
  if (areRiding()) {
    becomeViolated();
    return &violatedState;
  }
  if (qualityTouch()) {
    if (millis() < notInTheMoodUntil)
      return &notInTheMoodState;
    return &readyToRideState;
  }
  return this;
}

SheepState * ReadyToRideState::update() {
  if (areRiding())
    return &ridingState;
  if (!qualityTouch() && secondsSinceEnteredCurrentState() > 10) {
    if (isTouched())
      return &attentiveState;
    return &boredState;
  }

  return this;
}

SheepState * RidingState::update() {
  if (!areRiding()) {
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
  if (areRiding()) {
    becomeViolated();
    return &violatedState;
  }
  if (secondsSinceEnteredCurrentState() > 30) {
    if (qualityTouch()) {
      if (notInTheMoodUntil > 10000)
        notInTheMoodUntil -= 10000;
    }
    if (isTouched())
      return &attentiveState;
    return &boredState;
  }
  return this;
}


SheepState * ViolatedState::update() {
  if (secondsSinceEnteredCurrentState() > 60 && !isTouched())  {
    return &boredState;
  }
  return this;
}


