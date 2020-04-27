#ifndef TRTLE_TIMER_H
#define TRTLE_TIMER_H

#define LITTLE_ENDIAN

typedef struct GameBoy GameBoy;

typedef struct Timer {
    union {
        uint16_t internal_counter;
        struct {
#ifdef LITTLE_ENDIAN
            uint8_t __ignore;
            uint8_t div;
#elif BIG_ENDIAN
            uint8_t div;
            uint8_t __ignore;
#endif
        };
    };
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
    bool tima_overflow;
    bool writing_tima;
} Timer;

Timer * timer_create();
void timer_delete(Timer ** const t);
void timer_initialize(Timer * const t, bool skip_bootrom);

void timer_cycle(GameBoy * const gb);

void timer_write_div(GameBoy * const gb);

void timer_write_tima(GameBoy * const gb, uint8_t value);

void timer_write_tma(GameBoy * const gb, uint8_t value);

uint8_t timer_read_tac(GameBoy const * const gb);
void timer_write_tac(GameBoy * const gb, uint8_t value);

#endif /* !TRTLE_TIMER_H */
