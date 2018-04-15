#include "debug.h"

const uint8_t MAX_DEBUG_BUFFER_SIZE = 64;

void dbg(Stream *s, const char *msg) {
#ifdef __DEBUG__
    s->print(msg);
#endif
}

void dbgf(Stream *s, const char *format, ...) {
#ifdef __DEBUG__
    char buf[MAX_DEBUG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, MAX_DEBUG_BUFFER_SIZE, format, args);
    va_end(args);
    dbg(s, buf);
#endif
}

void dbg(Stream *s, const __FlashStringHelper *msg) {
#ifdef __DEBUG__
    s->print(msg);
#endif
}

void dbgf(Stream *s, const __FlashStringHelper *format, ...) {
#ifdef __DEBUG__
    // read from flash to memory
    char mfmt[MAX_DEBUG_BUFFER_SIZE];
    int i = 0;
    mfmt[i] = '\0';
    PGM_P p = reinterpret_cast<PGM_P>(format);
    char c;
    while((c = pgm_read_byte(p++)) != 0 && i < MAX_DEBUG_BUFFER_SIZE - 1) {
        mfmt[i++] = c;
        mfmt[i] = '\0';
    }
    char buf[MAX_DEBUG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, MAX_DEBUG_BUFFER_SIZE, mfmt, args);
    va_end(args);
    dbg(s, buf);
#endif
}
