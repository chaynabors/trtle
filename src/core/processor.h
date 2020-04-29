#ifndef TRTLE_PROCESSOR_H
#define TRTLE_PROCESSOR_H

#define PROCESSOR_CLOCK_SPEED (4194304)
#define LITTLE_ENDIAN

typedef struct GameBoy GameBoy;

typedef struct Processor {
    union {
        uint16_t af;
        struct {
#ifdef LITTLE_ENDIAN
            uint8_t f;
            uint8_t a;
#elif BIG_ENDIAN
            uint8_t a;
            uint8_t f;
#endif
        };
    };
    union {
        uint16_t bc;
        struct {
#ifdef LITTLE_ENDIAN
            uint8_t c;
            uint8_t b;
#elif BIG_ENDIAN
            uint8_t b;
            uint8_t c;
#endif
        };
    };
    union {
        uint16_t de;
        struct {
#ifdef LITTLE_ENDIAN
            uint8_t e;
            uint8_t d;
#elif BIG_ENDIAN
            uint8_t d;
            uint8_t e;
#endif
        };
    };
    union {
        uint16_t hl;
        struct {
#ifdef LITTLE_ENDIAN
            uint8_t l;
            uint8_t h;
#elif BIG_ENDIAN
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
