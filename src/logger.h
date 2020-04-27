#ifndef TRTLE_LOGGER_H
#define TRTLE_LOGGER_H

void logger_set_err_callback(void (*callback)(char * fmt, ...));
void logger_set_warn_callback(void (*callback)(char * fmt, ...));
void logger_log_err(char * fmt, ...);
void logger_log_warn(char * fmt, ...);

#ifdef TRTLE_LOGGING
    #define TRTLE_LOG_ERR(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while (0)
    #ifdef TRTLE_LOGGING_VERBOSE
        #define TRTLE_LOG_WARN(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while(0)
    #else
        #define TRTLE_LOG_WARN(fmt, ...)
    #endif /* !TRTLE_LOGGING_VERBOSE */
#elif TRTLE_CUSTOM_LOGGING
    #define TRTLE_LOG_ERR(fmt, ...) do { logger_log_err(fmt, __VA_ARGS__); } while (0)
    #ifdef TRTLE_LOGGING_VERBOSE
        #define TRTLE_LOG_WARN(fmt, ...) do { logger_log_warm(fmt, __VA_ARGS__); } while(0)
    #else
        #define TRTLE_LOG_WARN(fmt, ...)
    #endif /* !TRTLE_LOGGING_VERBOSE */
#else
    #define TRTLE_LOG_ERR(fmt, ...)
    #define TRTLE_LOG_WARN(fmt, ...)
#endif /* !TRTLE_LOGGING && !TRTLE_LOGGING_CUSTOM */

#endif /* !TRTLE_LOGGING_H*/
