#include "debug.h"

#include <Stream.h>

// Enable/Disable debug
#define __DEBUG__

void dbg(String msg, Stream *s) {
#ifdef __DEBUG__
    s.print(msg);
#endif
}
