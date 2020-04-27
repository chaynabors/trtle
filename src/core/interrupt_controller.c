#include "pch.h"
#include "interrupt_controller.h"

#include "gameboy.h"

#define INTERRUPT_MASK (0b11100000)

InterruptController * interrupt_controller_create() {
    return calloc(1, sizeof(InterruptController));
}

void interrupt_controller_delete(InterruptController ** const ic) {
    free(*ic);
    *ic = NULL;
}

void interrupt_controller_initialize(InterruptController * const ic, bool skip_bootrom) {
    ic->enables = 0;
    ic->flags = skip_bootrom ? 1 : 0;
    ic->ime = 0;
    ic->cycles_until_ime = -1;
}

uint8_t interrupt_controller_get_enables(GameBoy const * const gb) {
    return gb->interrupt_controller->enables;
}

void interrupt_controller_set_enables(GameBoy * const gb, uint8_t value) {
    gb->interrupt_controller->enables = value;
}

uint8_t interrupt_controller_get_flags(GameBoy const * const gb) {
    return gb->interrupt_controller->flags | INTERRUPT_MASK;
}

void interrupt_controller_set_flags(GameBoy * const gb, uint8_t value) {
    gb->interrupt_controller->flags = value | INTERRUPT_MASK;
}
