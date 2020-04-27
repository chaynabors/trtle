#include "pch.h"
#include "sound_controller.h"
#include "gameboy.h"

#define SOUND_CONTROLLER_NR10_MASK (0b10000000)
#define SOUND_CONTROLLER_NR30_MASK (0b01111111)
#define SOUND_CONTROLLER_NR32_MASK (0b10011111)
#define SOUND_CONTROLLER_NR41_MASK (0b11000000)
#define SOUND_CONTROLLER_NR44_MASK (0b00111111)
#define SOUND_CONTROLLER_NR52_MASK (0b01110000)

void sound_controller_initialize(GameBoy *const gb) {
    gb->sound_controller.nr10 = 0x80;
    gb->sound_controller.nr11 = 0xBF;
    gb->sound_controller.nr12 = 0xF3;
    gb->sound_controller.nr14 = 0xBF;
    gb->sound_controller.nr21 = 0x3F;
    gb->sound_controller.nr22 = 0x00;
    gb->sound_controller.nr24 = 0xBF;
    gb->sound_controller.nr30 = 0x7F;
    gb->sound_controller.nr31 = 0xFF;
    gb->sound_controller.nr32 = 0x9F;
    gb->sound_controller.nr34 = 0xBF;
    gb->sound_controller.nr41 = 0xFF;
    gb->sound_controller.nr42 = 0x00;
    gb->sound_controller.nr43 = 0x00;
    gb->sound_controller.nr44 = 0xBF;
    gb->sound_controller.nr50 = 0x77;
    gb->sound_controller.nr51 = 0xF3;
    gb->sound_controller.nr52 = 0xF1;
}

uint8_t sound_controller_read_nr10(GameBoy const * const gb) {
    return gb->sound_controller.nr10 | SOUND_CONTROLLER_NR10_MASK;
}

uint8_t sound_controller_read_nr30(GameBoy const* const gb) {
    return gb->sound_controller.nr30 | SOUND_CONTROLLER_NR30_MASK;
}
uint8_t sound_controller_read_nr32(GameBoy const* const gb) {
    return gb->sound_controller.nr32 | SOUND_CONTROLLER_NR32_MASK;
}

uint8_t sound_controller_read_nr41(GameBoy const* const gb) {
    return gb->sound_controller.nr41 | SOUND_CONTROLLER_NR41_MASK;
}

uint8_t sound_controller_read_nr44(GameBoy const* const gb) {
    return gb->sound_controller.nr44 | SOUND_CONTROLLER_NR44_MASK;
}

uint8_t sound_controller_read_nr52(GameBoy const* const gb) {
    return gb->sound_controller.nr52 | SOUND_CONTROLLER_NR52_MASK;
}
