#ifndef TRTLE_LOGGER_H
#define TRTLE_LOGGER_H

void logger_set_err_callback(void (*callback)(char * fmt, ...));
void logger_set_warn_callback(void (*callback)(char * fmt, ...));
void logger_set_info_callback(void (*callback)(char* fmt, ...));

#if defined(TRTLE_INTERNAL)
void logger_log_err(char * fmt, ...);
void logger_log_warn(char * fmt, ...);
void logger_log_info(char* fmt, ...);

#if defined(TRTLE_LOGGING)
#define TRTLE_LOG_ERR(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while (0)
#define TRTLE_LOG_WARN(fmt, ...)
#define TRTLE_LOG_INFO(fmt, ...)
#elif defined(TRTLE_LOGGING_VERBOSE)
#define TRTLE_LOG_ERR(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while (0)
#define TRTLE_LOG_WARN(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while(0)
#define TRTLE_LOG_INFO(fmt, ...) do { printf(fmt, __VA_ARGS__); } while(0)
#elif defined(TRTLE_CUSTOM_LOGGING)
#define TRTLE_LOG_ERR(fmt, ...) do { logger_log_err(fmt, __VA_ARGS__); } while (0)
#define TRTLE_LOG_WARN(fmt, ...)
#define TRTLE_LOG_INFO(fmt, ...)
#elif defined(TRTLE_CUSTOM_LOGGING_VERBOSE)
#define TRTLE_LOG_ERR(fmt, ...) do { logger_log_err(fmt, __VA_ARGS__); } while (0)
#define TRTLE_LOG_WARN(fmt, ...) do { logger_log_warn(fmt, __VA_ARGS__); } while(0)
#define TRTLE_LOG_INFO(fmt, ...) do { logger_log_info(fmt, __VA_ARGS__); } while(0)
#else
#define TRTLE_LOG_ERR(fmt, ...)
#define TRTLE_LOG_WARN(fmt, ...)
#define TRTLE_LOG_INFO(fmt, ...)
#endif
#endif /* !TRTLE_INTERNAL */

#endif /* !TRTLE_LOGGING_H*/
