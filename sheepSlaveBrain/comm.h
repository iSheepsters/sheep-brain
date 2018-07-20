#include <Wire.h>
#include "printf.h"

extern uint8_t mem[MEM_LEN];
// Function prototypes
void setupComm();
void receiveEvent(int len);
void requestEvent(void);


