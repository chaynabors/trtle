#include "pch.h"
#include "ppu.h"

#include "gameboy.h"
#include "interrupt_controller.h"

#define PPU_LCDC_LCD_ENABLE_BIT           (0b10000000)
#define PPU_LCDC_WINDOW_MAP_BIT           (0b01000000)
#define PPU_LCDC_WINDOW_ENABLE_BIT        (0b00100000)
#define PPU_LCDC_BG_WINDOW_MODE_BIT       (0b00010000)
#define PPU_LCDC_BG_MAP_BIT               (0b00001000)
#define PPU_LCDC_SPRITE_SIZE_BIT          (0b00000100)
#define PPU_LCDC_SPRITE_ENABLE_BIT        (0b00000010)
#define PPU_LCDC_BG_ENABLE_BIT            (0b00000001)

#define PPU_STAT_UNUSED_BITS              (0b10000000)
#define PPU_STAT_WRITE_BITS               (0b01111100)
#define PPU_STAT_LY_LYC_COMPARSION_ENABLE (0b01000000)
#define PPU_STAT_OAM_SEARCH_CHECK_ENABLE  (0b00100000)
#define PPU_STAT_VBLANK_CHECK_ENABLE      (0b00010000)
#define PPU_STAT_HBLANK_CHECK_ENABLE      (0b00001000)
#define PPU_STAT_LY_LYC_COMPARISON_SIGNAL (0b00000100)
#define PPU_STAT_MODE_BITS                (0b00000011)

#define PPU_SPRITE_TO_BG_PRIORITY_BIT     (0b10000000)
#define PPU_SPRITE_FLIP_Y_BIT             (0b01000000)
#define PPU_SPRITE_FLIP_X_BIT             (0b00100000)
#define PPU_SPRITE_PALETTE_BIT_DMG        (0b00010000)
#define PPU_SPRITE_VRAM_BANK              (0b00001000)
#define PPU_SPRITE_PALETTE_BITS_CGB       (0b00000111)

#define PPU_HBLANK_LENGTH (50)
#define PPU_VBLANK_LENGTH (114)
#define PPU_OAM_SEARCH_LENGTH (21)
#define PPU_DATA_TRANSFER_LENGTH (43)

#define PPU_BYTES_PER_TILE (16)
#define PPU_BYTES_PER_ROW  (2)

#define PPU_TS_WIDTH_IN_TILES   (16)

#define PPU_BG_MAP_OFFSET       (0x400)
#define PPU_BG_TILE_COUNT       (1024)
#define PPU_BG_WIDTH_IN_TILES   (32)
#define PPU_BACKGROUND1_START  (0x1800)
#define PPU_BACKGROUND2_START  (0x1C00)
#define PPU_BACKGROUND_LENGTH (0x0400)

#define PPU_LCD_COLOR_CODE (4)

PPU * ppu_create() {
    return calloc(1, sizeof(PPU));
}

void ppu_delete(PPU ** const ppu) {
    free(*ppu);
    *ppu = NULL;
}

void ppu_initialize(PPU * const ppu, bool skip_bootrom) {
    ppu->lcdc = 0x91;
    ppu->stat = 0x00;
    ppu->scy = 0x00;
    ppu->scx = 0x00;
    ppu->ly = 0x00;
    ppu->lyc = 0x00;
    ppu->bgp = 0xFC;
    ppu->obp0 = 0xFF;
    ppu->obp1 = 0xFF;
    ppu->wy = 0x00;
    ppu->wx = 0x00;

    ppu->window_internal_line = 0;

    ppu->count = 80;
}

size_t scx_cycle_offsets[] = {
    0, 1, 1, 1, 1, 2, 2, 2
};

static void ppu_hblank_enter(GameBoy * const gb) {
    gb->ppu->stat &= ~PPU_STAT_MODE_BITS;
    gb->ppu->stat |= GRAPHICS_MODE_HBLANK;
    gb->ppu->count += PPU_HBLANK_LENGTH - scx_cycle_offsets[gb->ppu->scx % 0x08];
}

static void ppu_vblank_enter(GameBoy * const gb) {
    gb->ppu->stat &= ~PPU_STAT_MODE_BITS;
    gb->ppu->stat |= GRAPHICS_MODE_VBLANK;
    gb->ppu->count += PPU_VBLANK_LENGTH;

    gb->ppu->window_internal_line = 0;

    gb->interrupt_controller->flags |= VBLANK_INTERRUPT_BIT;
    if ((gb->ppu->stat & PPU_STAT_VBLANK_CHECK_ENABLE) || (gb->ppu->stat & PPU_STAT_OAM_SEARCH_CHECK_ENABLE)) {
        gb->interrupt_controller->flags |= LCD_STAT_INTERRUPT_BIT;
    }
}

static void ppu_oam_search_enter(GameBoy * const gb) {
    gb->ppu->stat &= ~PPU_STAT_MODE_BITS;
    gb->ppu->stat |= GRAPHICS_MODE_OAM_SEARCH;
    gb->ppu->count += PPU_OAM_SEARCH_LENGTH;

    if (gb->ppu->stat & PPU_STAT_OAM_SEARCH_CHECK_ENABLE) {
        gb->interrupt_controller->flags |= LCD_STAT_INTERRUPT_BIT;
    }
}

static void ppu_data_transfer_enter(GameBoy * const gb) {
    gb->ppu->stat &= ~PPU_STAT_MODE_BITS;
    gb->ppu->stat |= GRAPHICS_MODE_DATA_TRANSFER;
    gb->ppu->count += PPU_DATA_TRANSFER_LENGTH + scx_cycle_offsets[gb->ppu->scx % 0x08];
}

static void ppu_compare_ly_lyc(GameBoy * const gb) {
    if (gb->ppu->ly != gb->ppu->lyc) {
        gb->ppu->stat &= ~PPU_STAT_LY_LYC_COMPARISON_SIGNAL;
    }
    else {
        gb->ppu->stat |= PPU_STAT_LY_LYC_COMPARISON_SIGNAL;
        if (gb->ppu->stat & PPU_STAT_LY_LYC_COMPARSION_ENABLE) {
            gb->interrupt_controller->flags |= LCD_STAT_INTERRUPT_BIT;
        }
    }
}

static void ppu_draw_line(GameBoy * const gb) {
    // Draw data if enabled
    if (gb->ppu->lcdc & PPU_LCDC_BG_ENABLE_BIT) {
        uint8_t background_row = gb->ppu->scy + gb->ppu->ly;
        for (size_t i = 0; i < GAMEBOY_DISPLAY_WIDTH; i++) {
            uint8_t background_column = gb->ppu->scx + i;

            uint16_t map_offset = (gb->ppu->lcdc & PPU_LCDC_BG_MAP_BIT) ? PPU_BACKGROUND2_START : PPU_BACKGROUND1_START;
            uint16_t tile_id = gb->ppu->vram[map_offset + (background_column / 8) + (background_row / 8) * PPU_BG_WIDTH_IN_TILES];
            tile_id = (gb->ppu->lcdc & PPU_LCDC_BG_WINDOW_MODE_BIT) ? tile_id : tile_id + (256 * (uint_fast16_t)(tile_id < 128));

            uint8_t color = gb->ppu->tile_buffer[tile_id][background_row % 8][background_column % 8];
            uint8_t offset = color * 2;
            uint8_t bits = 0b00000011 << offset;
            color = (gb->ppu->bgp & bits) >> offset;

            gb->ppu->display_buffer[i + (size_t)gb->ppu->ly * GAMEBOY_DISPLAY_WIDTH] = color;
        }
    }

    if ((gb->ppu->lcdc & PPU_LCDC_WINDOW_ENABLE_BIT) && gb->ppu->wy <= gb->ppu->ly && gb->ppu->wx - 7 <= 0xA6) {
        uint8_t wx = gb->ppu->wx - 7;
        for (size_t i = wx; i < GAMEBOY_DISPLAY_WIDTH; i++) {
            uint8_t window_column = gb->ppu->scx + i;
            if (window_column >= wx) window_column = i - wx;

            uint16_t map_offset = (gb->ppu->lcdc & PPU_LCDC_WINDOW_MAP_BIT) ? PPU_BACKGROUND2_START : PPU_BACKGROUND1_START;
            uint16_t tile_id = gb->ppu->vram[map_offset + (window_column / 8) + (gb->ppu->window_internal_line / 8) * PPU_BG_WIDTH_IN_TILES];
            tile_id = (gb->ppu->lcdc & PPU_LCDC_BG_WINDOW_MODE_BIT) ? tile_id : tile_id + (256 * (uint_fast16_t)(tile_id < 128));

            uint8_t color = gb->ppu->tile_buffer[tile_id][gb->ppu->window_internal_line % 8][window_column % 8];
            uint8_t offset = color * 2;
            uint8_t bits = 0b00000011 << offset;
            color = (gb->ppu->bgp & bits) >> offset;

            gb->ppu->display_buffer[i + (size_t)gb->ppu->ly * GAMEBOY_DISPLAY_WIDTH] = color;
        }
        gb->ppu->window_internal_line++;
    }

    if (gb->ppu->lcdc & PPU_LCDC_SPRITE_ENABLE_BIT) {
        uint8_t sprite_size = (gb->ppu->lcdc & PPU_LCDC_SPRITE_SIZE_BIT) ? 16 : 8;

        uint8_t sprites[40];
        size_t current = 0;
        for (size_t i = 0; i < 160; i += 4) {
            int32_t sprite_y = gb->ppu->oam[i] - 16;
            if (sprite_y <= gb->ppu->ly && (gb->ppu->ly - sprite_y) < sprite_size) {
                for (size_t k = 0; k < 4; k++) sprites[current * 4 + k] = gb->ppu->oam[i + k];
                if (++current == 10) break;
            }
        }

        for (size_t i = 0; i < current == 0 ? 0 : current - 1; i++) {
            size_t sprite_min_x = i;
            for (size_t k = i + 1; k < current; k++) {
                if (sprites[k * 4 + 1] < sprites[sprite_min_x * 4 + 1]) sprite_min_x = k;
            }

            if (sprite_min_x != i) {
                for (size_t k = 0; k < 4; k++) {
                    uint8_t temp = sprites[i * 4 + k];
                    sprites[i * 4 + k] = sprites[sprite_min_x * 4 + k];
                    sprites[sprite_min_x * 4 + k] = temp;
                }
            }
        }

        for (size_t i = current; i > 0; i--) {
            int32_t sprite_y = sprites[(i - 1) * 4] - 16;
            int32_t sprite_x = sprites[(i - 1) * 4 + 1] - 8;
            uint8_t sprite_t = sprites[(i - 1) * 4 + 2];
            uint8_t sprite_a = sprites[(i - 1) * 4 + 3];

            bool flip_x = (sprite_a & PPU_SPRITE_FLIP_X_BIT) >> 5;
            bool flip_y = (sprite_a & PPU_SPRITE_FLIP_Y_BIT) >> 6;
            bool priority = (sprite_a & PPU_SPRITE_TO_BG_PRIORITY_BIT) >> 7;

            uint16_t tile_id;
            if (sprite_size == 16) tile_id = sprite_t & 0xFE;
            else tile_id = sprite_t;

            uint8_t tile_row;
            if (sprite_a & PPU_SPRITE_FLIP_Y_BIT) tile_row = sprite_size - 1 - (gb->ppu->ly - sprite_y);
            else tile_row = gb->ppu->ly - sprite_y;
            if (tile_row >= 8) {
                tile_id += 1;
                tile_row -= 8;
            }

            for (size_t tile_column = 0; tile_column < 8; tile_column++) {
                if (sprite_x + tile_column < 0 || sprite_x + tile_column > GAMEBOY_DISPLAY_WIDTH - 1) continue;

                uint8_t color = gb->ppu->tile_buffer[tile_id][tile_row & 0x7][flip_x ? 7 - tile_column : tile_column];

                if (color != 0 && (!priority || priority && (gb->ppu->display_buffer[(sprite_x + tile_column) + (size_t)gb->ppu->ly * GAMEBOY_DISPLAY_WIDTH] == 0))) {
                    uint8_t offset = color * 2;
                    uint8_t bits = 0b00000011 << offset;
                    uint8_t palette = (sprite_a >> 4) & 1 ? gb->ppu->obp1 : gb->ppu->obp0;
                    color = (palette & bits) >> offset;
                    gb->ppu->display_buffer[sprite_x + tile_column + (size_t)gb->ppu->ly * GAMEBOY_DISPLAY_WIDTH] = color;
                }
            }
        }
    }
}

void ppu_cycle(GameBoy * const gb) {
    if (!(gb->ppu->lcdc & PPU_LCDC_LCD_ENABLE_BIT)) return;

    gb->ppu->count--;
    if (gb->ppu->count == 1 && (gb->ppu->stat & PPU_STAT_MODE_BITS) == GRAPHICS_MODE_DATA_TRANSFER) {
        if (gb->ppu->stat & PPU_STAT_HBLANK_CHECK_ENABLE) gb->interrupt_controller->flags |= LCD_STAT_INTERRUPT_BIT;
    }

    if (gb->ppu->count != 0) return;

    switch (gb->ppu->stat & PPU_STAT_MODE_BITS) {
        case GRAPHICS_MODE_HBLANK: {
            if (++gb->ppu->ly < 144) ppu_oam_search_enter(gb);
            else ppu_vblank_enter(gb);
            ppu_compare_ly_lyc(gb);
        } break;

        case GRAPHICS_MODE_VBLANK: {
            if (++gb->ppu->ly > 153) {
                gb->ppu->ly = 0;
                ppu_oam_search_enter(gb);
            }
            else gb->ppu->count += PPU_VBLANK_LENGTH;
            ppu_compare_ly_lyc(gb);
        } break;

        case GRAPHICS_MODE_OAM_SEARCH: {
            ppu_data_transfer_enter(gb);
        } break;

        case GRAPHICS_MODE_DATA_TRANSFER: {
            ppu_draw_line(gb);
            ppu_hblank_enter(gb);
        } break;
    }
}

uint8_t ppu_read_lcdc(GameBoy const * const gb) { return gb->ppu->lcdc; }

void ppu_write_lcdc(GameBoy * const gb, uint8_t value) {
    if (!(value & PPU_LCDC_LCD_ENABLE_BIT)) {
        gb->ppu->ly = 0;
        gb->ppu->count = 115;
        gb->ppu->stat = gb->ppu->stat & 0b11111100;
    }
    gb->ppu->lcdc = value;
}

uint8_t ppu_read_stat(GameBoy const * const gb) { return gb->ppu->stat | PPU_STAT_UNUSED_BITS; }

void ppu_write_stat(GameBoy * const gb, uint8_t value) { 
    gb->ppu->stat &= ~PPU_STAT_WRITE_BITS;
    gb->ppu->stat |= value & PPU_STAT_WRITE_BITS;
}

uint8_t ppu_read_oam(GameBoy const * const gb, uint16_t address) {
    return gb->ppu->oam[address];
}

void ppu_write_oam(GameBoy * const gb, uint16_t address, uint8_t value) {
    if ((gb->ppu->stat & PPU_STAT_MODE_BITS) == GRAPHICS_MODE_DATA_TRANSFER) return;
    if ((gb->ppu->stat & PPU_STAT_MODE_BITS) == GRAPHICS_MODE_OAM_SEARCH) return;
    gb->ppu->oam[address] = value;
}

uint8_t ppu_read_vram(GameBoy const * const gb, uint16_t address) { return gb->ppu->vram[address]; }

void ppu_write_vram(GameBoy * const gb, uint16_t address, uint8_t value) {
    gb->ppu->vram[address] = value;
    if (address < 0x1800) {
        uint8_t byte1 = gb->ppu->vram[(address & 0xFFFE)];
        uint8_t byte2 = gb->ppu->vram[(address & 0xFFFE) + 1];

        size_t tile = address / PPU_BYTES_PER_TILE;
        size_t row = (address % PPU_BYTES_PER_TILE) / PPU_BYTES_PER_ROW;

        for (size_t pixel = 0; pixel < PPU_PIXELS_PER_TILE_ROW; pixel++) {
            uint8_t mask = 1 << (7 - pixel);
            uint8_t bit1 = byte1 & mask;
            uint8_t bit2 = byte2 & mask;

            uint8_t color_code;
            if (bit1) color_code = bit2 ? 3 : 1;
            else color_code = bit2 ? 2 : 0;

            gb->ppu->tile_buffer[tile][row][pixel] = color_code;
        }
    }
}

GraphicsMode ppu_get_mode(GameBoy const* const gb) {
    return gb->ppu->stat & PPU_STAT_MODE_BITS;
}

size_t ppu_get_background_data(GameBoy const * const gb, uint8_t data[], size_t length) {
    for (size_t tile = 0; tile < PPU_BG_TILE_COUNT; tile++) {
        for (size_t row = 0; row < PPU_ROWS_PER_TILE; row++) {
            size_t y = tile / PPU_BG_WIDTH_IN_TILES * PPU_PIXELS_PER_TILE_ROW + row;
            for (size_t pixel = 0; pixel < PPU_PIXELS_PER_TILE_ROW; pixel++) {
                size_t x = (tile % PPU_BG_WIDTH_IN_TILES) * PPU_PIXELS_PER_TILE_ROW + pixel;

                uint_fast16_t tile_id = gb->ppu->vram[PPU_BACKGROUND1_START + tile];
                tile_id = (gb->ppu->lcdc & PPU_LCDC_BG_WINDOW_MODE_BIT) ? tile_id : tile_id + (256 * (uint_fast16_t)(tile_id < 128));

                data[x + y * PPU_BG_WIDTH_IN_PIXELS] = gb->ppu->tile_buffer[tile_id][row][pixel];
                if (x + y * PPU_BG_WIDTH_IN_PIXELS == length) return length;
            }
        }
    }
    return PPU_BG_WIDTH_IN_PIXELS * PPU_BG_HEIGHT_IN_PIXELS;
}

size_t ppu_get_display_data(GameBoy const * const gb, uint8_t data[], size_t length) {
    for (size_t x = 0; x < PPU_DISPLAY_WIDTH; x++) {
        for (size_t y = 0; y < PPU_DISPLAY_HEIGHT; x++) {
            if (gb->ppu->lcdc & PPU_LCDC_LCD_ENABLE_BIT) {
                data[x + y * PPU_DISPLAY_WIDTH] = gb->ppu->display_buffer[x + y * PPU_DISPLAY_WIDTH];
            }
            else data[x + y * PPU_DISPLAY_WIDTH] = PPU_LCD_COLOR_CODE;
            if (x + y * PPU_DISPLAY_WIDTH == length) return length;
        }
    }
    return PPU_DISPLAY_WIDTH * PPU_DISPLAY_HEIGHT;
}

size_t ppu_get_tileset_data(GameBoy const* const gb, uint8_t data[], size_t length) {
    for (size_t tile = 0; tile < PPU_TS_TILE_COUNT; tile++) {
        for (size_t row = 0; row < PPU_ROWS_PER_TILE; row++) {
            size_t y = tile / PPU_TS_WIDTH_IN_TILES * PPU_PIXELS_PER_TILE_ROW + row;
            for (size_t pixel = 0; pixel < PPU_PIXELS_PER_TILE_ROW; pixel++) {
                size_t x = (tile % PPU_TS_WIDTH_IN_TILES) * PPU_PIXELS_PER_TILE_ROW + pixel;

                data[x + y * PPU_TS_WIDTH_IN_PIXELS] = gb->ppu->tile_buffer[tile][row][pixel];
                if (x + y * PPU_TS_WIDTH_IN_PIXELS == length) return length;
            }
        }
    }
    return PPU_TS_WIDTH_IN_PIXELS * PPU_TS_HEIGHT_IN_PIXELS;
}
