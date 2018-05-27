/*********************************************************
  This code plots the Capacitive touch data for all 12 wires.

  Load this code, then use the Serial Plotter to view the data.

**********************************************************/

#include <Wire.h>
#include "Adafruit_MPR121.h"

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

void setup() {
  delay(500);

  Serial.begin(115200);
  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");

  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");
}


void loop() {

  if (false) {
    Serial.print(cap.filteredData(12));
    Serial.print(" ");
    Serial.print(cap.baselineData(12));
  } else
    for (uint8_t i = 0; i < 13; i++) {
      Serial.print(cap.filteredData(i)); Serial.print(" ");
    }
  Serial.println();
  // put a delay so it isn't overwhelming
  delay(100);
}
