/**
  ******************************************************************************
  * file   : player.c 
  * author : Osama Salah-ud-Din
  * version: V1.0
  * date   : 25-June-2016
  * brief  : Audio Player
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "player.h"
#include "usbd_msc.h"
#include "usbd_storage.h"

/* Private variables ---------------------------------------------------------*/
uint8_t BlinkSpeed = 0, str[20];
extern FATFS SD_FatFs;             /* File system object for SD logical drive */
extern char SD_Path[4];            /* SD card logical drive path              */
char Music_list[5][20];     /* Upto 255 Songs of 20 word char length          */
extern VS1053_InitTypeDef vs1053;  /* VS1053 Handler Object                   */
extern USBD_States_t USBD_States; /* USB States                               */

/* FileSystem Variables */
FILINFO MyFileInfo;
DIR MyDirectory;
FIL MyFile;
UINT BytesWritten, BytesRead;



/**
  * @brief  Play the music files in the directory.
  * @param  VS1053 Pointer to Instance for SPI commands
  * @retval None
  */
void Play_Directory(VS1053_InitTypeDef* vs1053)
{
    static DIR directory;
    FRESULT res;
    /* This function is assuming non-Unicode cfg. */
    char *fn;

    /* Buffer to Store Long File Name */
    static char lfn[_MAX_LFN + 1];

#if _USE_LFN
    /* Use Long File Name Strings instead */
    MyFileInfo.lfname = lfn;
    MyFileInfo.lfsize = sizeof(lfn);
#endif

    /* Open the directory */
    res = f_opendir(&directory, "/");
    if((res != FR_OK))
    {
        if(res == FR_NO_FILESYSTEM)
        {
            /* Display message: SD card not FAT formated */
            ERROR("SD CARD NOT FORMATTED");    
        }
        else
        {
            /* Display message: Fail to open directory */
            ERROR("SD CARD OPEN FAIL");  
        }
    }
    else
    {
        /* No Error Found. Root Directory Successfully loaded */
        TRACE("SD CARD LOADED SUCCESSFULLY");

        /* Read Directory Enteries in Sequence */
        for (;;)
        {
            static char music_file_num = 0;
            res = f_readdir(&directory, &MyFileInfo);
            if(res != FR_OK) 
            {
                ERROR("Directory Not Valid");
                break;
            
            }
            if (MyFileInfo.fname[0] == 0)
                break;
            if(MyFileInfo.fname[0] == '.') 
                continue;

            /* If Long File Name not present then just use the Short Name */
#if _USE_LFN
            fn = *MyFileInfo.lfname ? MyFileInfo.lfname : MyFileInfo.fname;
#else
            fn = MyFileInfo.fname;
#endif            
            TRACE(fn);

            /* Check if this entry is either a Directory or a File */
            if (!(MyFileInfo.fattrib & AM_DIR))
            { 
                /* It is a File. Extract the file extension */
                const char* file_ext = Get_Filename_Ext(fn);
                if ((!strcmp(file_ext,"mp3")) || (!strcmp(file_ext,"MP3")))
                {
                    /* Its an MP3 File */
                    char iter;
                    /* Save the FileName in the Music List */
                    for (iter = 0; iter < sizeof(MyFileInfo.fname);  iter++)
                        Music_list[music_file_num][iter] = MyFileInfo.fname[iter];
                    music_file_num++;
                }
            }

            /* Dont Do anything if its a directory */
            continue;
        }
        PlayMusic(vs1053);

        /* Playback Finished Close the Directory */
        res = f_closedir(&directory);

    }
}

/**
  * @brief  Extract the File Extension from Filename.
  * @param  Filename as char string
  * @retval None
  */
const char* Get_Filename_Ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

/**
  * @brief  Play Music List.
  * @param  VS1053 for SPI commands
  * @retval None
  */
void PlayMusic(VS1053_InitTypeDef* vs1053)
{
    char playnum = 0;
    FIL file;
    char temp[255];
    uint32_t progress;
    static uint8_t file_read_buff[MAX_FILE_BUFF_SIZE];
    uint32_t bytes_read;
    uint8_t EndFillByte;
    
    /* While there are still files in the Music List */
    while(*Music_list[playnum])
    {
        char* fileName = Music_list[playnum++];
        FRESULT fres;
        uint8_t* to_send;
        char i;

        uint16_t count = 0;
        uint16_t sampleRate = 0;
        uint16_t sciMode = 0;

        /* if USB is connected */
        if (CONNECTED == USBD_States)
        {
            /* Abort Current Playback */
            return;
        }

        fres = f_open(&file, fileName, FA_OPEN_EXISTING | FA_READ);
        if (FR_OK == fres) 
        {
            /* The file is opened correctly */
            TRACE("Now Playing: %s",fileName);
            //TRACE("SCI_STATUS: 0x%04X",VS1053_sci_read(vs1053,SCI_STATUS));

            /* Read Old SCI Mode and Set SM_SDINEW flag */
            sciMode = VS1053_sci_read(vs1053,SCI_MODE);
            sciMode |= (SM_SDINEW);

            /* Reset Playback */
            VS1053_sci_write(vs1053, SCI_MODE, sciMode);

            //TRACE("SCI_MODE: 0x%04X",VS1053_sci_read(vs1053,SCI_MODE));
            ///* Write Bass Register */
            //VS1053_sci_write(vs1053,SCI_BASS,0x7A00);

            /* Re Sync */
            VS1053_sci_write(vs1053,SCI_WRAMADDR,0x1e29);
            VS1053_sci_write(vs1053,SCI_WRAM,0x0000);

            /* As explained in datasheet, set twice 0 in REG_DECODETIME to set time back to 0 */
            VS1053_sci_write(vs1053,SCI_DECODETIME,0x00);
            VS1053_sci_write(vs1053,SCI_DECODETIME,0x00);
            HAL_Delay(10);

            /* Read MAX_FILE_BUFF_SIZE bytes in file_read_buff */
            fres = f_read(&file, file_read_buff, MAX_FILE_BUFF_SIZE, &bytes_read);

            count = min(bytes_read,MAX_FILE_BUFF_SIZE);
            to_send = file_read_buff + count;
            progress = 0;

            for(i = 0; i < (MAX_FILE_BUFF_SIZE/BYTES_2_WRITE) - count; i++)
            {
                while(!(HAL_GPIO_ReadPin(vs1053->DREQport,vs1053->DREQpin))); //Wait while DREQ is low*/
                to_send = file_read_buff;
                VS1053_sdi_write32(vs1053,(to_send + i * BYTES_2_WRITE));
                count = 0;
            }

            /* Write WRAM Address */
            VS1053_sci_write(vs1053,SCI_WRAMADDR,0x1e06);
            EndFillByte = VS1053_sci_read(vs1053,SCI_WRAM);

            sampleRate = VS1053_sci_read(vs1053,SCI_AUDATA);
            TRACE("Sample Rate: %d",sampleRate);

            printf("Progress:         %d",progress);
            progress++;

            while (!f_eof(&file)) /* End of File Reached or Not */
            {
                /* if USB is connected */
                if (CONNECTED == USBD_States)
                {
                    /* Close the Opened File and Abort Current Playback */
                    fres = f_close(&file);
                    break;
                }

                /* Read MAX_FILE_BUFF_SIZE bytes in file_read_buff */
                fres = f_read(&file, file_read_buff, MAX_FILE_BUFF_SIZE, &bytes_read);

                HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_RESET);
                SD_CS_HIGH();

                to_send = file_read_buff;

                /* Send Single Byte of Data to VS1053 for Playback */
                while(bytes_read--)
                    VS1053_sdi_write(vs1053,to_send++,1);

                if (progress/100000)
                    printf("\b\b\b\b\b\b%d",progress++);
                else if (progress/10000)
                    printf("\b\b\b\b\b%d",progress++);
                else if (progress/1000)
                    printf("\b\b\b\b%d",progress++);
                else if (progress/100)
                    printf("\b\b\b%d",progress++);
                else if (progress/10)
                    printf("\b\b%d",progress++);
                else
                    printf("\b%d",progress++);
            }
            printf("\r\n");

            TRACE("Finished Playing");

            /* Cancel the Playback so no noise can be heard between the songs */
            VS1053_SendZeros(vs1053, EndFillByte);
            VS1053_sci_write(vs1053,SCI_MODE,SM_CANCEL);
            VS1053_SendZeros(vs1053, EndFillByte);
        }
        else
        {
            sprintf(temp,"FILE CAN'T BE OPENED. ERROR CODE:%d",fres);
            ERROR(temp);
        }
    }
}

