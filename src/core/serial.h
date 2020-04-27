#ifndef TRTLE_SERIAL_H
#define TRTLE_SERIAL_H

typedef struct GameBoy GameBoy;

typedef struct Serial {
    uint8_t sb;
    uint8_t sc;
} Serial;

Serial * serial_create();
void serial_delete(Serial ** const s);
void serial_initialize(Serial * const s, bool skip_bootrom);

uint8_t serial_read_sc(GameBoy const * const gb);

#endif /* !TRTLE_SERIAL_H */
