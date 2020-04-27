#ifndef TRTLE_JOYPAD_H
#define TRTLE_JOYPAD_H

typedef struct GameBoy GameBoy;
typedef struct GameBoyInput GameBoyInput;

typedef struct Joypad {
    uint8_t p1;
    uint8_t buttons;
    uint8_t directions;
} Joypad;

Joypad * joypad_create();
void joypad_delete(Joypad ** const jp);
void joypad_initialize(Joypad * const jp, bool skip_bootrom);

uint8_t joypad_read_p1(GameBoy * const gb);
void joypad_write_p1(GameBoy * const gb, uint8_t value);

void joypad_update_p1(GameBoy * const gb, GameBoyInput input);

#endif /* !TRTLE_JOYPAD_H */
