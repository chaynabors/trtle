#ifndef TRTLE_INTERRUPT_CONTROLLER_H
#define TRTLE_INTERRUPT_CONTROLLER_H

#define VBLANK_INTERRUPT_BIT     (0b00000001)
#define LCD_STAT_INTERRUPT_BIT   (0b00000010)
#define TIMER_INTERRUPT_BIT      (0b00000100)
#define SERIAL_INTERRUPT_BIT     (0b00001000)
#define JOYPAD_INTERRUPT_BIT     (0b00010000)

typedef struct GameBoy GameBoy;

typedef struct InterruptController {
    uint8_t enables;
    uint8_t flags;
    uint8_t ime;
    int8_t cycles_until_ime;
} InterruptController;

void interrupt_controller_initialize(GameBoy * const gb, bool skip_bootrom);

uint8_t interrupt_controller_get_enables(GameBoy const * const gb);
void interrupt_controller_set_enables(GameBoy * const gb, uint8_t value);

uint8_t interrupt_controller_get_flags(GameBoy const * const gb);
void interrupt_controller_set_flags(GameBoy * const gb, uint8_t value);

#endif /* !TRTLE_INTERRUPT_CONTROLLER_H */
