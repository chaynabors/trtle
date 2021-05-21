#include "sound_controller.h"

#include "gameboy.h"

#define SOUND_CONTROLLER_NR10_MASK (0b10000000)
#define SOUND_CONTROLLER_NR30_MASK (0b01111111)
#define SOUND_CONTROLLER_NR32_MASK (0b10011111)
#define SOUND_CONTROLLER_NR41_MASK (0b11000000)
#define SOUND_CONTROLLER_NR44_MASK (0b00111111)
#define SOUND_CONTROLLER_NR52_MASK (0b01110000)

void sound_controller_initialize(SoundController * const sc, bool skip_bootrom) {
    sc->nr10 = 0x80;
    sc->nr11 = 0xBF;
    sc->nr12 = 0xF3;
    sc->nr14 = 0xBF;
    sc->nr21 = 0x3F;
    sc->nr22 = 0x00;
    sc->nr24 = 0xBF;
    sc->nr30 = 0x7F;
    sc->nr31 = 0xFF;
    sc->nr32 = 0x9F;
    sc->nr34 = 0xBF;
    sc->nr41 = 0xFF;
    sc->nr42 = 0x00;
    sc->nr43 = 0x00;
    sc->nr44 = 0xBF;
    sc->nr50 = 0x77;
    sc->nr51 = 0xF3;
    sc->nr52 = 0xF1;
}

uint8_t sound_controller_read_nr10(GameBoy const * const gb) {
    return gb->sound_controller->nr10 | SOUND_CONTROLLER_NR10_MASK;
}

uint8_t sound_controller_read_nr30(GameBoy const* const gb) {
    return gb->sound_controller->nr30 | SOUND_CONTROLLER_NR30_MASK;
}
uint8_t sound_controller_read_nr32(GameBoy const* const gb) {
    return gb->sound_controller->nr32 | SOUND_CONTROLLER_NR32_MASK;
}

uint8_t sound_controller_read_nr41(GameBoy const* const gb) {
    return gb->sound_controller->nr41 | SOUND_CONTROLLER_NR41_MASK;
}

uint8_t sound_controller_read_nr44(GameBoy const* const gb) {
    return gb->sound_controller->nr44 | SOUND_CONTROLLER_NR44_MASK;
}

uint8_t sound_controller_read_nr52(GameBoy const* const gb) {
    return gb->sound_controller->nr52 | SOUND_CONTROLLER_NR52_MASK;
}
