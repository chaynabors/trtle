#ifndef TRTLE_SOUND_CONTROLLER_H
#define TRTLE_SOUND_CONTROLLER_H

typedef struct GameBoy GameBoy;

typedef struct SoundController {
    uint8_t nr10;
    uint8_t nr11;
    uint8_t nr12;
    uint8_t nr13;
    uint8_t nr14;
    uint8_t nr21;
    uint8_t nr22;
    uint8_t nr23;
    uint8_t nr24;
    uint8_t nr30;
    uint8_t nr31;
    uint8_t nr32;
    uint8_t nr33;
    uint8_t nr34;
    uint8_t nr41;
    uint8_t nr42;
    uint8_t nr43;
    uint8_t nr44;
    uint8_t nr50;
    uint8_t nr51;
    uint8_t nr52;
} SoundController;

void sound_controller_initialize(SoundController * const sc, bool skip_bootrom);

uint8_t sound_controller_read_nr10(GameBoy const * const gb);

uint8_t sound_controller_read_nr30(GameBoy const * const gb);

uint8_t sound_controller_read_nr32(GameBoy const * const gb);

uint8_t sound_controller_read_nr41(GameBoy const * const gb);

uint8_t sound_controller_read_nr44(GameBoy const * const gb);

uint8_t sound_controller_read_nr52(GameBoy const * const gb);

#endif /* !TRTLE_SOUND_CONTROLLER_H */
