#include "pch.h"
#include "joypad.h"

#include "gameboy.h"
#include "interrupt_controller.h"

enum P1Bit {
    P1_BIT_RIGHT         = 0b00000001,
    P1_BIT_LEFT          = 0b00000010,
    P1_BIT_UP            = 0b00000100,
    P1_BIT_DOWN          = 0b00001000,
    P1_BIT_A             = 0b00000001,
    P1_BIT_B             = 0b00000010,
    P1_BIT_SELECT        = 0b00000100,
    P1_BIT_START         = 0b00001000,
    P1_BIT_DIRECTIONS    = 0b00010000,
    P1_BIT_BUTTONS       = 0b00100000,
    P1_BIT_READONLY      = 0b00001111,
    P1_BIT_WRITABLE      = 0b00110000,
    P1_BIT_UNUSED        = 0b11000000
};

void joypad_initialize(Joypad * const jp, bool skip_bootrom) {
    jp->p1 = 0;
}

static void joypad_update_internal(GameBoy * const gb) {
    uint8_t prev = gb->joypad->p1 & P1_BIT_READONLY;

    uint8_t input = 0;
    switch ((gb->joypad->p1 & P1_BIT_WRITABLE) >> 4) {
        case 0b00: input |= ~(gb->joypad->buttons | gb->joypad->directions); break;
        case 0b01: input |= ~(gb->joypad->buttons); break;
        case 0b10: input |= ~(gb->joypad->directions); break;

        case 0b11:
        default: break;
    }
    input &= P1_BIT_READONLY;

    if (prev != input) gb->interrupt_controller->flags |= JOYPAD_INTERRUPT_BIT;

    gb->joypad->p1 &= ~P1_BIT_READONLY;
    gb->joypad->p1 |= input;
}

uint8_t joypad_read_p1(GameBoy * const gb) {
    joypad_update_internal(gb);
    return gb->joypad->p1 | P1_BIT_UNUSED;
}

void joypad_write_p1(GameBoy * const gb, uint8_t value) {
    gb->joypad->p1 = value & P1_BIT_WRITABLE;
    joypad_update_internal(gb);
}

void joypad_update_p1(GameBoy * const gb, GameBoyInput input) {
    gb->joypad->buttons = 0;
    if (input.a) gb->joypad->buttons |= P1_BIT_A;
    if (input.b) gb->joypad->buttons |= P1_BIT_B;
    if (input.select) gb->joypad->buttons |= P1_BIT_SELECT;
    if (input.start) gb->joypad->buttons |= P1_BIT_START;

    gb->joypad->directions = 0;
    if (input.right) gb->joypad->directions |= P1_BIT_RIGHT;
    if (input.left) gb->joypad->directions |= P1_BIT_LEFT;
    if (input.up) gb->joypad->directions |= P1_BIT_UP;
    if (input.down) gb->joypad->directions |= P1_BIT_DOWN;
}
