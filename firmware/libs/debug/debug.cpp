#include "debug.h"

#include <MemoryFree.h>

const uint8_t MAX_DEBUG_BUFFER_SIZE = 64;

void dbg(Stream *s, const char *msg) {
    if (s) {
        if (msg[0] == ':' && msg[1] != '\0') {
            s->print(millis());
            s->print(F(":["));
            s->print(freeMemory());
            s->print(F("]"));
        }
        s->print(msg);
    }
}

void dbg(Stream *s, const __FlashStringHelper *msg) {
    if (s) {
        PGM_P p = reinterpret_cast<PGM_P>(msg);
        if (pgm_read_byte(p) == ':' && pgm_read_byte(++p) != '\0') {
            s->print(millis());
            s->print(F(":["));
            s->print(freeMemory());
            s->print(F("]"));
        }
        s->print(msg);
    }
}

void dbgf(Stream *s, const char *format, ...) {
    if (s) {
        char buf[MAX_DEBUG_BUFFER_SIZE];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, MAX_DEBUG_BUFFER_SIZE, format, args);
        va_end(args);
        dbg(s, buf);
    }
}

void dbgf(Stream *s, const __FlashStringHelper *format, ...) {
    if (s) {
        // read from flash to memory
        char mfmt[MAX_DEBUG_BUFFER_SIZE];
        int i = 0;
        mfmt[i] = '\0';
        PGM_P p = reinterpret_cast<PGM_P>(format);
        char c;
        while ((c = pgm_read_byte(p++)) != 0 && i < MAX_DEBUG_BUFFER_SIZE - 1) {
            mfmt[i++] = c;
            mfmt[i] = '\0';
        }
        char buf[MAX_DEBUG_BUFFER_SIZE];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, MAX_DEBUG_BUFFER_SIZE, mfmt, args);
        va_end(args);
        dbg(s, buf);
    }
}
