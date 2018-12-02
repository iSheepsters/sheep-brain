# Config file

| Value | Description |
| --- | --- |
| sheepNum | the number for the sheep voices |
| privates enabled | if non-zero, private sensor is enabled
| R rated | if non-zero, R rated sounds enabled
| default volume | 0 - 63, 63 is maximum, 0 is silent, 50 is a typical loud volume
| time zone offset | difference from EST
| # time periods | the number of lines each containing the following fields 
| start duration volume | start is the starting o’clock in EST (0-23) for a volume reduction, duration is the number of hours, volume in that window

For example, the following config file
```
1
0
0
48
0
1
22 10 40
```

Specifies the following:
* Sheep 1
* privates not enabled
* R-rated sounds not enabled
* default volume of 48
* no time zone offset (use EST)
* 1 time period offset
 * If time is between 10pm and 8am, use a volume of 40


# Libraries used

| Library | Version | Notes |
| --- | --- |  --- | 
|Adafruit Sleepydog
|Wire
|TimeLib
|Adafruit\_GPS
|RH\_RF95
|Adafruit\_VS1053 | Custom | Modified to use SdFat
|SdFat
| Adafruit\_MPR121 | 

# Hardware

| Module | I2C  | Serial | SPI |  Notes |
| --- | --- | --- | --- | --- | 
| [Feather M0/LoRa][1] |   |  | 
| [Music Maker FeatherWing][2] |  | | yes | Both audio codec and SD card; SD select = 5
| [GPS Featherwing][3] | | yes | |
| [Capacitive Touch Sensor][4] | 0x5A | | | 
| [20W amplifier][5] | 0x4B | | | |
| Teensy 3.5 | 0x44 | | | 

[1]:	https://learn.adafruit.com/adafruit-feather-m0-radio-with-lora-radio-module/downloads
[2]:	https://learn.adafruit.com/adafruit-music-maker-featherwing/
[3]:	https://learn.adafruit.com/adafruit-ultimate-gps-featherwing
[4]:	https://learn.adafruit.com/adafruit-mpr121-12-key-capacitive-touch-sensor-breakout-tutorial
[5]:	https://learn.adafruit.com/adafruit-20w-stereo-audio-amplifier-class-d-max9744
