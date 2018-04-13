#ifndef DEBUG_H_
#define DEBUG_H_

#include "Arduino.h"

void dbg(String msg, Stream *s = &Serial);
void dbg(int msg, Stream *s = &Serial);

#endif /* DEBUG_H_ */
