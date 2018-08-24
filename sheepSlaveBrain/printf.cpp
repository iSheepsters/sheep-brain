#include <Arduino.h>

#include "printf.h"

void myprintf( const char *fmt, ... ) {
  char buf[500]; // resulting string limited to 256 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, 500, fmt, args);
  va_end (args);
  Serial.print(buf);
}




