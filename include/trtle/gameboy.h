#ifndef TRTLE_GAMEBOY_H
#define TRTLE_GAMEBOY_H

#include <stdbool.h>
#include <stdint.h>

#define GAMEBOY_BACKGROUND_WIDTH  (256)
#define GAMEBOY_BACKGROUND_HEIGHT (256)
#define GAMEBOY_DISPLAY_WIDTH     (160)
#define GAMEBOY_DISPLAY_HEIGHT    (144)
#define GAMEBOY_TILESET_WIDTH     (128)
#define GAMEBOY_TILESET_HEIGHT    (192)

typedef struct Cartridge Cartridge;
typedef struct GameBoy GameBoy;

typedef struct GameBoyInput {
    bool a;
    bool b;
    bool start;
    bool select;
    bool up;
    bool down;
    bool left;
    bool right;
} GameBoyInput;

extern GameBoy * gameboy_create();
extern void gameboy_delete(GameBoy * gb);
extern void gameboy_insert_cartridge(GameBoy * gb, Cartridge * cart);
extern void gameboy_initialize(GameBoy * const gb, bool skip_bootrom);
extern void gameboy_update(GameBoy * const gb, GameBoyInput input);
extern void gameboy_update_to_vblank(GameBoy * const gb, GameBoyInput input);
extern size_t gameboy_get_background_data(GameBoy const * const gb, uint8_t data[], size_t length);
extern size_t gameboy_get_display_data(GameBoy const * const gb, uint8_t data[], size_t length);
extern size_t gameboy_get_tileset_data(GameBoy const * const gb, uint8_t data[], size_t length);

#endif /* !TRTLE_GAMEBOY_H */
