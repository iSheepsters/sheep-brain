/*********************************************************
This is a library for the MPR121 12-channel Capacitive touch sensor

Designed specifically to work with the MPR121 Breakout in the Adafruit shop 
  ----> https://www.adafruit.com/products/

These sensors use I2C communicate, at least 2 pins are required 
to interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.  
BSD license, all text above must be included in any redistribution
**********************************************************/

#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "Adafruit_SSD1306.h"

#define OLED_RESET 0  // GPIO0 aka D3
Adafruit_SSD1306 display(OLED_RESET);

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("Booted up!");


  
  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
//    u8x8.drawString(0, 2, "No touch sensors");
    delay(100);
//    display.drawString(0,1,"Touch Failed");
//    display.display();
    while (1);
  }
//  u8x8.drawString(0, 2, "Found touch sensors!");
  Serial.println("MPR121 found!");
//  display.drawString(0,1,"Touch enabled");
//  display.display();


  delay(1000);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
//  display.invertDisplay(1);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("B");
  display.display();
  delay(1000);
  for (int i=0; i<4; i++) {
    display.print("a");
    delay(1000);
    display.display();
  }
  display.clearDisplay();
  display.display();
}

int sensorTable[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

void displayState() {
  display.clearDisplay();
  display.setCursor(0,0);
  for ( int i = 0; i < 12; i++ ) {
    if ( sensorTable[i] ) {
      display.print(i);
      display.print(" ");
    }
  }
  display.display();
}

void loop() {

  
  // Get the currently touched pads
  currtouched = cap.touched();
  bool stateChanged = false;
  
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      Serial.print(i); Serial.println(" touched");
      sensorTable[i] = 1;
      stateChanged = true;
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      Serial.print(i); Serial.println(" released");
      sensorTable[i] = 0;
      stateChanged = true;
    }
  }

  // reset our state
  lasttouched = currtouched;

  if ( stateChanged ) {
    displayState();
  }


  // comment out this line for detailed data from the sensor!
  return;
  
  // debugging info, what
  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t\t 0x"); Serial.println(cap.touched(), HEX);
  Serial.print("Filt: ");
  for (uint8_t i=0; i<12; i++) {
    Serial.print(cap.filteredData(i)); Serial.print("\t");
  }
  Serial.println();
  Serial.print("Base: ");
  for (uint8_t i=0; i<12; i++) {
    Serial.print(cap.baselineData(i)); Serial.print("\t");
  }
  Serial.println();
  
  // put a delay so it isn't overwhelming
  delay(100);
}
