#include "pch.h"
#include "serial.h"
#include "gameboy.h"

#define SERIAL_SC_MASK (0b01111110)

void serial_initialize(GameBoy * const gb) {
    gb->serial.sb = 0x00; // TODO TEMP
    gb->serial.sc = 0x00; // TODO TEMP
}

uint8_t serial_read_sc(GameBoy const * const gb) {
    return gb->serial.sc | SERIAL_SC_MASK;
}
