#ifndef TRTLE_DMA_H
#define TRTLE_DMA_H

typedef struct GameBoy GameBoy;

typedef struct DMA {
    uint8_t dma;
    int32_t queue;
    uint8_t current;
    bool delay;
    bool active;
} DMA;

void dma_initialize(DMA * const dma, bool skip_bootrom);

void dma_cycle(GameBoy * const gb);

void dma_write_dma(GameBoy * const gb, uint8_t value);

uint8_t dma_read_external_rom(GameBoy const * const gb, uint16_t address);

uint8_t dma_read_external_ram(GameBoy const * const gb, uint16_t address);

uint8_t dma_read_oam(GameBoy const * const gb, uint16_t address);

uint8_t dma_read_vram(GameBoy const * const gb, uint16_t address);

#endif /* !TRTLE_DMA_H */
