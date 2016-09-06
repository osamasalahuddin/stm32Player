/**
  ******************************************************************************
  * file   : player.h
  * author : Osama Salah-ud-Din
  * version: V1.0
  * date   : 25-June-2016
  * brief  : Header file for Audio Player
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "Debug.h"
#include "VS1053.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define MAX_FILE_BUFF_SIZE      512
#define BYTES_2_WRITE           32

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void Play_Directory(VS1053_InitTypeDef* vs1053);
const char* Get_Filename_Ext(const char *filename);
void PlayMusic(VS1053_InitTypeDef* vs1053);
