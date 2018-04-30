# sheep-brain
The brains of the Sheep, running on an ESP32

See https://github.com/iSheepsters/sheep-brain/issues for work we are trying to do.


# Installation

For OSX, we had to install the following:

- Arduino IDE 1.8.5
- Install Heltec ESP32 board info (https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/mac.md). You may also need to install a modern version of Python
- Install Slab to UART (http://esp-idf.readthedocs.io/en/latest/get-started/establish-serial-connection.html)

# Hardware

| Module | I2C  | Serial | Notes |
| --- | --- | --- | --- |
| MP3 | No | Yes |  |
| Touch  | Yes  | No |  |
| GPS | No | Yes |  |
| LORA | No | No | Attached to ESP32 Development Board |
| OLED Display | Yes | No | Attached to ESP32 Development Board |
| FM Receiver | Yes | No | Not purchased yet. TEA5767 maybe?
