#include <string.h>
#include "time.h"
#include "printf.h"

#ifndef __DEBUG_H__
#define __DEBUG_H__

#define MAX_RX_STRLEN 60            //Maximum Length of string which can be recieved from Consol

extern char     timestamp[];
extern int      sysTicks;
extern sys_time sysTime;

/* Use in code only for Logging of Fatal Errors */
#if ERROR_ENABLE
#define ERROR(...)                  \
        sysTime = get_sysTime();    \
        sprintf(                    \
        timestamp,                  \
        "%d:%02d:%02d.%03d",        \
        sysTime.hr,                 \
        sysTime.min,                \
        sysTime.sec,                \
        sysTime.milli);             \
        printf(                     \
        "[%s]%s(%d): [ERROR] ",     \
        timestamp,                  \
        __FILE__,                   \
        __LINE__                    \
        );                          \
        printf(__VA_ARGS__);        \
        printf("\r\n");
#else
#define ERROR(x)
#endif

/* Use in code only for Logging of Warnings */
#if WARNING_ENABLE
#define WARNING(...)                \
        sysTime = get_sysTime();    \
        sprintf(                    \
        timestamp,                  \
        "%d:%02d:%02d.%03d",        \
        sysTime.hr,                 \
        sysTime.min,                \
        sysTime.sec,                \
        sysTime.milli);             \
        printf(                     \
        "[%s]%s(%d): [WARNING] ",   \
        timestamp,                  \
        __FILE__,                   \
        __LINE__                    \
        );                          \
        printf(__VA_ARGS__);        \
        printf("\r\n");
#else
#define WARNING(x)
#endif

/* Use in code only for Log messages */
#if TRACE_ENABLE
#define TRACE(...)                  \
        sysTime = get_sysTime();    \
        sprintf(                    \
        timestamp,                  \
        "%d:%02d:%02d.%03d",        \
        sysTime.hr,                 \
        sysTime.min,                \
        sysTime.sec,                \
        sysTime.milli);             \
        printf(                     \
        "[%s]%s(%d): ",             \
        timestamp,                  \
        __FILE__,                   \
        __LINE__                    \
        );                          \
        printf(__VA_ARGS__);        \
        printf("\r\n");
#else
#define TRACE(x)
#endif

void init_debug(void);
#endif

