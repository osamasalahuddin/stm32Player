#include "stm32f1xx_hal_def.h"

#define SCI_MODE                0x00
#define SCI_STATUS              0x01
#define SCI_BASS                0x02
#define SCI_CLOCKF              0x03
#define SCI_DECODETIME          0x04
#define SCI_AUDATA              0x05
#define SCI_WRAM                0x06
#define SCI_WRAMADDR            0x07
#define SCI_HDAT0               0x08
#define SCI_HDAT1               0x09
#define SCI_VOL                 0x0B


#define SM_DIFF                 0x01
#define SM_JUMP                 0x02
#define SM_RESET                0x04
#define SM_CANCEL               0x08
#define SM_PDOWN                0x10
#define SM_TESTS                0x20
#define SM_STREAM               0x40
#define SM_PLUSV                0x80
#define SM_DACT                 0x100
#define SM_SDIORD               0x200
#define SM_SDISHARE             0x400
#define SM_SDINEW               0x800
#define SM_ADPCM                0x1000
#define SM_ADPCM_HP             0x2000
#define SM_LINE1                0x4000
#define SM_CLK_RANGE            0x8000

#define SCI_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE();\
                                __HAL_RCC_GPIOB_CLK_ENABLE()
                                
#define SCI_GPIO_CLK_DISABLE()  __HAL_RCC_GPIOA_CLK_DISABLE();\
                                __HAL_RCC_GPIOB_CLK_DISABLE()

typedef struct
{
    SPI_HandleTypeDef*  hspi;

    GPIO_TypeDef*       DREQport;
    uint16_t            DREQpin;

    GPIO_TypeDef*       CSport;
    uint16_t            CSpin;

    GPIO_TypeDef*       DCSport;
    uint16_t            DCSpin;

    GPIO_TypeDef*       RSTport;
    uint16_t            RSTpin;

    uint32_t            timeout;

    uint8_t             initialised;

    __IO                uint8_t playback;
    __IO                uint8_t datareq;
}VS1053_InitTypeDef;

void VS1053_configure(VS1053_InitTypeDef* vs1053,SPI_HandleTypeDef* hspi,GPIO_TypeDef* DREQport,uint16_t DREQpin,GPIO_TypeDef* CSport,uint16_t CSpin,
    GPIO_TypeDef* DCSport,uint16_t DCSpin,GPIO_TypeDef* RSTport,uint16_t RSTpin,uint32_t timeout);

void VS1053_sci_write(VS1053_InitTypeDef* vs1053,uint8_t addr,uint16_t data);
uint16_t VS1053_sci_read(VS1053_InitTypeDef* vs1053,uint8_t addr);
void VS1053_sdi_write(VS1053_InitTypeDef* vs1053,uint8_t* txbuff,uint16_t datasize);
void VS1053_sdi_write32(VS1053_InitTypeDef* vs1053,uint8_t* txbuff);
void VS1053_sci_write_DMA(VS1053_InitTypeDef* vs1053,uint8_t addr,uint16_t data);
void VS1053_sdi_write_DMA(VS1053_InitTypeDef* vs1053,uint8_t* txbuff,uint16_t datasize);
void SPI_Set_Baud_Prescaler(SPI_HandleTypeDef *hspi,uint32_t baud);
void VS1053_sci_setattenuation(VS1053_InitTypeDef* vs1053,uint8_t l_chan,uint8_t r_chan);
void VS1053_SoftReset(VS1053_InitTypeDef* vs1053);
void VS1053_SendZeros(VS1053_InitTypeDef* vs1053, uint8_t EndFillByte);
