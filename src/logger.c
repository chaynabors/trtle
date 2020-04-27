#include "pch.h"
#include "logger.h"

static void (* err_callback)(char * fmt, ...);
static void (* warn_callback)(char * fmt, ...);
static void (* info_callback)(char * fmt, ...);

void logger_set_err_callback(void (*callback)(char* fmt, ...)) {
    err_callback = callback;
}

void logger_set_warn_callback(void (*callback)(char* fmt, ...)) {
    warn_callback = callback;
}

void logger_set_info_callback(void (*callback)(char* fmt, ...)) {
    info_callback = callback;
}

void logger_log_err(char* fmt, ...) {
    err_callback(fmt, ...);
}

void logger_log_warn(char* fmt, ...) {
    warn_callback(fmt, ...);
}

void logger_log_info(char* fmt, ...) {
    info_callback(fmt, ...);
}
