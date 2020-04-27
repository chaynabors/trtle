#ifndef TRTLE_SERIAL_H
#define TRTLE_SERIAL_H

typedef struct GameBoy GameBoy;

typedef struct Serial {
    uint8_t sb;
    uint8_t sc;
} Serial;

void serial_initialize(GameBoy * const gb);

uint8_t serial_read_sc(GameBoy const * const gb);

#endif /* !TRTLE_SERIAL_H */
