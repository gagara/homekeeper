#include "debug.h"

void dbg(String msg, Stream *s) {
#ifdef __DEBUG__
    s.print(msg);
#endif
}

void dbg(int msg, Stream *s) {
#ifdef __DEBUG__
    s.print(msg);
#endif
}
