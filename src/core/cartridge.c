#include "pch.h"
#include "cartridge.h"

#include "gameboy.h"

#define MBC_CARTRIDGE_TYPE_ADDRESS (0x0147)
#define MBC_ROM_SIZE_ADDRESS       (0x0148)
#define MBC_RAM_SIZE_ADDRESS       (0x0149)
#define ROM_BANK_SIZE       (0x4000)
#define RAM_BANK_SIZE       (0x2000)

#define RAMG_ENABLE        (0b00001010)

#define MBC1_RAMG_MASK     (0b00001111)
#define MBC1_ROMB0_MASK    (0b00011111)
#define MBC1_ROMB1_MASK    (0b00000011)

#define MBC2_RAMG_MASK     (0b00001111)
#define MBC2_ROMB0_MASK    (0b00001111)

#define MBC5_ROMB1_MASK    (0b00000001)
#define MBC5_RAMB_MASK     (0b00001111)

static CartridgeError cartridge_setup_rom(Cartridge * const cart, char const * const path) {
    FILE * file = fopen(path, "rb");
    if (file == NULL) return CARTRIDGE_ERROR_FILE_NOT_FOUND;

    fseek(file, 0, SEEK_END);
    size_t buffer_length = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t * buffer = calloc(1, buffer_length);
    if (buffer == NULL) {
        fclose(file);
        return CARTRIDGE_ERROR_ROM_ALLOCATION_FAILED;
    }

    fread(buffer, 1, buffer_length, file);
    cart->rom = buffer;
    cart->rom_size = buffer_length;

    fclose(file);

    size_t rom_size = cart->rom[MBC_ROM_SIZE_ADDRESS];
    switch (rom_size) {
        case 0x00: rom_size = ROM_BANK_SIZE * 2; break;
        case 0x01: rom_size = ROM_BANK_SIZE * 4; break;
        case 0x02: rom_size = ROM_BANK_SIZE * 8; break;
        case 0x03: rom_size = ROM_BANK_SIZE * 16; break;
        case 0x04: rom_size = ROM_BANK_SIZE * 32; break;
        case 0x05: rom_size = ROM_BANK_SIZE * 64; break;
        case 0x06: rom_size = ROM_BANK_SIZE * 128; break;
        case 0x07: rom_size = ROM_BANK_SIZE * 256; break;
        case 0x08: rom_size = ROM_BANK_SIZE * 512; break;
        case 0x52: rom_size = ROM_BANK_SIZE * 72; break;
        case 0x53: rom_size = ROM_BANK_SIZE * 80; break;
        case 0x54: rom_size = ROM_BANK_SIZE * 96; break;
        default: TRTLE_LOG_ERR("Attempted to initialize a cart with an unknown ROM size: %zX\n", rom_size); break;
    }
    if (cart->rom_size != rom_size) {
        TRTLE_LOG_WARN("Cartridge size mismatch (Expected %zX bytes, Allocated %zX bytes)", rom_size, cart->rom_size);
    }

    return CARTRIDGE_ERROR_NONE;
}

static CartridgeError cartridge_setup_ram(Cartridge* const cart) {
    uint8_t ram_size = cart->rom[MBC_RAM_SIZE_ADDRESS];
    switch (ram_size) {
        case 0x00: cart->ram_size = 0; break;
        case 0x01: cart->ram_size = RAM_BANK_SIZE / 4; break;
        case 0x02: cart->ram_size = RAM_BANK_SIZE; break;
        case 0x03: cart->ram_size = RAM_BANK_SIZE * 4; break;
        case 0x04: cart->ram_size = RAM_BANK_SIZE * 16; break;
        case 0x05: cart->ram_size = RAM_BANK_SIZE * 8; break;
        default: TRTLE_LOG_ERR("Attempted to initialize a cart with an unknown RAM size: %X\n", ram_size); break;
    }

    // MBC2 declares itself to have a ram size of 0, so this is required
    if (cart->type == MBC_MBC2 || cart->type == MBC_MBC2_BATTERY) cart->ram_size = 0x200;

    if (cart->ram_size > 0) {
        cart->ram = calloc(1, cart->ram_size);
        if (cart->ram == NULL) return CARTRIDGE_ERROR_RAM_ALLOCATION_FAILED;
        memset(cart->ram, 0xFF, sizeof(uint8_t) * cart->ram_size);
    }

    return CARTRIDGE_ERROR_NONE;
}

CartridgeError cartridge_from_file(Cartridge ** return_cart, char const * const path) {
    if (return_cart == NULL) return CARTIRDGE_ERROR_RETURN_ARGUMENT_NULL;

    Cartridge * cart = calloc(1, sizeof(Cartridge));
    if (cart == NULL) return CARTRIDGE_ERROR_CARTRIDGE_ALLOCATION_FAILED;
    memset(cart, 0, sizeof(Cartridge));

    CartridgeError error = cartridge_setup_rom(cart, path);
    if (error) {
        free(cart);
        cart = NULL;
        return error;
    }

    cart->type = cart->rom[MBC_CARTRIDGE_TYPE_ADDRESS];
    switch (cart->type) {
        case MBC_NONE:
        case MBC_NONE_RAM:
        case MBC_NONE_RAM_BATTERY: {
            cart->romb0 = 0;
            cart->romb1 = 0;
            cart->ramb = 0;
            cart->ramg = false;
            cart->mode = false;
        } break;

        case MBC_MBC1:
        case MBC_MBC1_RAM:
        case MBC_MBC1_RAM_BATTERY: {
            cart->romb0 = 1;
            cart->romb1 = 0;
            cart->ramb = 0;
            cart->ramg = false;
            cart->mode = false;
        } break;

        case MBC_MBC2:
        case MBC_MBC2_BATTERY: {
            cart->romb0 = 1;
            cart->romb1 = 0;
            cart->ramb = 0;
            cart->ramg = false;
            cart->mode = false;
        } break;

        case MBC_MBC5:
        case MBC_MBC5_RAM:
        case MBC_MBC5_RAM_BATTERY:
        case MBC_MBC5_RUMBLE:
        case MBC_MBC5_RUMBLE_RAM:
        case MBC_MBC5_RUMBLE_RAM_BATTERY: {
            cart->romb0 = 1;
            cart->romb1 = 0;
            cart->ramb = 0;
            cart->ramg = false;
            cart->mode = false;
        } break;

        default: return CARTRIDGE_ERROR_MBC_NOT_SUPPORTED;
    }

    error = cartridge_setup_ram(cart);
    if (error) {
        free(cart);
        cart = NULL;
        return error;
    }

    *return_cart = cart;
    return CARTRIDGE_ERROR_NONE;
}

void cartridge_delete(Cartridge ** const cart) {
    if (*cart != NULL) {
        if ((*cart)->rom != NULL) {
            free((*cart)->rom);
            (*cart)->rom = NULL;
        }
        if ((*cart)->ram != NULL) {
            free((*cart)->ram);
            (*cart)->ram = NULL;
        }
        free(*cart);
        *cart = NULL;
    }
}

static uint8_t cartridge_read_low(GameBoy const * const gb, uint16_t address) {
    address &= 0x3FFF;
    size_t size = gb->cartridge->rom_size - 1; // For address wrap
    switch (gb->cartridge->type) {
        case MBC_NONE:
        case MBC_NONE_RAM:
        case MBC_NONE_RAM_BATTERY: {
            return gb->cartridge->rom[address & size];
        } break;

        case MBC_MBC1:
        case MBC_MBC1_RAM:
        case MBC_MBC1_RAM_BATTERY: {
            size_t rom_bank = gb->cartridge->mode ? ((size_t)gb->cartridge->romb1 << 5) : 0;
            return gb->cartridge->rom[((rom_bank * ROM_BANK_SIZE) | address) & size];
        } break;

        case MBC_MBC2:
        case MBC_MBC2_BATTERY: {
            return gb->cartridge->rom[address & size];
        } break;

        case MBC_MBC5:
        case MBC_MBC5_RAM:
        case MBC_MBC5_RAM_BATTERY:
        case MBC_MBC5_RUMBLE:
        case MBC_MBC5_RUMBLE_RAM:
        case MBC_MBC5_RUMBLE_RAM_BATTERY: {
            return gb->cartridge->rom[address & size];
        } break;

        default: {
            TRTLE_LOG_ERR("Attempted to read from ROM with a Non-supported MBC");
            return 0xFF;
        } break;
    }
    TRTLE_LOG_ERR("Switch leaked during low external ROM read");
}

static uint8_t cartridge_read_high(GameBoy const * const gb, uint16_t address) {
    address &= ROM_BANK_SIZE - 1;
    size_t size = gb->cartridge->rom_size - 1; // For address wrap
    switch (gb->cartridge->type) {
        case MBC_NONE:
        case MBC_NONE_RAM:
        case MBC_NONE_RAM_BATTERY: {
            return gb->cartridge->rom[((size_t)address + ROM_BANK_SIZE) & size];
        } break;

        case MBC_MBC1:
        case MBC_MBC1_RAM:
        case MBC_MBC1_RAM_BATTERY: {
            size_t rom_bank = (gb->cartridge->romb1 << 5) | gb->cartridge->romb0;
            return gb->cartridge->rom[((rom_bank * ROM_BANK_SIZE) | address) & size];
        } break;

        case MBC_MBC2:
        case MBC_MBC2_BATTERY: {
            return gb->cartridge->rom[((gb->cartridge->romb0 * ROM_BANK_SIZE) | address) & size];
        } break;

        case MBC_MBC5:
        case MBC_MBC5_RAM:
        case MBC_MBC5_RAM_BATTERY:
        case MBC_MBC5_RUMBLE:
        case MBC_MBC5_RUMBLE_RAM:
        case MBC_MBC5_RUMBLE_RAM_BATTERY: {
            size_t rom_bank = (gb->cartridge->romb1 << 8) | gb->cartridge->romb0;
            return gb->cartridge->rom[((rom_bank * ROM_BANK_SIZE) | address) & size];
        } break;

        default: {
            TRTLE_LOG_ERR("Attempted to read from ROM with a non-supported MBC");
            return 0xFF;
        } break;
    }
    TRTLE_LOG_ERR("Switch leaked during high external ROM read");
}

uint8_t cartridge_read_rom(GameBoy const * const gb, uint16_t address) {
    if (gb->cartridge != NULL) {
        if (address <= 0x3FFF) return cartridge_read_low(gb, address);
        else return cartridge_read_high(gb, address);
    }

    return 0xFF;
}

void cartridge_write_rom(GameBoy * const gb, uint16_t address, uint8_t value) {
    switch (gb->cartridge->type) {
        case MBC_NONE:
        case MBC_NONE_RAM:
        case MBC_NONE_RAM_BATTERY: {
            TRTLE_LOG_INFO("Attempted to write to ROM with no MBC\n");
        } break;

        case MBC_MBC1:
        case MBC_MBC1_RAM:
        case MBC_MBC1_RAM_BATTERY: {
            if (address <= 0x1FFF) gb->cartridge->ramg = (value & MBC1_RAMG_MASK) == RAMG_ENABLE;
            else if (address <= 0x3FFF) gb->cartridge->romb0 = (value & MBC1_ROMB0_MASK) == 0 ? 1 : (value & MBC1_ROMB0_MASK);
            else if (address <= 0x5FFF) gb->cartridge->romb1 = value & MBC1_ROMB1_MASK;
            else gb->cartridge->mode = value & 1;
        } break;

        case MBC_MBC2:
        case MBC_MBC2_BATTERY: {
            if (address <= 0x3FFF) {
                if (!((address >> 8) & 1)) gb->cartridge->ramg = (value & MBC2_RAMG_MASK) == RAMG_ENABLE;
                else gb->cartridge->romb0 = (value & MBC2_ROMB0_MASK) == 0 ? 1 : (value & MBC2_ROMB0_MASK);
            }
        } break;

        case MBC_MBC5:
        case MBC_MBC5_RAM:
        case MBC_MBC5_RAM_BATTERY:
        case MBC_MBC5_RUMBLE:
        case MBC_MBC5_RUMBLE_RAM:
        case MBC_MBC5_RUMBLE_RAM_BATTERY: {
            if (address <= 0x1FFF) gb->cartridge->ramg = value == RAMG_ENABLE;
            else if (address <= 0x2FFF) gb->cartridge->romb0 = value;
            else if (address <= 0x3FFF) gb->cartridge->romb1 = value & MBC5_ROMB1_MASK;
            else if (address <= 0x5FFF) gb->cartridge->ramb = value & MBC5_RAMB_MASK;
        } break;

        default: {
            TRTLE_LOG_ERR("Attempted to write to ROM with a non-supported MBC");
        } break;
    }
}

uint8_t cartridge_read_ram(GameBoy const * const gb, uint16_t address) {
    address &= RAM_BANK_SIZE - 1;
    size_t size = gb->cartridge->ram_size - 1; // For address wrap
    switch (gb->cartridge->type) {
        case MBC_NONE_RAM:
        case MBC_NONE_RAM_BATTERY: {
            TRTLE_LOG_WARN("Attempted to read from external RAM with no MBC, this needs to be tested"); // TODO:
        } break;

        case MBC_MBC1_RAM:
        case MBC_MBC1_RAM_BATTERY: {
            if (gb->cartridge->ramg) {
                size_t ram_bank = gb->cartridge->mode ? gb->cartridge->romb1 : 0;
                return gb->cartridge->ram[((ram_bank * RAM_BANK_SIZE) | address) & size];
            }
            else return 0xFF;
        } break;

        case MBC_MBC2:
        case MBC_MBC2_BATTERY: {
            if (gb->cartridge->ramg) return 0xF0 | (gb->cartridge->ram[address & size] & 0x0F);
            else return 0xFF;
        } break;

        case MBC_MBC5_RAM:
        case MBC_MBC5_RAM_BATTERY:
        case MBC_MBC5_RUMBLE_RAM:
        case MBC_MBC5_RUMBLE_RAM_BATTERY: {
            if (gb->cartridge->ramg) {
                return gb->cartridge->ram[((gb->cartridge->ramb * RAM_BANK_SIZE) | address) & size];
            }
            else return 0xFF;
        } break;

        case MBC_NONE:
        case MBC_MBC1:
        case MBC_MBC5:
        case MBC_MBC5_RUMBLE: {
            return 0xFF;
        } break;

        default: {
            TRTLE_LOG_ERR("Attempted to read from RAM with a non-supported MBC");
            return 0xFF;
        } break;
    }
    TRTLE_LOG_ERR("Switch leaked during external RAM read");
    return 0xFF;
}

void cartridge_write_ram(GameBoy * const gb, uint16_t address, uint8_t value) {
    address &= RAM_BANK_SIZE - 1;
    size_t size = gb->cartridge->ram_size - 1; // For address wrap
    switch (gb->cartridge->type) {
        case MBC_NONE_RAM:
        case MBC_NONE_RAM_BATTERY: {
            TRTLE_LOG_WARN("Attempted to write external RAM with no MBC, this needs to be tested\n"); // TODO:
        } break;

        case MBC_MBC1_RAM:
        case MBC_MBC1_RAM_BATTERY: {
            if (gb->cartridge->ramg) {
                size_t ram_bank = gb->cartridge->mode ? gb->cartridge->romb1 : 0;
                gb->cartridge->ram[((ram_bank * RAM_BANK_SIZE) | address) & size] = value;
            }
        } break;

        case MBC_MBC2:
        case MBC_MBC2_BATTERY: {
            if (gb->cartridge->ramg) {
                // MBC2 is 4 bit ram hence the bitwise and on the value
                gb->cartridge->ram[address & size] = value & 0x0F;
            }
        } break;

        case MBC_MBC5_RAM:
        case MBC_MBC5_RAM_BATTERY:
        case MBC_MBC5_RUMBLE_RAM:
        case MBC_MBC5_RUMBLE_RAM_BATTERY: {
            if (gb->cartridge->ramg) {
                gb->cartridge->ram[((gb->cartridge->ramb * RAM_BANK_SIZE) | address) & size] = value;
            }
        } break;

        case MBC_NONE:
        case MBC_MBC1:
        case MBC_MBC5:
        case MBC_MBC5_RUMBLE: {
            // Swallow
        } break;

        default: {
            TRTLE_LOG_ERR("Attempted to write RAM with a non-supported MBC\n");
        } break;
    }
}
