#include "util.h"
#include "touchSensors.h"
#include "sound.h"
#include "soundFile.h"
#include "state.h"
#include "printf.h"


unsigned long nextRandomSound = 10000;
unsigned long timeEnteredCurrentState = 0;

BoredState boredState;
AttentiveState attentiveState;
ReadyToRideState readyToRideState;
RidingState ridingState;
NotInTheMoodState notInTheMoodState;


SheepState * currentSheepState = &boredState;

boolean areRiding() {
  return touchDuration(BACK_SENSOR) > 1200
         && (touchDuration(LEFT_SENSOR) > 550 || touchDuration(RIGHT_SENSOR) > 550 ||  touchDuration(RUMP_SENSOR) > 550 )
         || touchDuration(BACK_SENSOR) > 9500;
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


void updateState(unsigned long now) {
  
  if (false) myprintf(Serial, "update state %d: %6d %6d %6d %6d %6d\n", currentSheepState->state,
                        combinedTouchDuration(HEAD_SENSOR),
                        combinedTouchDuration(BACK_SENSOR),
                        combinedTouchDuration(LEFT_SENSOR),
                        combinedTouchDuration(RIGHT_SENSOR),
                        combinedTouchDuration(RUMP_SENSOR));

  SheepState * newState = currentSheepState->update();
  if (newState != currentSheepState) {
    // got an update
    timeEnteredCurrentState = now;
    myprintf(Serial, "State changed from %s to %s\n", currentSheepState->name, newState->name);
    newState->sounds.playSound(now, false);
    nextRandomSound = now + 12000 + random(1, 15000);
    currentSheepState = newState;
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
  if (isTouched())
    return &attentiveState;
  return this;
}


SheepState * AttentiveState::update() {
  if (isIgnored())
    return &boredState;
  if (areRiding())
    return &ridingState;
  return this;
}

SheepState * ReadyToRideState::update() {
  return this;
}

SheepState * RidingState::update() {
  if (isIgnored())
    return &attentiveState;
  return this;
}


SheepState * NotInTheMoodState::update() {
  return this;
}



