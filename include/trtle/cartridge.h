#ifndef TRTLE_CARTRIDGE_H
#define TRTLE_CARTRIDGE_H

typedef struct GameBoy GameBoy;

typedef enum CartridgeError {
    CARTRIDGE_ERROR_NONE = 0,
    CARTIRDGE_ERROR_RETURN_ARGUMENT_NULL,
    CARTRIDGE_ERROR_FILE_NOT_FOUND,
    CARTRIDGE_ERROR_CARTRIDGE_ALLOCATION_FAILED,
    CARTRIDGE_ERROR_ROM_ALLOCATION_FAILED,
    CARTRIDGE_ERROR_RAM_ALLOCATION_FAILED,
    CARTRIDGE_ERROR_MBC_NOT_SUPPORTED
} CartridgeError;

#if defined(TRTLE_INTERNAL)
typedef enum MBC {
    MBC_NONE                           = 0x00,
    MBC_MBC1                           = 0x01,
    MBC_MBC1_RAM                       = 0x02,
    MBC_MBC1_RAM_BATTERY               = 0x03,
    MBC_MBC2                           = 0x05,
    MBC_MBC2_BATTERY                   = 0x06,
    MBC_NONE_RAM                       = 0x08,
    MBC_NONE_RAM_BATTERY               = 0x09,
    MBC_MMM01                          = 0x0B,
    MBC_MMM01_RAM                      = 0x0C,
    MBC_MMM01_RAM_BATTERY              = 0x0D,
    MBC_MBC3_TIMER_BATTERY             = 0x0F,
    MBC_MBC3_TIMER_RAM_BATTERY         = 0x10,
    MBC_MBC3                           = 0x11,
    MBC_MBC3_RAM                       = 0x12,
    MBC_MBC3_RAM_BATTERY               = 0x13,
    MBC_MBC5                           = 0x19,
    MBC_MBC5_RAM                       = 0x1A,
    MBC_MBC5_RAM_BATTERY               = 0x1B,
    MBC_MBC5_RUMBLE                    = 0x1C,
    MBC_MBC5_RUMBLE_RAM                = 0x1D,
    MBC_MBC5_RUMBLE_RAM_BATTERY        = 0x1E,
    MBC_MBC6                           = 0x20,
    MBC_MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,

    MBC_POCKET_CAMERA                  = 0xFC,
    MBC_BANDAI_TAMA5                   = 0xFD,
    MBC_HuC3                           = 0xFE,
    MBC_HuC1_RAM_BATTERY               = 0xFF
} MBC;

typedef struct Cartridge {
    MBC type;
    uint8_t * rom;
    uint8_t * ram;
    size_t rom_size;
    size_t ram_size;

    uint8_t romb0;
    uint8_t romb1;
    uint8_t ramb;
    bool ramg;
    bool mode;
} Cartridge;

uint8_t cartridge_read_rom(GameBoy const * const gb, uint16_t address);
void cartridge_write_rom(GameBoy * const gb, uint16_t address, uint8_t value);

uint8_t cartridge_read_ram(GameBoy const * const gb, uint16_t address);
void cartridge_write_ram(GameBoy * const gb, uint16_t address, uint8_t value);
#endif /* !TRTLE_INTERNAL */

CartridgeError cartridge_from_file(Cartridge ** return_cart, char const * const path);
void cartridge_delete(Cartridge ** const cart);

#endif /* !TRTLE_CARTRIDGE_H */
