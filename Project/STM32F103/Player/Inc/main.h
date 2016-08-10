/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_nucleo.h"
#include "Debug.h"
#include <stdio.h>
#include <stdlib.h>
#include "VS1053.h"

/* FatFs includes component */
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "fatfs_storage.h"


/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Definition for USARTx clock resources */
#define USARTx                           USART2
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART2_CLK_ENABLE();
#define USARTx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define USARTx_FORCE_RESET()             __HAL_RCC_USART2_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART2_RELEASE_RESET()

/* Definition for USARTx Pins */
#define USARTx_TX_PIN                    GPIO_PIN_2
#define USARTx_TX_GPIO_PORT              GPIOA
#define USARTx_RX_PIN                    GPIO_PIN_3
#define USARTx_RX_GPIO_PORT              GPIOA

/* Definition for USARTx's NVIC */
#define USARTx_IRQn                      USART2_IRQn
#define USARTx_IRQHandler                USART2_IRQHandler


/* Definition for SPI_VS clock resources */
#define SPI_VS                           SPI2
#define SPI_VS_CLK_ENABLE()              __HAL_RCC_SPI2_CLK_ENABLE()
#define SPI_VS_SCK_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOB_CLK_ENABLE()
#define SPI_VS_MISO_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOB_CLK_ENABLE()
#define SPI_VS_MOSI_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOB_CLK_ENABLE()

/* Definition for SPI_VS Pins */
#define SPI_VS_SCK_PIN                   GPIO_PIN_13
#define SPI_VS_SCK_GPIO_PORT             GPIOB
#define SPI_VS_MISO_PIN                  GPIO_PIN_14
#define SPI_VS_MISO_GPIO_PORT            GPIOB
#define SPI_VS_MOSI_PIN                  GPIO_PIN_15
#define SPI_VS_MOSI_GPIO_PORT            GPIOB

/* Definition for SCI_VS Pins */
#define SCI_VS_DREQ_PIN                  GPIO_PIN_1
#define SCI_VS_DREQ_GPIO_PORT            GPIOA
#define SCI_VS_CS_PIN                    GPIO_PIN_6
#define SCI_VS_CS_GPIO_PORT              GPIOB
#define SCI_VS_DCS_PIN                   GPIO_PIN_4
#define SCI_VS_DCS_GPIO_PORT             GPIOA
#define SCI_VS_RST_PIN                   GPIO_PIN_0
#define SCI_VS_RST_GPIO_PORT             GPIOA

/* Definition of VS1053 Timeout value 5sec */
#define VS1053_TIMEOUT                   5000

/* Definition for SPI_VS's NVIC */
#define SPIx_IRQn                        SPI2_IRQn
#define SPIx_IRQHandler                  SPI2_IRQHandler


/* Definition for SPI_SD clock resources */
#define SPI_SD                           SPI1
#define SPI_SD_CLK_ENABLE()              __HAL_RCC_SPI1_CLK_ENABLE()
#define SPI_SD_SCK_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI_SD_MISO_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOA_CLK_ENABLE()
#define SPI_SD_MOSI_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOA_CLK_ENABLE()

/* Definition for SPI_SD Pins */
#define SPI_SD_SCK_PIN                   GPIO_PIN_5
#define SPI_SD_SCK_GPIO_PORT             GPIOA
#define SPI_SD_MISO_PIN                  GPIO_PIN_6
#define SPI_SD_MISO_GPIO_PORT            GPIOA
#define SPI_SD_MOSI_PIN                  GPIO_PIN_7
#define SPI_SD_MOSI_GPIO_PORT            GPIOA

#define SCI_SD_CS_PIN                    GPIO_PIN_5
#define SCI_SD_CS_GPIO_PORT              GPIOB

/* Definition for SPI_SD's NVIC */
#define SPI_SD_IRQn                      SPI1_IRQn
#define SPI_SD_IRQHandler                SPI1_IRQHandler

#define min(a,b) (((a)<(b))?(a):(b))

#endif /* __MAIN_H__ */
