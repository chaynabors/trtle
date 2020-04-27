#ifndef TRTLE_TRTLE_CARTRIDGE_H
#define TRTLE_TRTLE_CARTRIDGE_H

typedef struct Cartridge Cartridge;

// TODO: This enum is defined twice, figure the contract out to the frontend
typedef enum CartridgeError {
    CARTRIDGE_ERROR_NONE = 0,
    CARTIRDGE_ERROR_RETURN_ARGUMENT_NULL,
    CARTRIDGE_ERROR_FILE_NOT_FOUND,
    CARTRIDGE_ERROR_CARTRIDGE_ALLOCATION_FAILED,
    CARTRIDGE_ERROR_ROM_ALLOCATION_FAILED,
    CARTRIDGE_ERROR_RAM_ALLOCATION_FAILED,
    CARTRIDGE_ERROR_MBC_NOT_SUPPORTED
} CartridgeError;
   

extern CartridgeError cartridge_from_file(Cartridge ** return_cart, char const* const path);
extern void cartridge_delete(Cartridge * cart);

#endif /* !TRTLE_TRTLE_CARTRIDGE_H */
