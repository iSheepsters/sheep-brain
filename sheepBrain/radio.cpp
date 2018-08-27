
#include <Arduino.h>
#include <RH_RF95.h>

#include "util.h"
#include "sound.h"
#include "radio.h"
#include "printf.h"
#include "secret.h"
#include "touchSensors.h"
#include "state.h"
#include "GPS.h"
#include "logging.h"
#include <TimeLib.h>

typedef uint16_t hash_t;

/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3


const uint16_t RADIO_WINDOW = 4;

time_t lastEpoc = 0;
struct __attribute__ ((packed)) HashInfo {
  uint16_t sheepNumber;
  const uint16_t secret = SECRET_SALT;
  time_t currentTime;
  hash_t getHash() {
    return sheepNumber + secret + currentTime;
  }

};


struct  __attribute__ ((packed)) RadioInfo {
  const enum PacketKind packetKind = InfoPacket;
  uint8_t sheepNumber;
  uint8_t waywardSheepNumber;
  uint8_t currTouched;
  enum State currState;
  time_t currentTime;
  hash_t hashValue;
  SheepInfo myInfo;
  SheepInfo waywardSheep;
};


struct  __attribute__ ((packed)) DistressInfo {
  enum PacketKind packetKind = DistressPacket;
  uint8_t sheepNumber;
  hash_t hashValue;
  time_t currentTime;

  char msg[200];
};

DistressInfo distressInfo;

RadioInfo radioInfo;
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0


// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

boolean radioAvailable = false;
boolean setupRadio() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  setupDelay(10);
  digitalWrite(RFM95_RST, LOW);
  setupDelay(10);
  digitalWrite(RFM95_RST, HIGH);
  setupDelay(10);
  memset(&radioInfo, 0, sizeof(radioInfo));
  memset(&distressInfo, 0, sizeof(distressInfo));

  if (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    return false;
  }

  Serial.println("LoRa radio init OK!");
  if (false) {
    myprintf(Serial, "Size of sheepInfo is %d\n", sizeof(SheepInfo));
    myprintf(Serial, "Size of radioInfo is %d\n", sizeof(radioInfo));
    myprintf(Serial, "Size of RadioInfo is %d\n", sizeof(RadioInfo));
  }
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    return false;
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // change to < Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
  if (true)
    rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512);
  else
    rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);// The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  radioAvailable = true;
  return true;
}

int lastWaywardSheep = 0;

int nextSheep(int s) {
  if (s >= NUMBER_OF_SHEEP)
    return 1;
  if (s < 0)
    return 1;
  return s + 1;
}

boolean findNextWaywardSheep() {
  int count = 0;
  int s = lastWaywardSheep;
  boolean continueLooking = true;
  while (continueLooking) {
    count++;
    if (count > NUMBER_OF_SHEEP)
      return false;
    s = nextSheep(s);
    if (s == lastWaywardSheep)
      continueLooking = false;
    if (s == sheepNumber)
      continue;
    if (getSheep(s).time == 0)
      continue;
    if (inCorral(s))
      continue;
    lastWaywardSheep = s;
    return true;
  }
}

HashInfo hashInfo;
int badPackets = 0;
int badPacketReport = 10;

void updateRadio() {
  if (!radioAvailable) return;
  if (badPacketReport < badPackets) {
    myprintf(Serial, "Received %d bad packets\n", badPackets);
    badPacketReport = 10 + 3 * badPacketReport / 2;

  }
  while (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      HashInfo hashInfo;



      switch ((enum PacketKind) buf[0]) {
        case InfoPacket: {
            RadioInfo * received = (RadioInfo *)buf;
            uint8_t fromNum = received->sheepNumber;
            SheepInfo & from = received->myInfo;
            hashInfo.sheepNumber = fromNum;
            hashInfo.currentTime = received->currentTime;
            if (hashInfo.getHash() != received->hashValue) {
              Serial.println("Radio Info with bad hash value, rejecting");
              badPackets++;
              break;
            }

            myprintf(Serial, "Received info packet of length %d from sheep %d\n", len, fromNum);
            memcpy(&getSheep(fromNum), &from,  sizeof(SheepInfo));
            logRadioUpdate(fromNum, from);
            uint8_t waywardSheepNumber = received->waywardSheepNumber;
            if (len == sizeof(radioInfo) && waywardSheepNumber != 0
                && received->waywardSheep.time > getSheep(waywardSheepNumber).time) {
              memcpy(&getSheep(waywardSheepNumber), &received->waywardSheep, sizeof(SheepInfo));
              logRadioUpdate(waywardSheepNumber, received->waywardSheep);
            }
            break;
          }
        case DistressPacket: {
            DistressInfo * received = (DistressInfo *)buf;
            uint8_t fromNum = received->sheepNumber;
            hashInfo.sheepNumber = fromNum;
            hashInfo.currentTime = received->currentTime;
            if (hashInfo.getHash() != received->hashValue) {
              Serial.println("Radio Distress packet with bad hash value, rejecting");
              badPackets++;
              break;
            }
            myprintf(Serial, "Received distress packet of length %d from sheep %d\n", len, fromNum);
            myprintf(Serial, "msg: %s\n",  received->msg);
            logRadioDistress(fromNum, distressInfo.currentTime, getSheep(fromNum), received->msg);
            break;
          }
        default: {
            //myprintf(Serial, "Received unknown packet of length %d of type %d\n", len, buf[0]);
            badPackets++;
          }

      }
    } else {
      Serial.println("Radio receive failed");
      break;
    }
  }
  time_t trueTime = now();
  time_t radioEpoc = trueTime / RADIO_EPOC;
  uint16_t radioWindow = (trueTime % RADIO_EPOC) / RADIO_WINDOW;
  if (radioEpoc != lastEpoc && radioWindow == sheepNumber) {
    myprintf(Serial, "Sending out radio packet for epoc %d\n", radioEpoc);
    if (ampOn)
      Serial.println("amp on");
    else
      Serial.println("amp off");
    lastEpoc = radioEpoc;
    hashInfo.sheepNumber = sheepNumber;
    hashInfo.currentTime = trueTime;

    radioInfo.sheepNumber = sheepNumber;
    radioInfo.currentTime = hashInfo.currentTime;
    radioInfo.hashValue = hashInfo.getHash();
    radioInfo.currState = currentSheepState->state;
    radioInfo.currTouched = currTouched;

    memcpy(&(radioInfo.myInfo), &getSheep(), sizeof(SheepInfo));
    Serial.println("Looking for wayward sheep");
    boolean waywardSheep = findNextWaywardSheep();

    size_t length = sizeof(radioInfo);
    if (waywardSheep) {
      myprintf(Serial, "Found wayward sheep %d\n", lastWaywardSheep );

      radioInfo.waywardSheepNumber = lastWaywardSheep;
      memcpy(&(radioInfo.waywardSheep), &getSheep(lastWaywardSheep), sizeof(SheepInfo));
    } else {
      Serial.println("No wayward sheep found");
      radioInfo.waywardSheepNumber = 0;
      length -= sizeof(SheepInfo);
    }
    uint8_t * data = (uint8_t *)&radioInfo;
    myprintf(Serial, "Sending radio packet of size %d\n", length);

    if (rf95.send(data,  length)) {
      // rf95.waitPacketSent();
    } else
      Serial.println("Radio packet send failed");
  }
}

void distressPacket(char * msg) {
  if (!radioAvailable) return;
  int msgLength = strlen(msg);
  if (msgLength > 199) {
    msg[200] = '\0';
    msgLength = strlen(msg);
  }
  if (msgLength > 199) {
    Serial.println("msg length too long");
    return;
  }
  hashInfo.sheepNumber = sheepNumber;
  hashInfo.currentTime = now();
  distressInfo.packetKind = DistressPacket;
  distressInfo.sheepNumber = sheepNumber;
  distressInfo.currentTime = hashInfo.currentTime;
  distressInfo.hashValue = hashInfo.getHash();
  strncpy(distressInfo.msg, msg, 200);
  size_t packetLength = sizeof(distressInfo) - 200 + strlen(msg) + 1;
  if (rf95.send((uint8_t *)&distressInfo, packetLength)) {
    myprintf(Serial, "distress Radio packet sent.  length %d\n  ",
             packetLength);
    for (int i = 0; i < 16; i++)
      myprintf(Serial, "%02x ",  ((uint8_t *)&distressInfo)[i]);
    Serial.println();
  }
  else
    Serial.println("distress Radio packet send failed");

  rf95.waitPacketSent();
}


