#include <i2c_t3.h>
#include "printf.h"

extern uint8_t mem[MEM_LEN];
// Function prototypes
void setupComm();
void receiveEvent(size_t len);
void requestEvent(void);
void print_i2c_status(void);

