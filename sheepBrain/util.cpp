
#include "util.h"
#include "printf.h"

 SdFat SD;

uint16_t totalYield;
/* return voltage for 12v battery */
uint16_t batteryVoltageRaw() {
  return analogRead(A2);
}

void writeSheepNumber(int s) {
  File configFile = SD.open("config.txt", O_WRITE | O_TRUNC );
  configFile.println(s);
  configFile.close();
  myprintf(Serial, "Wrote %d to config.txt\n", sheepNumber);
}
uint8_t millisToSecondsCapped(unsigned long ms) {
  ms = ms / 1000;
  if (ms < 255) return ms;
  return 255;
}

int sheepToSwitchTo(int s) {
  if (minutesPerSheep == 0)
    return s;
  s = s + 1;
  if (s > 12)
    return s - 12;
  return s;
}

SheepInfo & getSheep() {
  if (sheepNumber == 0)
    Serial.println("huh. sheepNumber is 0 in  getSheep()");
  return getSheep(sheepNumber);
}

void setupDelay(uint16_t ms) {
  delay(ms);
}

void yield(uint16_t ms) {
  totalYield += ms;

  delay(ms);
}

float batteryVoltage() {
  return batteryVoltageRaw() * 3.3 * 5.7 / 1024;
}
uint8_t batteryCharge() {
  // https://www.energymatters.com.au/components/battery-voltage-discharge/
  float v = batteryVoltage();
  v = 50 + (v - 12.2) / 0.2 * 25;
  if (v > 100) v = 100;
  if (v < 0) v = 0;
  return (int) v;
}
