#ifndef TRTLE_LOGGING_H
#define TRTLE_LOGGING_H

extern void logger_set_err_callback(void (*callback)(char* fmt, ...));
extern void logger_set_warn_callback(void (*callback)(char* fmt, ...));

#endif /* !TRTLE_LOGGING_H */
