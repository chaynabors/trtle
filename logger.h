#ifndef TRTLE_LOGGER_H
#define TRTLE_LOGGER_H

#if defined(TRTLE_LOGGING)
#define TRTLE_LOG_ERR(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while (0)
#define TRTLE_LOG_WARN(fmt, ...)
#define TRTLE_LOG_INFO(fmt, ...)
#elif defined(TRTLE_LOGGING_VERBOSE)
#define TRTLE_LOG_ERR(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while (0)
#define TRTLE_LOG_WARN(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); } while(0)
#define TRTLE_LOG_INFO(fmt, ...) do { printf(fmt, __VA_ARGS__); } while(0)
#else
#define TRTLE_LOG_ERR(fmt, ...)
#define TRTLE_LOG_WARN(fmt, ...)
#define TRTLE_LOG_INFO(fmt, ...)
#endif

#endif /* !TRTLE_LOGGING_H*/
