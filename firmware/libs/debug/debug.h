#ifndef DEBUG_H_
#define DEBUG_H_

#include "Arduino.h"

#define __DEBUG__

void dbg(Stream *s, const char *msg);
void dbgf(Stream *s, const char *fomat, ...);
void dbg(Stream *s, const __FlashStringHelper *msg);
void dbgf(Stream *s, const __FlashStringHelper *fomat, ...);

#endif /* DEBUG_H_ */
