#ifndef TRTLE_GAMEBOY_H
#define TRTLE_GAMEBOY_H

#include <stdbool.h>
#include <stdint.h>

#define GAMEBOY_TILESET_WIDTH  (128)
#define GAMEBOY_TILESET_HEIGHT (192)
#define GAMEBOY_BACKGROUND_WIDTH  (256)
#define GAMEBOY_BACKGROUND_HEIGHT (256)
#define GAMEBOY_DISPLAY_WIDTH  (160)
#define GAMEBOY_DISPLAY_HEIGHT (144)
#define GAMEBOY_DISPLAY_PIXEL_COUNT (GAMEBOY_DISPLAY_WIDTH * GAMEBOY_DISPLAY_HEIGHT)

#define GAMEBOY_BOOTROM_ADDRESS     (0x0000)
#define GAMEBOY_ROM_ADDRESS         (0x0000)
#define GAMEBOY_VRAM_ADDRESS        (0x8000)
#define GAMEBOY_OAM_ADDRESS         (0xFE00)

typedef struct Cartridge Cartridge;
typedef struct DMA DMA;
typedef struct InterruptController InterruptController;
typedef struct Joypad Joypad;
typedef struct PPU PPU;
typedef struct Processor Processor;
typedef struct Serial Serial;
typedef struct SoundController SoundController;
typedef struct Timer Timer;

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

typedef struct GameBoy {
    Cartridge * cartridge;
    DMA * dma;
    InterruptController * interrupt_controller;
    Joypad * joypad;
    PPU * ppu;
    Processor * processor;
    Serial * serial;
    SoundController * sound_controller;
    Timer * timer;
    uint8_t boot;
} GameBoy;

GameBoy * gameboy_create();
void gameboy_delete(GameBoy * gb);
void gameboy_reset(GameBoy * gb);

void gameboy_set_cartridge(GameBoy * const gb, Cartridge * const cartridge);

void gameboy_update(GameBoy * const gb, GameBoyInput input);
void gameboy_update_to_vblank(GameBoy * const gb, GameBoyInput input);

size_t gameboy_get_background_data(GameBoy const * const gb, uint32_t * data, size_t length);
size_t gameboy_get_display_data(GameBoy const * const gb, uint32_t * data, size_t length);
size_t gameboy_get_tileset_data(GameBoy const * const gb, uint32_t * data, size_t length);

void gameboy_cycle(GameBoy* const gb);

uint8_t gameboy_read(GameBoy* const gb, uint16_t address);
void gameboy_write(GameBoy* const gb, uint16_t address, uint8_t value);

#endif /* !TRTLE_GAMEBOY_H */
