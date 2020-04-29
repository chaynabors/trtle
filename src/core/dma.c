#include "pch.h"
#include "dma.h"

#include "gameboy.h"
#include "processor.h"

void dma_initialize(DMA * const dma, bool skip_bootrom) {
    dma->dma = 0xCC; // TODO: Make random
    dma->queue = -1;
    dma->current = 0x00;
    dma->delay = false;
    dma->active = false;
}

void dma_cycle(GameBoy * const gb) {
    if (gb->dma->queue != -1) {
        if (!gb->dma->delay) {
            gb->dma->dma = gb->dma->queue & 0xFF;
            gb->dma->queue = -1;
            gb->dma->current = 0x00;
            gb->dma->active = true;
        }
        else gb->dma->delay = false;
    }

    if (gb->dma->active && gb->dma->current < 0xA0) {
        uint8_t value;
        uint16_t address = (gb->dma->dma << 8) | gb->dma->current;
        if (address <= 0x7FFF) value = cartridge_read_rom(gb, address);
        else if (address <= 0x9FFF) value = ppu_read_vram(gb, address - 0x8000);
        else if (address <= 0xBFFF) value = cartridge_read_ram(gb, address - 0xA000);
        else if (address <= 0xDFFF) value = gb->processor->ram[address - 0xC000];
        else value = gb->processor->ram[address - 0xE000];
        gameboy_write(gb, GAMEBOY_OAM_ADDRESS | gb->dma->current, value);
        gb->dma->current++;
    }
    else gb->dma->active = false;
}

void dma_write_dma(GameBoy * const gb, uint8_t value) {
    gb->dma->queue = value;
    gb->dma->delay = true;
}

uint8_t dma_read_external_rom(GameBoy const * const gb, uint16_t address) {
    if (gb->dma->active) return cartridge_read_rom(gb, (gb->dma->dma << 8) | gb->dma->current);
    else return cartridge_read_rom(gb, address);
}

uint8_t dma_read_external_ram(GameBoy const * const gb, uint16_t address) {
    if (gb->dma->active) return cartridge_read_ram(gb, (gb->dma->dma << 8) | gb->dma->current);
    else return cartridge_read_ram(gb, address);
}

uint8_t dma_read_oam(GameBoy const * const gb, uint16_t address) {
    if (gb->dma->active) return 0xFF;
    else return ppu_read_oam(gb, address);
}

uint8_t dma_read_vram(GameBoy const * const gb, uint16_t address) {
    if (gb->dma->active) return ppu_read_vram(gb, (gb->dma->dma << 8) | gb->dma->current);
    else return ppu_read_vram(gb, address);
}
