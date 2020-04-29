#ifndef TRTLE_PPU_H
#define TRTLE_PPU_H

#define PPU_ROWS_PER_TILE       (8)
#define PPU_PIXELS_PER_TILE_ROW (8)

// Tilemap macros
#define PPU_TS_TILE_COUNT       (384 * 2)
#define PPU_TS_WIDTH_IN_PIXELS  (128)
#define PPU_TS_HEIGHT_IN_PIXELS (192)

// Background macros
#define PPU_BG_WIDTH_IN_PIXELS  (256)
#define PPU_BG_HEIGHT_IN_PIXELS (256)

// TODO: Consider the fact that this is duplicated in gameboy.h
#define PPU_DISPLAY_WIDTH  (160)
#define PPU_DISPLAY_HEIGHT (144)

typedef struct GameBoy GameBoy;

typedef enum GraphicsMode {
    GRAPHICS_MODE_HBLANK = 0,
    GRAPHICS_MODE_VBLANK = 1,
    GRAPHICS_MODE_OAM_SEARCH = 2,
    GRAPHICS_MODE_DATA_TRANSFER = 3,
} GraphicsMode;

typedef struct PPU {
    uint8_t lcdc;
    uint8_t stat;
    uint8_t scy;
    uint8_t scx;
    uint8_t ly;
    uint8_t lyc;
    uint8_t bgp;
    uint8_t obp0;
    uint8_t obp1;
    uint8_t wy;
    uint8_t wx;
    uint8_t oam[0xA0];
    uint8_t vram[0x2000];

    uint8_t window_internal_line;

    uint8_t tile_buffer[PPU_TS_TILE_COUNT][PPU_ROWS_PER_TILE][PPU_PIXELS_PER_TILE_ROW];
    uint8_t display_buffer[PPU_DISPLAY_WIDTH * PPU_DISPLAY_HEIGHT];

    size_t count;
} PPU;

void ppu_initialize(PPU * const ppu, bool skip_bootrom);

void ppu_cycle(GameBoy * const gb);

uint8_t ppu_read_lcdc(GameBoy const * const gb);
void ppu_write_lcdc(GameBoy * const gb, uint8_t value);

uint8_t ppu_read_stat(GameBoy const * const gb);
void ppu_write_stat(GameBoy * const gb, uint8_t value);

uint8_t ppu_read_oam(GameBoy const * const gb, uint16_t address);
void ppu_write_oam(GameBoy * const gb, uint16_t address, uint8_t value);

uint8_t ppu_read_vram(GameBoy const * const gb, uint16_t address);
void ppu_write_vram(GameBoy * const gb, uint16_t address, uint8_t value);

GraphicsMode ppu_get_mode(GameBoy const * const gb);

size_t ppu_get_background_data(GameBoy const* const gb, uint8_t data[], size_t length);
size_t ppu_get_display_data(GameBoy const* const gb, uint8_t data[], size_t length);
size_t ppu_get_tileset_data(GameBoy const * const gb, uint8_t data[], size_t length);

#endif /* !TRTLE_PPU_H */
