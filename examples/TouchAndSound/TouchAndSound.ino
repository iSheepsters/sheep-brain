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
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"


/**
 * OLED Module (Wemos D1 Mini tested only)
 */

//#define USE_DISPLAY

#ifdef USE_DISPLAY
#define OLED_RESET 0  // GPIO0 aka D3
Adafruit_SSD1306 display(OLED_RESET);
#endif

/**
 *  Touch Sensor Module
 */
 
// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

/**
 * MP3 Module
 */
SoftwareSerial MP3Serial(13, 15); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);


/**
 * Main Routine
 */
void setup() {
  
  delay(5000);
  MP3Serial.begin(9600);


  Serial.begin(115200);
  Serial.println();
  Serial.println("Booted up!");


  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(MP3Serial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true);
  }
  Serial.println(F("DFPlayer Mini online."));

  
  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
//    u8x8.drawString(0, 2, "No touch sensors");
    delay(1000);
//    display.drawString(0,1,"Touch Failed");
//    display.display();
    while (1);
  }
//  u8x8.drawString(0, 2, "Found touch sensors!");
  Serial.println("MPR121 found!");
//  display.drawString(0,1,"Touch enabled");
//  display.display();


  delay(1000);

#ifdef USE_DISPLAY
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
//  display.invertDisplay(1);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("B");
  display.display();
  delay(500);
  for (int i=0; i<4; i++) {
    display.print("a");
    display.display();
    delay(500);
  }
  display.clearDisplay();
  display.display();

  display.clearDisplay();
  display.println("Ready");
  display.display();
#endif
//  delay(1000);
//  display.clearDisplay();
//  display.display();

  myDFPlayer.volume(30);  //Set volume value. From 0 to 30
  myDFPlayer.play(1);  //Play the first mp3
}

int sensorTable[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

void displayState() {
#ifdef USE_DISPLAY
  display.clearDisplay();
  display.setCursor(0,0);
  for ( int i = 0; i < 12; i++ ) {
    if ( sensorTable[i] ) {
      display.print(i);
      display.print(" ");
    }
  }
  display.display();
#endif
}

void loop() {


  static unsigned long last_sheep_sound = 0;
  
  // Get the currently touched pads
  currtouched = cap.touched();
  bool stateChanged = false;
  bool canMakeSound = false;
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      Serial.print(i); Serial.println(" touched");
      sensorTable[i] = 1;
      stateChanged = true;
      canMakeSound = true;
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      Serial.print(i); Serial.println(" released");
      sensorTable[i] = 0;
      stateChanged = true;
    }
  }

  if ( canMakeSound && millis() - last_sheep_sound > 3000 ) {
    last_sheep_sound = millis();
    myDFPlayer.next();  //Play next mp3 every 3 second.    
  }
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
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


/**
 * MP3 Module Helper
 */
 
void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
