#include "libretro.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#if defined(_WIN32) && !defined(_XBOX)
#include <windows.h>
#endif

#define TRTLE_LOGGING_VERBOSE
#include "trtle.h"

static GameBoy * gameboy;
static Cartridge * cart;
static uint32_t * frame_buf;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static void fallback_log(enum retro_log_level level, const char* fmt, ...) {
    (void)level;
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

void retro_init(void) {
    gameboy = gameboy_create();
    frame_buf = calloc(GAMEBOY_DISPLAY_PIXEL_COUNT, sizeof(uint32_t));
}

void retro_deinit(void) {
    free(frame_buf);
    frame_buf = NULL;
    gameboy_delete(gameboy);
    gameboy = NULL;
}

unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

void retro_get_system_info(struct retro_system_info * info) {
    memset(info, 0, sizeof(*info));
    info->library_name     = "trtle";
    info->library_version  = "0.1";
    info->need_fullpath    = false;
    info->valid_extensions = "gb";
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
    return;
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
    memset(info, 0, sizeof(*info));

    info->timing = (struct retro_system_timing){
        .fps = 59.727500569606f,
    };

    info->geometry = (struct retro_game_geometry){
        .base_width = GAMEBOY_DISPLAY_WIDTH,
        .base_height = GAMEBOY_DISPLAY_HEIGHT,
        .max_width = GAMEBOY_DISPLAY_WIDTH,
        .max_height = GAMEBOY_DISPLAY_HEIGHT,
        .aspect_ratio = GAMEBOY_DISPLAY_WIDTH / GAMEBOY_DISPLAY_HEIGHT,
    };
}

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;

    bool no_rom = false;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

    if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) log_cb = logging.log;
    else log_cb = fallback_log;

    static const struct retro_controller_description controllers[] = {
       { "Nintendo Game Boy", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
    };

    static const struct retro_controller_info ports[] = {
       { controllers, 1 },
       { NULL, 0 },
    };

    cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
    video_cb = cb;
}

void retro_reset(void) {
    gameboy_reset(gameboy);
}

void retro_run(void) {
    input_poll_cb();
    GameBoyInput input = {
        input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A),
        input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B),
        input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START),
        input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT),
        input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP),
        input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN),
        input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT),
        input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)
    };

    gameboy_update_to_vblank(gameboy, input);
    gameboy_get_display_data(gameboy, frame_buf, GAMEBOY_DISPLAY_PIXEL_COUNT);
    video_cb(frame_buf, GAMEBOY_DISPLAY_WIDTH, GAMEBOY_DISPLAY_HEIGHT, sizeof(uint32_t) * GAMEBOY_DISPLAY_WIDTH);
}

bool retro_load_game(const struct retro_game_info *info) {
    struct retro_input_descriptor desc[] = {
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
       { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
       { 0 },
    };

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
       log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
       return false;
    }

    if (info && info->data) {
        CartridgeError error = cartridge_from_memory(&gameboy->cartridge, info->data, info->size);
        if (error) {
            log_cb(RETRO_LOG_ERROR, "Error loading cartridge: %i.\n", error);
            return false;
        }
    }

    return true;
}

void retro_unload_game(void) {
    gameboy_set_cartridge(gameboy, NULL);
    cartridge_delete(cart);
}

unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    return false;
}

size_t retro_serialize_size(void) {
   return false;
}

bool retro_serialize(void *data_, size_t size) {
    return false;
}

bool retro_unserialize(const void *data_, size_t size) {
    return false;
}

void * retro_get_memory_data(unsigned id) {
    return NULL;
}

size_t retro_get_memory_size(unsigned id) {
    return 0;
}

void retro_cheat_reset(void) {
    return;
}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    return;
}
