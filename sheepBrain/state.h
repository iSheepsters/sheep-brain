#ifndef _STATE_H
#define _STATE_H
#include "soundFile.h"


enum State {
  Bored,
  Attentive,
  Riding,
  ReadyToRide,
  NotInTheMood,
  Violated,
};

extern unsigned long timeEnteredCurrentState;

extern void updateState(unsigned long now);
extern boolean privateSensorEnabled();
class SheepState {
  public:
    virtual SheepState * update() = 0;
    const char * const name;
    const enum State state;
    SoundCollection & sounds;
    SheepState(enum State st, const char * n, SoundCollection & s) : state(st), name(n), sounds(s) {};
    boolean playSound(unsigned long now, boolean ambientNoise);

};

extern SheepState * currentSheepState;


class BoredState : public SheepState {
 public:
    BoredState() : SheepState(Bored, "Bored", boredSounds) {};
    SheepState * update();

};

class AttentiveState : public SheepState {
   public:
    AttentiveState() : SheepState(Attentive, "Attentive", attentiveSounds) {};
    SheepState * update();
};

class ReadyToRideState : public SheepState {
   public:
    ReadyToRideState() : SheepState(ReadyToRide, "Ready to ride", readyToRideSounds) {};
    SheepState * update();
};

class RidingState : public SheepState {
   public:
    RidingState() : SheepState(Riding, "Riding", ridingSounds) {};

    SheepState * update();
};

class NotInTheMoodState : public SheepState {
   public:
    NotInTheMoodState() : SheepState(NotInTheMood, "Not in the mood", notInTheMoodSounds) {};
    SheepState * update();
};


class ViolatedState : public SheepState {
   public:
    ViolatedState() : SheepState(Violated, "Violated", violatedSounds) {};
    SheepState * update();
};

#endif

