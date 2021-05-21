#ifndef TRTLE_PROCESSOR_H
#define TRTLE_PROCESSOR_H

#include <stdbool.h>
#include <stdint.h>

#define PROCESSOR_CLOCK_SPEED (4194304)

typedef struct GameBoy GameBoy;

typedef struct Processor {
    union {
        uint16_t af;
        struct {
#ifdef __ORDER_LITTLE_ENDIAN__
            uint8_t f;
            uint8_t a;
#else
            uint8_t a;
            uint8_t f;
#endif
        };
    };
    union {
        uint16_t bc;
        struct {
#ifdef __ORDER_LITTLE_ENDIAN__
            uint8_t c;
            uint8_t b;
#else
            uint8_t b;
            uint8_t c;
#endif
        };
    };
    union {
        uint16_t de;
        struct {
#ifdef __ORDER_LITTLE_ENDIAN__
            uint8_t e;
            uint8_t d;
#else
            uint8_t d;
            uint8_t e;
#endif
        };
    };
    union {
        uint16_t hl;
        struct {
#ifdef __ORDER_LITTLE_ENDIAN__
            uint8_t l;
            uint8_t h;
#else
            uint8_t h;
            uint8_t l;
#endif
        };
    };
    uint16_t sp;
    uint16_t pc;
    uint8_t ram[8192];
    uint8_t hram[0x7F];
    bool halt_mode;
    bool skip_pc_increment;
    bool skip_next_interrupt;
} Processor;

void processor_initialize(Processor * const p, bool skip_bootrom);

void processor_process_instruction(GameBoy * const gb);

#endif /* !TRTLE_PROCESSOR_H */
