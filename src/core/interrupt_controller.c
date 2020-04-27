#include "pch.h"
#include "interrupt_controller.h"
#include "gameboy.h"

#define INTERRUPT_MASK (0b11100000)

void interrupt_controller_initialize(GameBoy * const gb, bool skip_bootrom) {
    gb->interrupt_controller.enables = 0;
    gb->interrupt_controller.flags = skip_bootrom ? 1 : 0;
    gb->interrupt_controller.ime = 0;
    gb->interrupt_controller.cycles_until_ime = -1;
}

uint8_t interrupt_controller_get_enables(GameBoy const * const gb) { return gb->interrupt_controller.enables; }
void interrupt_controller_set_enables(GameBoy * const gb, uint8_t value) { gb->interrupt_controller.enables = value; }

uint8_t interrupt_controller_get_flags(GameBoy const * const gb) { return gb->interrupt_controller.flags | INTERRUPT_MASK; }
void interrupt_controller_set_flags(GameBoy * const gb, uint8_t value) { gb->interrupt_controller.flags = value | INTERRUPT_MASK; }
