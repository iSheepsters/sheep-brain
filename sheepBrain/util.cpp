
#include "util.h"
#include "printf.h"
/* return voltage for 12v battery */
uint16_t batteryVoltageRaw() {
  return analogRead(A2);
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
