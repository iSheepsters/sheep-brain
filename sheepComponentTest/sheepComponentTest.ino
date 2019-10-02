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
  delay(500);
  log(setVolume(31), 1, "MAX9744");
  log(cap.begin(0x5A), 2, "MPR121");
  log(musicPlayer.begin(), 3, "VS1053");
  if (log(SD.begin(5), 4, "SD Card")) {
    printDirectory(SD.open(" / "), 0);
  }
}


void loop() {
  // put your main code here, to run repeatedly:

}
