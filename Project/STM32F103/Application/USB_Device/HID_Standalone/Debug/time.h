#ifndef __TIME_H__
#define __TIME_H__

typedef struct systime_t{
    char    milli;
    char    sec;
    char    min;
    char    hr;
}sys_time;

sys_time    get_sysTime(void);
#endif
