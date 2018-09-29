

enum AnimationKind { OPC_KIND, GPS_KIND, CUSTOM_KIND, BASIC_KIND };



class Animation {
  public:
    int index;
    enum AnimationKind kind;
    virtual void initialize(int index) {};
    virtual void update(unsigned long now) {};
    virtual void close() {};
    virtual boolean isOK() {
      return true;
    }
    virtual void printName() { Serial.println("abstract animation"); }
};


extern void nextAnimation();

extern boolean scheduleSetUp;

extern Animation * currentAnimation;
extern void setupAnimations();
extern unsigned long updateAnimation(unsigned long now);
extern int animationEPOC;
extern long millisToNextEpoc() ;
