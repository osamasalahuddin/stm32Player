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

/** @addtogroup STM32F1xx_HAL_Validation
  * @{
  */

/** @addtogroup STANDARD_CHECK
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
USBD_HandleTypeDef USBD_Device;
extern USBD_States_t USBD_States; /* USB States                               */
SPI_HandleTypeDef SpiHandle;/* SPI handler declaration                        */
VS1053_InitTypeDef vs1053;  /* VS1053 Handler Object                          */
FATFS SD_FatFs;             /* File system object for SD card logical drive   */
char SD_Path[4];            /* SD card logical drive path                     */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void Error_Handler(void);
static void GetPointerData(uint8_t *pbuf);
static void SPI_VS_Config(void);
static void SDCard_Config(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    
    /* Configure the system clock to 72 MHz */
    SystemClock_Config();
  
    init_debug();

    USBD_States = UNINITIALIZED;

    /* Initialize LED2 */
    BSP_LED_Init(LED2);

    /* Configure Key button for remote wakeup */
    BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

    TRACE("Hello World");

    /* Init Device Library */
    USBD_Init(&USBD_Device, &MSC_Desc, 0);

    /* Register the HID class */
    USBD_RegisterClass(&USBD_Device, USBD_MSC_CLASS);

    /* Add Storage callbacks for MSC Class */
    USBD_MSC_RegisterStorage(&USBD_Device, &USBD_DISK_fops);

    /* Start Device Process */
    USBD_Start(&USBD_Device);

    TRACE("USB Configured");

//    /* Initialize SPI for VS1053 */
    SPI_VS_Config();

    /* Configure VS1053 Chip */
    VS1053_configure(&vs1053,&SpiHandle,
                     SCI_VS_DREQ_GPIO_PORT,SCI_VS_DREQ_PIN,
                     SCI_VS_CS_GPIO_PORT  ,SCI_VS_CS_PIN  ,
                     SCI_VS_DCS_GPIO_PORT ,SCI_VS_DCS_PIN ,
                     SCI_VS_RST_GPIO_PORT ,SCI_VS_RST_PIN ,
                     VS1053_TIMEOUT);

    /* Check the availability of the SD card here. */
    /* Configure SD card */
    SDCard_Config();
    TRACE("SD Card Configured");

    /* Perform a Soft Reset of VS1053 */
    VS1053_SoftReset(&vs1053);

    /* Adjust the Volume at level 30 for both channels */
    VS1053_sci_setattenuation(&vs1053,30,30);

    /* Initialize the Directory Files pointers (heap) */
    Play_Directory(&vs1053);

    TRACE("Playback Finished");
    while (1)
    {
        /* Insert delay 100 ms */
        HAL_Delay(100);  
        BSP_LED_Toggle(LED2);
        HAL_Delay(100);  
    }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 72000000
  *            HCLK(Hz)                       = 72000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            HSE Frequency(Hz)              = 8000000
  *            HSE PREDIV1                    = 1
  *            PLLMUL                         = 9
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef clkinitstruct = {0};
    RCC_OscInitTypeDef oscinitstruct = {0};
    RCC_PeriphCLKInitTypeDef rccperiphclkinit = {0};

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    oscinitstruct.OscillatorType  = RCC_OSCILLATORTYPE_HSE;
    oscinitstruct.HSEState        = RCC_HSE_ON;
    oscinitstruct.HSEPredivValue  = RCC_HSE_PREDIV_DIV1;
    oscinitstruct.PLL.PLLMUL      = RCC_PLL_MUL9;

    oscinitstruct.PLL.PLLState    = RCC_PLL_ON;
    oscinitstruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;

    if (HAL_RCC_OscConfig(&oscinitstruct)!= HAL_OK)
    {
        /* Start Conversation Error */
        Error_Handler(); 
    }

    /* USB clock selection */
    rccperiphclkinit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    rccperiphclkinit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
    HAL_RCCEx_PeriphCLKConfig(&rccperiphclkinit);

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
    clocks dividers */
    clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    clkinitstruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clkinitstruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;  
    clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2)!= HAL_OK)
    {
        /* Start Conversation Error */
        Error_Handler(); 
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  BSP_LED_On(LED2);
  while (1)
  {
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
        /* Initialize the SD mounted */
        if(BSP_SD_Init() != MSD_OK)
        {
            ERROR("SD CARD INITIALIZATION FAILED");
        }

        /* Check the mounted device */
        if(f_mount(&SD_FatFs, (TCHAR const*)"/", 0) != FR_OK)
        {
            ERROR("FATFS MOUNT FAILED");
        }
        else
        {
            /* All OK Do Nothing */
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

  /* Infinite loop */
  while (1)
  {
  }
}

#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
