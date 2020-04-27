#include "pch.h"
#include "gameboy.h"

#define UNMAPPED_ALL_ONES (0b11111111)

static uint8_t dmg_boot[] = {
    0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
    0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
    0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
    0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
    0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
    0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
    0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
    0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
    0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
    0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
    0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
    0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50
};

GameBoy * gameboy_create() {
    GameBoy * gb = malloc(sizeof(GameBoy));
    gb->cartridge = NULL;
    gb->dma = dma_create();
    gb->interrupt_controller = interrupt_controller_create();
    gb->joypad = joypad_create();
    gb->ppu = ppu_create();
    gb->processor = processor_create();
    gb->serial = serial_create();
    gb->sound_controller = sound_controller_create();
    gb->timer = timer_create();
    gb->boot = false;
    return gb;
}

void gameboy_delete(GameBoy ** gb) { 
    if (gb != NULL) {
        dma_delete(&(*gb)->dma);
        interrupt_controller_delete(&(*gb)->interrupt_controller);
        joypad_delete(&(*gb)->joypad);
        ppu_delete(&(*gb)->ppu);
        processor_delete(&(*gb)->processor);
        serial_delete(&(*gb)->serial);
        sound_controller_delete(&(*gb)->sound_controller);
        timer_delete(&(*gb)->timer);
        free(gb);
        gb = NULL;
    }
};

void gameboy_initialize(GameBoy* const gb, bool skip_bootrom) {
    dma_initialize(gb->dma, skip_bootrom);
    interrupt_controller_initialize(gb->interrupt_controller, skip_bootrom);
    joypad_initialize(gb->joypad, skip_bootrom);
    ppu_initialize(gb->ppu, skip_bootrom);
    processor_initialize(gb->processor, skip_bootrom);
    serial_initialize(gb->serial, skip_bootrom);
    sound_controller_initialize(gb->sound_controller, skip_bootrom);
    timer_initialize(gb->timer, skip_bootrom);
    gb->boot = skip_bootrom;
}

void gameboy_set_cartridge(GameBoy * const gb, Cartridge * const cart) {
    gb->cartridge = cart;
}

void gameboy_update(GameBoy * const gb, GameBoyInput input) {
    if (gb == NULL) {
        TRTLE_LOG_ERR("Attempted to pass a null argument into gameboy_update");
        return;
    }

    joypad_update_p1(gb, input);
    processor_process_instruction(gb);
}

void gameboy_update_to_vblank(GameBoy* const gb, GameBoyInput input) {
    if (gb == NULL) {
        TRTLE_LOG_ERR("Attempted to pass a null argument into gameboy_update_to_vblank");
        return;
    }

    while (ppu_get_mode(gb) == GRAPHICS_MODE_VBLANK) {
        gameboy_update(gb, input);
    }
    while (ppu_get_mode(gb) != GRAPHICS_MODE_VBLANK) {
        gameboy_update(gb, input);
    }
}

size_t gameboy_get_background_data(GameBoy const * const gb, uint8_t data[], size_t length) {
    if (gb == NULL || data == NULL) {
        TRTLE_LOG_ERR("Null argument received while fetching background data");
        return 0;
    }
    if (length == 0) return 0;
    return ppu_get_background_data(gb, data, length);
}

size_t gameboy_get_display_data(GameBoy const * const gb, uint8_t data[], size_t length) {
    if (gb == NULL || data == NULL) {
        TRTLE_LOG_ERR("Null argument received while fetching display data");
        return 0;
    }
    if (length == 0) return 0;
    return ppu_get_display_data(gb, data, length);
}

size_t gameboy_get_tileset_data(GameBoy const * const gb, uint8_t data[], size_t length) {
    if (gb == NULL || data == NULL) {
        TRTLE_LOG_ERR("Null argument received while fetching tileset data");
        return 0;
    }
    if (length == 0) return 0;
    return ppu_get_tileset_data(gb, data, length);
}

void gameboy_cycle(GameBoy * const gb) { 
    dma_cycle(gb);
    timer_cycle(gb);
    ppu_cycle(gb);
}

uint8_t gameboy_read(GameBoy * const gb, uint16_t address) {
    if      (address <= 0x00FF && !gb->boot) return dmg_boot[address];
    else if (address <= 0x7FFF) return dma_read_external_rom(gb, address);
    else if (address <= 0x9FFF) return dma_read_vram(gb, address - 0x8000);
    else if (address <= 0xBFFF) return dma_read_external_ram(gb, address);
    else if (address <= 0xDFFF) return gb->processor->ram[address - 0xC000];
    else if (address <= 0xFDFF) return gb->processor->ram[address - 0xE000];      // Read from ECHO
    else if (address <= 0xFE9F) return dma_read_oam(gb, address - 0xFE00); // Read from OAM
    else if (address <= 0xFEFF) return 0x00;
    else if (address == 0xFF00) return joypad_read_p1(gb);
    else if (address == 0xFF01) return gb->serial->sb;
    else if (address == 0xFF02) return serial_read_sc(gb);
    else if (address == 0xFF03) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF04) return gb->timer->div;
    else if (address == 0xFF05) return gb->timer->tima;
    else if (address == 0xFF06) return gb->timer->tma;
    else if (address == 0xFF07) return timer_read_tac(gb);
    else if (address == 0xFF08) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF09) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF0A) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF0B) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF0C) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF0D) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF0E) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF0F) return interrupt_controller_get_flags(gb);
    else if (address == 0xFF10) return sound_controller_read_nr10(gb);
    else if (address == 0xFF11) return gb->sound_controller->nr11;
    else if (address == 0xFF12) return gb->sound_controller->nr12;
    else if (address == 0xFF13) return gb->sound_controller->nr13;
    else if (address == 0xFF14) return gb->sound_controller->nr14;
    else if (address == 0xFF15) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF16) return gb->sound_controller->nr21;
    else if (address == 0xFF17) return gb->sound_controller->nr22;
    else if (address == 0xFF18) return gb->sound_controller->nr23;
    else if (address == 0xFF19) return gb->sound_controller->nr24;
    else if (address == 0xFF1A) return sound_controller_read_nr30(gb);
    else if (address == 0xFF1B) return gb->sound_controller->nr31;
    else if (address == 0xFF1C) return sound_controller_read_nr32(gb);
    else if (address == 0xFF1D) return gb->sound_controller->nr33;
    else if (address == 0xFF1E) return gb->sound_controller->nr34;
    else if (address == 0xFF1F) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF20) return sound_controller_read_nr41(gb);
    else if (address == 0xFF21) return gb->sound_controller->nr42;
    else if (address == 0xFF22) return gb->sound_controller->nr43;
    else if (address == 0xFF23) return sound_controller_read_nr44(gb);
    else if (address == 0xFF24) return gb->sound_controller->nr50;
    else if (address == 0xFF25) return gb->sound_controller->nr51;
    else if (address == 0xFF26) return sound_controller_read_nr52(gb);
    else if (address == 0xFF27) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF28) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF29) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF40) return ppu_read_lcdc(gb);
    else if (address == 0xFF41) return ppu_read_stat(gb);
    else if (address == 0xFF42) return gb->ppu->scy;
    else if (address == 0xFF43) return gb->ppu->scx;
    else if (address == 0xFF44) return gb->ppu->ly;
    else if (address == 0xFF45) return gb->ppu->lyc;
    else if (address == 0xFF46) return gb->dma->dma;
    else if (address == 0xFF47) return gb->ppu->bgp;
    else if (address == 0xFF48) return gb->ppu->obp0;
    else if (address == 0xFF49) return gb->ppu->obp1;
    else if (address == 0xFF4A) return gb->ppu->wy;
    else if (address == 0xFF4B) return gb->ppu->wx;
    else if (address == 0xFF4C) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF4D) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF4E) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF4F) return UNMAPPED_ALL_ONES;
    else if (address == 0xFF50) return gb->boot | 0b11111110; 
    else if (address >= 0xFF51 && address <= 0xFF7F) return UNMAPPED_ALL_ONES;
    else if (address >= 0xFF80 && address <= 0xFFFE) return gb->processor->hram[address - 0xFF80];
    else if (address == 0xFFFF) return interrupt_controller_get_enables(gb);
    else TRTLE_LOG_ERR("Attempted to read an unsupported address %X\n", address);
    return 0xFF;
}

void gameboy_write(GameBoy * const gb, uint16_t address, uint8_t value) {
    if      (address <= 0x7FFF) cartridge_write_rom(gb, address, value);
    else if (address <= 0x9FFF) ppu_write_vram(gb, address - 0x8000, value);
    else if (address <= 0xBFFF) cartridge_write_ram(gb, address, value - 0xA000);
    else if (address <= 0xDFFF) gb->processor->ram[address - 0xC000] = value;
    else if (address <= 0xFDFF) gb->processor->ram[address - 0xE000] = value;
    else if (address <= 0xFE9F) ppu_write_oam(gb, address - 0xFE00, value);
    else if (address == 0xFF00) joypad_write_p1(gb, value);
    else if (address == 0xFF01) gb->serial->sb = value;
    else if (address == 0xFF02) gb->serial->sc = value;
    else if (address == 0xFF03) return; // Unmapped
    else if (address == 0xFF04) timer_write_div(gb);
    else if (address == 0xFF05) timer_write_tima(gb, value);
    else if (address == 0xFF06) timer_write_tma(gb, value);
    else if (address == 0xFF07) timer_write_tac(gb, value);
    else if (address == 0xFF08) return; // Unmapped
    else if (address == 0xFF09) return; // Unmapped
    else if (address == 0xFF0A) return; // Unmapped
    else if (address == 0xFF0B) return; // Unmapped
    else if (address == 0xFF0C) return; // Unmapped
    else if (address == 0xFF0D) return; // Unmapped
    else if (address == 0xFF0E) return; // Unmapped
    else if (address == 0xFF0F) interrupt_controller_set_flags(gb, value);
    else if (address == 0xFF10) gb->sound_controller->nr10 = value;
    else if (address == 0xFF11) gb->sound_controller->nr11 = value;
    else if (address == 0xFF12) gb->sound_controller->nr12 = value;
    else if (address == 0xFF13) gb->sound_controller->nr13 = value;
    else if (address == 0xFF14) gb->sound_controller->nr14 = value;
    else if (address == 0xFF15) return; // Unmapped
    else if (address == 0xFF16) gb->sound_controller->nr21 = value;
    else if (address == 0xFF17) gb->sound_controller->nr22 = value;
    else if (address == 0xFF18) gb->sound_controller->nr23 = value;
    else if (address == 0xFF19) gb->sound_controller->nr24 = value;
    else if (address == 0xFF1A) gb->sound_controller->nr30 = value;
    else if (address == 0xFF1B) gb->sound_controller->nr31 = value;
    else if (address == 0xFF1C) gb->sound_controller->nr32 = value;
    else if (address == 0xFF1D) gb->sound_controller->nr33 = value;
    else if (address == 0xFF1E) gb->sound_controller->nr34 = value;
    else if (address == 0xFF1F) return; // Unmapped
    else if (address == 0xFF20) gb->sound_controller->nr41 = value;
    else if (address == 0xFF21) gb->sound_controller->nr42 = value;
    else if (address == 0xFF22) gb->sound_controller->nr43 = value;
    else if (address == 0xFF23) gb->sound_controller->nr44 = value;
    else if (address == 0xFF24) gb->sound_controller->nr50 = value;
    else if (address == 0xFF25) gb->sound_controller->nr51 = value;
    else if (address == 0xFF26) gb->sound_controller->nr52 = value;
    else if (address == 0xFF27) return; // Unmapped
    else if (address == 0xFF28) return; // Unmapped
    else if (address == 0xFF29) return; // Unmapped
    else if (address == 0xFF40) ppu_write_lcdc(gb, value);
    else if (address == 0xFF41) ppu_write_stat(gb, value);
    else if (address == 0xFF42) gb->ppu->scy = value;
    else if (address == 0xFF43) gb->ppu->scx = value;
    else if (address == 0xFF44) gb->ppu->ly = value;
    else if (address == 0xFF45) gb->ppu->lyc = value;
    else if (address == 0xFF46) dma_write_dma(gb, value);
    else if (address == 0xFF47) gb->ppu->bgp = value;
    else if (address == 0xFF48) gb->ppu->obp0 = value;
    else if (address == 0xFF49) gb->ppu->obp1 = value;
    else if (address == 0xFF4A) gb->ppu->wy = value;
    else if (address == 0xFF4B) gb->ppu->wx = value;
    else if (address >= 0xFF4C && address <= 0xFF4F) return; // Unmapped
    else if (address == 0xFF50) { if (gb->boot == 0) gb->boot = value == 0 ? 0 : 1; }
    else if (address >= 0xFF51 && address <= 0xFF7F) return; // Unmapped
    else if (address >= 0xFF80 && address <= 0xFFFE) gb->processor->hram[address - 0xFF80] = value;
    else if (address == 0xFFFF) interrupt_controller_set_enables(gb, value);
    else TRTLE_LOG_ERR("Attempted to write to an unsupported address %X\n", address);
}
