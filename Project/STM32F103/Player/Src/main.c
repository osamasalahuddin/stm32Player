/**
  ******************************************************************************
  * file   : main.c 
  * author : Osama Salah-ud-Din
  * version: V1.0
  * date   : 16-June-2016
  * brief  : Main program body
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint8_t BlinkSpeed = 0, str[20];
FATFS SD_FatFs;             /* File system object for SD card logical drive   */
char SD_Path[4];            /* SD card logical drive path                     */
char Music_list[5][20];   /* Upto 255 Songs of 20 word char length          */
VS1053_InitTypeDef vs1053;  /* VS1053 Handler Object                          */
SPI_HandleTypeDef SpiHandle;/* SPI handler declaration                        */

/* FileSystem Variables */
FILINFO MyFileInfo;
DIR MyDirectory;
FIL MyFile;
UINT BytesWritten, BytesRead;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void LED2_Blink(void);
static void SPI_VS_Config(void);
static void SPI_SD_Config(void);
static void SDCard_Config(void);
static void Play_Directory(void);
static const char* Get_Filename_Ext(const char *filename);
void PlayMusic(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{  
    /* STM32F103xB HAL library initialization:
       - Configure the Flash prefetch
       - Systick timer is configured by default as source of time base, but user 
         can eventually implement his proper time base source (a general purpose 
         timer for example or other time source), keeping in mind that Time base 
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
    uint16_t temp;

    HAL_Init();

    /* Configure the system clock = 64 MHz */
    SystemClock_Config();

    init_debug();

    TRACE("Hello World");

    /* Initialize SPI for VS1053 */
    SPI_VS_Config();

    /* Configure VS1053 Chip */
    VS1053_configure(&vs1053,&SpiHandle,
                     SCI_VS_DREQ_GPIO_PORT,SCI_VS_DREQ_PIN,
                     SCI_VS_CS_GPIO_PORT  ,SCI_VS_CS_PIN,
                     SCI_VS_DCS_GPIO_PORT ,SCI_VS_DCS_PIN,
                     SCI_VS_RST_GPIO_PORT ,SCI_VS_RST_PIN,
                     VS1053_TIMEOUT);

    /* Read Chip ID of VS1053 */
    temp = VS1053_sci_read(&vs1053,SCI_STATUS);
    TRACE2("VS1053 Chip ID: 0x%04X",temp);

    /* Initialize SPI for SD Card */
    //SPI_SD_Config();
    TRACE("SD Card SPI Initialized");

    /* ToDo: Check the availability of the SD card here. */
    if(1)
    {
        /* Configure SD card */
        SDCard_Config();
    }
    else /* SD Card not mounted */
    {
        ERROR("Failed to load SD Card");
        LED2_Blink();
    }

    /* Initialize SPI for VS1053 */
    //SPI_VS_Config();

    /* Write Clock Register, Doubler etc */
    VS1053_sci_write(&vs1053,SCI_CLOCKF,0xa000);

    /* Read Clock F register */
    TRACE2("Clock F: 0x%04X",(VS1053_sci_read(&vs1053,SCI_CLOCKF)));

    /* Perform a Soft Reset of VS1053 */
    VS1053_SoftReset(&vs1053);

    /* Write WRAM Address */
    VS1053_sci_write(&vs1053,SCI_WRAMADDR,0xC013);

    temp = (VS1053_sci_read(&vs1053,SCI_WRAMADDR));

    /* Reading the Value Twice Just to be sure that the value is not changed in between */
    if (VS1053_sci_read(&vs1053,SCI_WRAMADDR) == temp) 
        TRACE2("WRAMADDR1: 0x%04X",temp);

    /* Adjust the Volume at level 20 for both channels */
    VS1053_sci_setattenuation(&vs1053,20,20);


    BSP_SD_Init();

    /* Initialize the Directory Files pointers (heap) */
    Play_Directory();

    /* Infinite loop */
    while (1)
    {;}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 64000000
  *            HCLK(Hz)                       = 64000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            PLLMUL                         = 16
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef clkinitstruct = {0};
    RCC_OscInitTypeDef oscinitstruct = {0};

    /* Configure PLL ------------------------------------------------------*/
    /* PLL configuration: PLLCLK = (HSI / 2) * PLLMUL = (8 / 2) * 16 = 64 MHz */
    /* PREDIV1 configuration: PREDIV1CLK = PLLCLK / HSEPredivValue = 64 / 1 = 64 MHz */
    /* Enable HSI and activate PLL with HSi_DIV2 as source */
    oscinitstruct.OscillatorType  = RCC_OSCILLATORTYPE_HSI;
    oscinitstruct.HSEState        = RCC_HSE_OFF;
    oscinitstruct.LSEState        = RCC_LSE_OFF;
    oscinitstruct.HSIState        = RCC_HSI_ON;
    oscinitstruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscinitstruct.HSEPredivValue  = RCC_HSE_PREDIV_DIV1;
    oscinitstruct.PLL.PLLState    = RCC_PLL_ON;
    oscinitstruct.PLL.PLLSource   = RCC_PLLSOURCE_HSI_DIV2;
    oscinitstruct.PLL.PLLMUL      = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&oscinitstruct)!= HAL_OK)
    {
      /* Initialization Error */
      while(1); 
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
       clocks dividers */
    clkinitstruct.ClockType      = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    clkinitstruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clkinitstruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
    clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;  
    if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2)!= HAL_OK)
    {
        /* Initialization Error */
        while(1); 
    }
}


/**
  * @brief  SD Card Configuration.
  * @param  None
  * @retval None
  */
static void SDCard_Config(void)
{
    if(FATFS_LinkDriver(&SD_Driver, SD_Path) == 0)
    {
        /* Initialize the SD mounted on adafruit 1.8" TFT shield */
        if(BSP_SD_Init() != MSD_OK)
        {
            ERROR("BSP_SD_INIT_FAILED");
        }

        /* Check the mounted device */
        if(f_mount(&SD_FatFs, (TCHAR const*)"/", 0) != FR_OK)
        {
            ERROR("FATFS_NOT_MOUNTED");
        }
        else
        {
            
        }
    }
}


/**
  * @brief  Blinks LED2 with two frequencies depending on User press button.
  * @param  None
  * @retval None
  */
static void LED2_Blink(void)
{
    /* Configure LED2 on Nucleo */
    BSP_LED_Init(LED2);

    /* Configure the User Button in EXTI Mode */
    BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
  
    /* Initiate BlinkSpeed variable */ 
    BlinkSpeed = 0;  
  
    /* Infinite loop */
    while(1)
    {
        /* Test on blink speed */
        if(BlinkSpeed == 0)
        {
            BSP_LED_Toggle(LED2);
            /* Wait for 500ms */      
            HAL_Delay(500);      
        }      
        else if(BlinkSpeed == 1)
        {
            BSP_LED_Toggle(LED2);
            /* Wait for 100ms */
            HAL_Delay(100); 
        }
        else if(BlinkSpeed == 2)
        {
            BSP_LED_Toggle(LED2);    
            /* wait for 50ms */
            HAL_Delay(50);  
        }
    }
}


/**
  * @brief  Configure SPI to be used with VS1053.
  * @param  None
  * @retval None
  */
static void SPI_VS_Config(void)
{
    /*##-1- Configure the SPI peripheral #######################################*/

    /* Set the SPI parameters */
    SpiHandle.Instance               = SPI_VS;
    SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
    SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
    SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
    SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
    SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
    SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    SpiHandle.Init.CRCPolynomial     = 7;
    SpiHandle.Init.NSS               = SPI_NSS_SOFT;
    SpiHandle.Init.Mode              = SPI_MODE_MASTER;

    if(HAL_SPI_Init(&SpiHandle) != HAL_OK)
    {
        /* Initialization Error */
        ERROR("SPI Initialization Failed");
    }

    /* Enable SPI */
    __HAL_SPI_ENABLE(&SpiHandle);
}

/**
  * @brief  Configure SPI to be used with SD Card.
  * @param  None
  * @retval None
  */
static void SPI_SD_Config(void)
{
    /*##-1- Configure the SPI peripheral #######################################*/

    /* Set the SPI parameters */
    SpiHandle.Instance               = SPI_SD;
    SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
    SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
    SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
    SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
    SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
    SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    SpiHandle.Init.CRCPolynomial     = 7;
    SpiHandle.Init.NSS               = SPI_NSS_SOFT;
    SpiHandle.Init.Mode              = SPI_MODE_MASTER;

    if(HAL_SPI_Init(&SpiHandle) != HAL_OK)
    {
        /* Initialization Error */
        ERROR("SPI Initialization Failed");
    }

    /* Enable SPI */
    __HAL_SPI_ENABLE(&SpiHandle);
}

/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(BlinkSpeed == 2)
    {
        BlinkSpeed = 0;
    }
    else
    {
        BlinkSpeed++;
    }
}

/**
  * @brief  Play the music files in the directory.
  * @param  None
  * @retval None
  */
static void Play_Directory(void)
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
        char temp[20];
        /* No Error Found. Root Directory Successfully loaded */
        TRACE("SD CARD LOADED SUCCESSFULLY");

        /* Read Directory Enteries in Sequence */
        for (;;)
        {
            static char music_file_num = 0;
            res = f_readdir(&directory, &MyFileInfo);
            if (MyFileInfo.fname[0] == 0)
                break;
            if(res != FR_OK) 
            {
                ERROR("Directory Not Valid");
                break;
            
            }
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
        PlayMusic();
    }
}

/**
  * @brief  Extract the File Extension from Filename.
  * @param  Filename as char string
  * @retval None
  */
static const char* Get_Filename_Ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

/**
  * @brief  Play Music List.
  * @param  None
  * @retval None
  */
void PlayMusic(void)
{
    char playnum = 0;
    FIL file;
    char temp[255];
    
    /* While there are still files in the Music List */
    while(*Music_list[playnum])
    {
        char* temp2 = Music_list[playnum++];
        FRESULT fres;
        fres = f_open(&file, temp2, FA_OPEN_EXISTING | FA_READ);
        if (FR_OK == fres) 
        {
            /* The file is opened correctly */
            sprintf(temp, "Now Playing: %s",temp2);
            TRACE(temp);
            
            
        }
        else
        {
            sprintf(temp,"FILE CAN'T BE OPENED. ERROR CODE:%d",fres);
            ERROR(temp);
        }
    }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    ERROR("Assert Failed");
    /* Infinite loop */
    while (1)
    {;}
}
#endif

/************************ (C) COPYRIGHT Osama Salah-ud-Din *****END OF FILE****/
