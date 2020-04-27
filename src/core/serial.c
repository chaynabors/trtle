#include "pch.h"
#include "serial.h"

#include "gameboy.h"

#define SERIAL_SC_MASK (0b01111110)

Serial * serial_create() {
    return calloc(1, sizeof(Serial));
}

void serial_delete(Serial ** const s) {
    free(*s);
    *s = NULL;
}

void serial_initialize(Serial * const s, bool skip_bootrom) {
    // Stub
}

uint8_t serial_read_sc(GameBoy const * const gb) {
    return gb->serial->sc | SERIAL_SC_MASK;
}
