const int MAX9744_I2CADDR = 0x4B;
#include "Adafruit_MPR121.h"
#include <Adafruit_VS1053.h>
#include <SPI.h>
#include <SdFat.h>

SdFat SD;

// These are the pins used
#define VS1053_RESET   -1     // VS1053 reset pin (not used!)


#define VS1053_CS       6     // VS1053 chip select pin (output)
#define VS1053_DCS     10     // VS1053 Data/command select pin (output)
#define CARDCS          5     // Card chip select pin
// DREQ should be an Int pin *if possible* (not possible on 32u4)
#define VS1053_DREQ     9     // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

const int led = 13;

Adafruit_MPR121 cap;


/**
   I2C_ClearBus
   (http://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html)
   (c)2014 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code may be freely used for both private and commerical use
*/

/**
   This routine turns off the I2C bus and clears it
   on return SCA and SCL pins are tri-state inputs.
   You need to call Wire.begin() after this to re-enable I2C
   This routine does NOT use the Wire library at all.

   returns 0 if bus cleared
           1 if SCL held low.
           2 if SDA held low by slave clock stretch for > 2sec
           3 if SDA held low after 20 clocks.
*/
int I2C_ClearBus() {
#if defined(TWCR) && defined(TWEN)
  TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif

  pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
  pinMode(SCL, INPUT_PULLUP);

  delay(500);  // Wait 2.5 secs. This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.

  boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
  if (SCL_LOW) { //If it is held low Arduno cannot become the I2C master.
    return 1; //I2C bus error. Could not clear SCL clock line held low
  }

  boolean SDA_LOW = (digitalRead(SDA) == LOW);  // vi. Check SDA input.
  int clockCount = 20; // > 2x9 clock

  while (SDA_LOW && (clockCount > 0)) { //  vii. If SDA is Low,
    clockCount--;
    // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
    pinMode(SCL, INPUT); // release SCL pullup so that when made output it will be LOW
    pinMode(SCL, OUTPUT); // then clock SCL Low
    delayMicroseconds(10); //  for >5uS
    pinMode(SCL, INPUT); // release SCL LOW
    pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5uS
    // The >5uS is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0)) {  //  loop waiting for SCL to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(SCL) == LOW);
    }
    if (SCL_LOW) { // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
  }
  if (SDA_LOW) { // still low
    return 3; // I2C bus error. Could not clear. SDA data line held low
  }

  // else pull SDA line low for Start or Repeated Start
  pinMode(SDA, INPUT); // remove pullup.
  pinMode(SDA, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10); // wait >5uS
  pinMode(SDA, INPUT); // remove output low
  pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
  delayMicroseconds(10); // x. wait >5uS
  pinMode(SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
  pinMode(SCL, INPUT);
  return 0; // all ok
}

void flash(int f, int on = 500, int off = 300) {
  for (int i = 0; i < f; i++) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(on);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(off);
  }

}
bool log(bool found, int flashes, const char * component) {
  if (Serial) {
    if (found) Serial.print("Found ");
    else Serial.print("Did not find ");
    Serial.println(component);
  } else {
    delay(1000);
    if (found) {
      flash(1, 1000, 300);
      flash(flashes, 400, 200);
    } else {
      flash(1, 500, 0);
      flash(5, 100, 100);
      delay(500);
      flash(flashes, 400, 200);
    }
  }
}

void printDirectory(File dir, int numTabs) {
  while (true) {
    char buf[40];
    File entry = dir.openNextFile();
    if (! entry) {
      // no more files
      //Serial.println("**nomorefiles**");
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    entry.getName(buf, 40);
    Serial.print(buf);
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}


// Setting the volume is very simple! Just write the 6-bit
// volume to the i2c bus. That's it!
boolean setVolume(int8_t v) {
  // cant be higher than 63 or lower than 0
  if (v > 63) v = 63;
  if (v < 0) v = 0;

  Serial.print("Setting volume to ");
  Serial.println(v);
  Wire.beginTransmission(MAX9744_I2CADDR);
  Wire.write(v);
  if (Wire.endTransmission() == 0)
    return true;
  else
    return false;
}


void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(100);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
  }
  Serial.println("Sheep component test");
  if (false) {
    int clearBus = I2C_ClearBus();
    while (clearBus != 0) {
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
      clearBus = I2C_ClearBus();
    }
  }
  
  Wire.begin();

  delay(500);
 // log(cap.begin(0x5A), 2, "MPR121");
  log(musicPlayer.begin(), 3, "VS1053");
  if (log(SD.begin(5), 4, "SD Card")) {
    printDirectory(SD.open(" / "), 0);
  }
  log(setVolume(31), 1, "MAX9744");

}


void loop() {
  Serial.println(millis());
  delay(1000);
  // put your main code here, to run repeatedly:

}
