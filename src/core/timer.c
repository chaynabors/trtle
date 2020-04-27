#include "pch.h"
#include "timer.h"

#include "interrupt_controller.h"
#include "processor.h"
#include "gameboy.h"

#define TIMER_TAC_MASK          (0b11111000)
#define TIMER_CLOCK_SELECT_BITS (0b00000011)
#define TIMER_START_BIT         (0b00000100)

Timer * timer_create() {
    return calloc(1, sizeof(Timer));
}

void timer_delete(Timer ** const t) {
    free(*t);
    *t = NULL;
}

void timer_initialize(Timer * const t, bool skip_bootrom) {
    if (skip_bootrom) t->internal_counter = 0xABCC;
    else t->internal_counter = 0x0000;
    t->tima = 0x00;
    t->tma = 0x00;
    t->tac = 0x00;
    t->tima_overflow = false;
    t->writing_tima = false;
}

static uint8_t timer_get_frequency_bit(GameBoy * const gb) {
    switch (gb->timer->tac & TIMER_CLOCK_SELECT_BITS) {
        case 0b00: return (gb->timer->internal_counter & (1 << 9)) >> 9;
        case 0b01: return (gb->timer->internal_counter & (1 << 3)) >> 3;
        case 0b10: return (gb->timer->internal_counter & (1 << 5)) >> 5;
        case 0b11: return (gb->timer->internal_counter & (1 << 7)) >> 7;
    }

    abort(); // TODO: remove this nasty thing
    return 0xFF;
}

static void timer_increment_tima(GameBoy * const gb) {
    gb->timer->tima++;
    if (gb->timer->tima == 0) gb->timer->tima_overflow = true;
}

void timer_cycle(GameBoy * const gb) {
    gb->timer->writing_tima = false;

    if (gb->timer->tima_overflow) {
        gb->timer->tima_overflow = false;
        gb->interrupt_controller->flags |= TIMER_INTERRUPT_BIT;
        gb->timer->tima = gb->timer->tma;
        gb->timer->writing_tima = true;
    }

    bool old_bit = timer_get_frequency_bit(gb);
    gb->timer->internal_counter += 4;
    bool new_bit = timer_get_frequency_bit(gb);

    if ((gb->timer->tac & TIMER_START_BIT) && old_bit && !new_bit) timer_increment_tima(gb);
}

void timer_write_div(GameBoy * const gb) {
    bool old_bit = timer_get_frequency_bit(gb);
    gb->timer->internal_counter = 0;
    bool new_bit = timer_get_frequency_bit(gb);

    if ((gb->timer->tac & TIMER_START_BIT) && old_bit && !new_bit) timer_increment_tima(gb);
}

void timer_write_tima(GameBoy * const gb, uint8_t value) {
    if (!gb->timer->writing_tima) {
        gb->timer->tima = value;
        gb->timer->tima_overflow = false;
    }
}

void timer_write_tma(GameBoy * const gb, uint8_t value) {
    gb->timer->tma = value;
    if (gb->timer->writing_tima) gb->timer->tima = value;
}

uint8_t timer_read_tac(GameBoy const * const gb) {
    return gb->timer->tac | TIMER_TAC_MASK;
}

void timer_write_tac(GameBoy * const gb, uint8_t value) {
    bool old_bit = timer_get_frequency_bit(gb) && gb->timer->tac & TIMER_START_BIT;
    gb->timer->tac = value | TIMER_TAC_MASK;
    bool new_bit = timer_get_frequency_bit(gb) && gb->timer->tac & TIMER_START_BIT;

    if (old_bit && !new_bit) timer_increment_tima(gb);
}
