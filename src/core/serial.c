#include "pch.h"
#include "serial.h"

#include "gameboy.h"

#define SERIAL_SC_MASK (0b01111110)

void serial_initialize(Serial * const s, bool skip_bootrom) {
    // Stub
}

uint8_t serial_read_sc(GameBoy const * const gb) {
    return gb->serial->sc | SERIAL_SC_MASK;
}
