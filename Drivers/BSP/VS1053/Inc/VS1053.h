#include "stm32f1xx_hal_def.h"

/* FatFs includes component */
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "fatfs_storage.h"

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
#define SCI_AICTRL0             0x0C /* VS1063, VS1053, VS1033, VS1003, VS1011 */
#define SCI_MIXERVOL            0x0C /* VS1103 */
#define SCI_AICTRL1             0x0D /* VS1063, VS1053, VS1033, VS1003, VS1011 */
#define SCI_ADPCMRECCTL         0x0D /* VS1103 */
#define SCI_AICTRL2             0x0E
#define SCI_AICTRL3             0x0F


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

/* VS1063 / VS1053 Parametric */
#define PAR_CHIP_ID                  0x1e00 /* VS1063, VS1053, 32 bits */
#define PAR_VERSION                  0x1e02 /* VS1063, VS1053 */
#define PAR_CONFIG1                  0x1e03 /* VS1063, VS1053 */
#define PAR_PLAY_SPEED               0x1e04 /* VS1063, VS1053 */
#define PAR_BITRATE_PER_100          0x1e05 /* VS1063 */
#define PAR_BYTERATE                 0x1e05 /* VS1053 */
#define PAR_END_FILL_BYTE            0x1e06 /* VS1063, VS1053 */
#define PAR_RATE_TUNE                0x1e07 /* VS1063,         32 bits */
#define PAR_PLAY_MODE                0x1e09 /* VS1063 */
#define PAR_SAMPLE_COUNTER           0x1e0a /* VS1063,         32 bits */
#define PAR_VU_METER                 0x1e0c /* VS1063 */
#define PAR_AD_MIXER_GAIN            0x1e0d /* VS1063 */
#define PAR_AD_MIXER_CONFIG          0x1e0e /* VS1063 */
#define PAR_PCM_MIXER_RATE           0x1e0f /* VS1063 */
#define PAR_PCM_MIXER_FREE           0x1e10 /* VS1063 */
#define PAR_PCM_MIXER_VOL            0x1e11 /* VS1063 */
#define PAR_EQ5_DUMMY                0x1e12 /* VS1063 */
#define PAR_EQ5_LEVEL1               0x1e13 /* VS1063 */
#define PAR_EQ5_FREQ1                0x1e14 /* VS1063 */
#define PAR_EQ5_LEVEL2               0x1e15 /* VS1063 */
#define PAR_EQ5_FREQ2                0x1e16 /* VS1063 */
#define PAR_JUMP_POINTS              0x1e16 /*         VS1053 */
#define PAR_EQ5_LEVEL3               0x1e17 /* VS1063 */
#define PAR_EQ5_FREQ3                0x1e18 /* VS1063 */
#define PAR_EQ5_LEVEL4               0x1e19 /* VS1063 */
#define PAR_EQ5_FREQ4                0x1e1a /* VS1063 */
#define PAR_EQ5_LEVEL5               0x1e1b /* VS1063 */
#define PAR_EQ5_UPDATED              0x1e1c /* VS1063 */
#define PAR_SPEED_SHIFTER            0x1e1d /* VS1063 */
#define PAR_EARSPEAKER_LEVEL         0x1e1e /* VS1063 */
#define PAR_SDI_FREE                 0x1e1f /* VS1063 */
#define PAR_AUDIO_FILL               0x1e20 /* VS1063 */
#define PAR_RESERVED0                0x1e21 /* VS1063 */
#define PAR_RESERVED1                0x1e22 /* VS1063 */
#define PAR_RESERVED2                0x1e23 /* VS1063 */
#define PAR_RESERVED3                0x1e24 /* VS1063 */
#define PAR_LATEST_SOF               0x1e25 /* VS1063,         32 bits */
#define PAR_LATEST_JUMP              0x1e26 /*         VS1053 */
#define PAR_POSITION_MSEC            0x1e27 /* VS1063, VS1053, 32 bits */
#define PAR_RESYNC                   0x1e29 /* VS1063, VS1053 */

#define PAR_PLAY_MODE_PAUSE_ENA         (1<<1) /* VS1063 */

/* Following are for VS1053 and VS1063 */
#define SC_MULT_53_10X 0x0000
#define SC_MULT_53_20X 0x2000
#define SC_MULT_53_25X 0x4000
#define SC_MULT_53_30X 0x6000
#define SC_MULT_53_35X 0x8000
#define SC_MULT_53_40X 0xa000
#define SC_MULT_53_45X 0xc000
#define SC_MULT_53_50X 0xe000

/* Following are for VS1053 and VS1063 */
#define SC_ADD_53_00X 0x0000
#define SC_ADD_53_10X 0x0800
#define SC_ADD_53_15X 0x1000
#define SC_ADD_53_20X 0x1800

#define PAR_CONFIG1_AAC_SBR_SELECTIVE_UPSAMPLE 0x0010 /* VS1063, VS1053 */

/* The following macro is for VS1063, VS1053, VS1033, VS1003, VS1103.
   Divide hz by two when calling if SM_CLK_RANGE = 1 */
#define HZ_TO_SC_FREQ(hz) (((hz)-8000000+2000)/4000)

#define SCI_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE();\
                                __HAL_RCC_GPIOB_CLK_ENABLE()
                                
#define SCI_GPIO_CLK_DISABLE()  __HAL_RCC_GPIOA_CLK_DISABLE();\
                                __HAL_RCC_GPIOB_CLK_DISABLE()

#define ReadSci(x, y)           VS1053_sci_read(x,y)
#define WriteSci(x, y, z)       VS1053_sci_write(x, y, z)
#define WriteSdi(x, y, z)       VS1053_sdi_write(x, y, z)


#define min(a,b) (((a)<(b))?(a):(b))

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
void VS1053_SineTest(VS1053_InitTypeDef* vs1053);

void LoadPlugin(VS1053_InitTypeDef* vs1053, const uint16_t *d, uint16_t len);
int VSTestHandleFile(VS1053_InitTypeDef* vs1053, const char *fileName, int record);
int VSTestInitSoftware(VS1053_InitTypeDef* vs1053);
