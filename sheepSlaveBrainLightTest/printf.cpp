
#include "printf.h"

 void printf(Print & printer, const char *fmt, ... ){
        char buf[256]; // resulting string limited to 256 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 256, fmt, args);
        va_end (args);
        printer.print(buf);
}

