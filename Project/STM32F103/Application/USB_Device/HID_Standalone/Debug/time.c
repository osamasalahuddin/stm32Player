/**
  ******************************************************************************
  * file   : time.c
  * author : Osama Salah-ud-Din
  * version: V1.0
  * date   : 16-June-2016
  * brief  : Obtains current system up time
  ******************************************************************************
  */

#include "time.h"
#include "stm32f1xx_hal.h"

char        timestamp[30] = {0};
int         sysTicks = 0;
sys_time    sysTime;

sys_time    get_sysTime(void)
{
    sys_time    time;
    sysTicks    = HAL_GetTick()/10;
    time.milli  = sysTicks%100;                     // Milli seconds
    time.sec    = (sysTicks/100)%60;                // Seconds
    time.min    = ((sysTicks/100)/60)%60;           // Minutes
    time.hr     = (((sysTicks/100)/60)/60)%24;      // Hourse

    return  time;
}
/*      End of File time.c     */
