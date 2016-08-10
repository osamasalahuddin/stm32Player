//VS1053 STM32 Driver
#include "stm32f1xx_hal.h"
#include "Debug.h"
#include "VS1053.h"

/* Download the latest VS1053a Patches package and its
   vs1053b-patches-flac.plg. If you want to use the smaller patch set
   which doesn't contain the FLAC decoder, use vs1053b-patches.plg instead.
   The patches package is available at
   http://www.vlsi.fi/en/support/software/vs10xxpatches.html */
#include "vs1053b-patches.plg"


/* Note: code SS_VER=2 is used for both VS1002 and VS1011e */
const uint16_t chipNumber[16] = {
  1001, 1011, 1011, 1003, 1053, 1033, 1063, 1103,
  0, 0, 0, 0, 0, 0, 0, 0
};

uint8_t adpcmHeader[60] = {
  'R', 'I', 'F', 'F',
  0xFF, 0xFF, 0xFF, 0xFF,
  'W', 'A', 'V', 'E',
  'f', 'm', 't', ' ',
  0x14, 0, 0, 0,          /* 20 */
  0x11, 0,                /* IMA ADPCM */
  0x1, 0,                 /* chan */
  0x0, 0x0, 0x0, 0x0,     /* sampleRate */
  0x0, 0x0, 0x0, 0x0,     /* byteRate */
  0, 1,                   /* blockAlign */
  4, 0,                   /* bitsPerSample */
  2, 0,                   /* byteExtraData */
  0xf9, 0x1,              /* samplesPerBlock = 505 */
  'f', 'a', 'c', 't',     /* subChunk2Id */
  0x4, 0, 0, 0,           /* subChunk2Size */
  0xFF, 0xFF, 0xFF, 0xFF, /* numOfSamples */
  'd', 'a', 't', 'a',
  0xFF, 0xFF, 0xFF, 0xFF
};

uint8_t pcmHeader[44] = {
  'R', 'I', 'F', 'F',
  0xFF, 0xFF, 0xFF, 0xFF,
  'W', 'A', 'V', 'E',
  'f', 'm', 't', ' ',
  0x10, 0, 0, 0,          /* 16 */
  0x1, 0,                 /* PCM */
  0x1, 0,                 /* chan */
  0x0, 0x0, 0x0, 0x0,     /* sampleRate */
  0x0, 0x0, 0x0, 0x0,     /* byteRate */
  2, 0,                   /* blockAlign */
  0x10, 0,                /* bitsPerSample */
  'd', 'a', 't', 'a',
  0xFF, 0xFF, 0xFF, 0xFF
};

const char *afName[] = {
  "unknown",
  "RIFF",
  "Ogg",
  "MP1",
  "MP2",
  "MP3",
  "AAC MP4",
  "AAC ADTS",
  "AAC ADIF",
  "FLAC",
  "WMA",
  "MIDI",
};

enum PlayerStates 
{
  psPlayback = 0,
  psUserRequestedCancel,
  psCancelSentToVS10xx,
  psStopped
} playerState;


enum AudioFormat {
  afUnknown,
  afRiff,
  afOggVorbis,
  afMp1,
  afMp2,
  afMp3,
  afAacMp4,
  afAacAdts,
  afAacAdif,
  afFlac,
  afWma,
  afMidi,
} audioFormat = afUnknown;

void VS1053_configure(VS1053_InitTypeDef* vs1053,SPI_HandleTypeDef* hspi,
                      GPIO_TypeDef* DREQport,uint16_t DREQpin,
                      GPIO_TypeDef* CSport,uint16_t CSpin,
                      GPIO_TypeDef* DCSport,uint16_t DCSpin,
                      GPIO_TypeDef* RSTport,uint16_t RSTpin,
                      uint32_t timeout)
{
    uint8_t tempData    = 0xFF;
    uint16_t ssVer;

    vs1053->hspi        = hspi;
    vs1053->DREQport    = DREQport;
    vs1053->DREQpin     = DREQpin;
    vs1053->CSport      = CSport;
    vs1053->CSpin       = CSpin;
    vs1053->DCSport     = DCSport;
    vs1053->DCSpin      = DCSpin;
    vs1053->RSTport     = RSTport;
    vs1053->RSTpin      = RSTpin;
    vs1053->timeout     = timeout;

    /* Configure the above configured GPIOs */
    GPIO_InitTypeDef  gpioinitstruct = {0};
  
    /* Enable the SCI Pins Clock */
    SCI_GPIO_CLK_ENABLE();

    /* Configure the SCI pins */
    gpioinitstruct.Mode   = GPIO_MODE_OUTPUT_PP;
    gpioinitstruct.Pull   = GPIO_NOPULL;
    gpioinitstruct.Speed  = GPIO_SPEED_FREQ_HIGH;
    gpioinitstruct.Pin    = vs1053->CSpin;
    HAL_GPIO_Init(vs1053->CSport, &gpioinitstruct);
    gpioinitstruct.Pin    = vs1053->DCSpin;
    HAL_GPIO_Init(vs1053->DCSport, &gpioinitstruct);
    gpioinitstruct.Pin    = vs1053->RSTpin;
    HAL_GPIO_Init(vs1053->RSTport, &gpioinitstruct);

    gpioinitstruct.Mode   = GPIO_MODE_INPUT;
    gpioinitstruct.Pull   = GPIO_NOPULL;
    gpioinitstruct.Speed  = GPIO_SPEED_FREQ_MEDIUM;
    gpioinitstruct.Pin    = vs1053->DREQpin;
    HAL_GPIO_Init(vs1053->DREQport, &gpioinitstruct);
    
    /* Put VS1053 in reset state */
    HAL_GPIO_WritePin(vs1053->RSTport,vs1053->RSTpin,GPIO_PIN_SET);     //RESET High
    HAL_GPIO_WritePin(vs1053->CSport, vs1053->CSpin, GPIO_PIN_SET);     //xCS High
    HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_RESET);     //xDCS Low
    HAL_Delay(10);

    /* Send Dummy Data on SPI bus */
    if(HAL_OK != HAL_SPI_Transmit(vs1053->hspi,&tempData,1,vs1053->timeout))
    {
        ERROR("SPI TX FAILED");
        return;
    }

    HAL_Delay(100);

    /* Release VS1053 from reset state */
    HAL_GPIO_WritePin(vs1053->CSport, vs1053->CSpin, GPIO_PIN_SET);     //xCS High
    HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_SET);     //xDCS High
    HAL_GPIO_WritePin(vs1053->RSTport,vs1053->RSTpin,GPIO_PIN_SET);     //RESET High

    while(!HAL_GPIO_ReadPin(vs1053->DREQport,vs1053->DREQpin));

    TRACE("VS1053 Configuration Complete");

    vs1053->initialised=1;
    vs1053->playback=0;
    vs1053->datareq=0;
    
    /* Check VS10xx type */
    ssVer = ((ReadSci(vs1053, SCI_STATUS) >> 4) & 15);
    if (chipNumber[ssVer]) 
    {
        TRACE2("Chip is VS%d", chipNumber[ssVer]);
        if (chipNumber[ssVer] != 1053) 
        {
            ERROR("Incorrect chip\n");
            return ;
        }
    } 
    else 
    {
        TRACE2("Unknown VS10xx SCI_MODE field SS_VER = %d\n", ssVer);
        return;
    }
}

void SPI_Set_Baud_Prescaler(SPI_HandleTypeDef *hspi,uint32_t baud)
{
    hspi->Instance->CR1&=0xFFFFFFC7;
    hspi->Instance->CR1|=baud;
}

void VS1053_sdi_write_DMA(VS1053_InitTypeDef* vs1053,uint8_t* txbuff,uint16_t datasize)
{
    if(vs1053->initialised)
    {
        while(__HAL_SPI_GET_FLAG(vs1053->hspi, SPI_FLAG_BSY));
        while(!(HAL_GPIO_ReadPin(vs1053->DREQport,vs1053->DREQpin))); //Wait while DREQ is low
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_RESET);
        HAL_SPI_Transmit_DMA(vs1053->hspi,txbuff,datasize);
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->CSpin,GPIO_PIN_SET);
    }
}

void VS1053_sdi_write(VS1053_InitTypeDef* vs1053,uint8_t* txbuff,uint16_t datasize)
{
    if(vs1053->initialised)
    {

        while(!(HAL_GPIO_ReadPin(vs1053->DREQport,vs1053->DREQpin))); //Wait while DREQ is low
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_RESET);

        if(HAL_OK != HAL_SPI_Transmit(vs1053->hspi,txbuff,datasize,vs1053->timeout))//Write Data
        {
            ERROR("SPI TX FAILED");
            return;
        }
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_SET);

    }
}

void VS1053_sdi_write32(VS1053_InitTypeDef* vs1053,uint8_t* txbuff)
{
    int count = 0;
    if(vs1053->initialised)
    {
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_RESET);

        while (count++ < 32)
        {
            while(!(HAL_GPIO_ReadPin(vs1053->DREQport,vs1053->DREQpin))); //Wait while DREQ is low
            if(HAL_OK != HAL_SPI_Transmit(vs1053->hspi,txbuff++,1,vs1053->timeout))//Write Data
            {
                ERROR("SPI TX FAILED");
                return;
            }
        }
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_SET);

    }
}

void VS1053_sci_write_DMA(VS1053_InitTypeDef* vs1053,uint8_t addr,uint16_t data)
{
    uint8_t cmd=0x02;
    uint8_t result[2];
    result[0]=(data>>8)&0xFF;
    result[1]=data&0xFF;
    if(vs1053->initialised)
    {
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_RESET);

        HAL_SPI_Transmit_DMA(vs1053->hspi,&cmd,1);//Write command
        HAL_SPI_Transmit_DMA(vs1053->hspi,&addr,1);
        HAL_SPI_Transmit_DMA(vs1053->hspi,result,2);
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_SET);
        while(!(vs1053->datareq));
        vs1053->datareq=0;
    }
}

void VS1053_sci_write(VS1053_InitTypeDef* vs1053,uint8_t addr,uint16_t data)
{
    uint8_t cmd=0x02;
    uint8_t result[2];
    result[0]=(data>>8)&0xFF;
    result[1]=data&0xFF;
    if(vs1053->initialised)
    {
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_RESET);

        if(HAL_OK != HAL_SPI_Transmit(vs1053->hspi,&cmd,1,vs1053->timeout))//Write command
        {
            ERROR("SPI TX FAILED");
            return;
        }
        if(HAL_OK != HAL_SPI_Transmit(vs1053->hspi,&addr,1,vs1053->timeout))
        {
            ERROR("SPI TX FAILED");
            return;
        }
        if(HAL_OK != HAL_SPI_Transmit(vs1053->hspi,result,2,vs1053->timeout))
        {
            ERROR("SPI TX FAILED");
            return;
        }
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_SET);
        //while((HAL_GPIO_ReadPin(vs1053->DREQport,vs1053->DREQpin))); //Wait while DREQ is high
        while(!(HAL_GPIO_ReadPin(vs1053->DREQport,vs1053->DREQpin))); //Wait while DREQ is low
    }
}


uint16_t VS1053_sci_read(VS1053_InitTypeDef* vs1053,uint8_t addr)
{
    uint8_t cmd=0x03;
    uint8_t received[2];
    uint16_t result=0;
    if(vs1053->initialised)
    {
        
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_RESET);

        if(HAL_OK != HAL_SPI_Transmit(vs1053->hspi,&cmd,1,vs1053->timeout))//Read command
        {
            ERROR("SPI TX FAILED");
            return 0;
        }
        if(HAL_OK != HAL_SPI_Transmit(vs1053->hspi,&addr,1,vs1053->timeout))
        {
            ERROR("SPI TX FAILED");
            return 0;
        }
        HAL_Delay(1);
        if(HAL_OK != HAL_SPI_Receive(vs1053->hspi,received,2,vs1053->timeout))
        {
            ERROR("SPI RX FAILED");
            return 0;
        }
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_SET);
    }
    result=received[0]<<8|received[1];
    return result;
}



//Set Volume
void VS1053_sci_setattenuation(VS1053_InitTypeDef* vs1053,uint8_t l_chan,uint8_t r_chan)
{
    VS1053_sci_write(vs1053,SCI_VOL,(l_chan*256)+r_chan);
}

// Perform Soft Reset
void VS1053_SoftReset(VS1053_InitTypeDef* vs1053)
{
    /* Newmode, Reset, No L1-2 */
    VS1053_sci_write(vs1053, SCI_MODE, SM_RESET | SM_SDINEW );

    /* A quick sanity check: write to two registers, then test if we
       get the same results. Note that if you use a too high SPI
       speed, the MSB is the most likely to fail when read again. 
    */
    VS1053_sci_write(vs1053, SCI_AICTRL1, 0xABAD);
    VS1053_sci_write(vs1053, SCI_AICTRL2, 0x7E57);

    if (VS1053_sci_read(vs1053, SCI_AICTRL1) != 0xABAD || 
        VS1053_sci_read(vs1053, SCI_AICTRL2) != 0x7E57) 
    {
        ERROR("There is something wrong with VS10xx SCI registers");
        return;
    }

    /* Reset the values */
    VS1053_sci_write(vs1053, SCI_AICTRL1, 0x00);
    VS1053_sci_write(vs1053, SCI_AICTRL2, 0x00);

    /* Set clock register, doubler etc */
    VS1053_sci_write(vs1053, SCI_CLOCKF, 0xA000);

    /* Now when we have upped the VS10xx clock speed, the microcontroller
       SPI bus can run faster. Do that before you start playing or
       recording files. */

    LoadPlugin(vs1053, plugin, PLUGIN_SIZE);

    HAL_Delay(100);
}

// Send 2048 Zeros
void VS1053_SendZeros(VS1053_InitTypeDef* vs1053, uint8_t EndFillByte)
{
    uint16_t i;
    /* Send A zero */
    VS1053_sdi_write(vs1053,&EndFillByte,1);

    for (i=0; i<2052; i++)
    { 
        VS1053_sdi_write(vs1053,&EndFillByte,1);
    }

}

void VS1053_SineTest(VS1053_InitTypeDef* vs1053)
{
    uint8_t SineStartHeader[] = {0x53,0xEF,0x6E,0x44,0x00,0x00,0x00,0x00};
    uint8_t SineEndHeader[]   = {0x45,0x78,0x69,0x74,0x00,0x00,0x00,0x00};
    uint16_t i;
    uint16_t MP3SCI_MODE = VS1053_sci_read(vs1053,SCI_MODE);

    /* Newmode, Enable Test Mode */
    VS1053_sci_write(vs1053, SCI_MODE, MP3SCI_MODE | SM_TESTS);

    /* Write Sine Header to VS1053 chip so that Sine Sound starts playing */
    for (i=0; i<sizeof(SineStartHeader); i++)
    { 
        VS1053_sdi_write(vs1053,&SineStartHeader[i],1);
    }

    /* Wait for a couple of seconds to Let Sine Wave Play */
    HAL_Delay(2000);

    /* Send Sine End Header to VS1053 chip so that Sine Sound stops playing */
    for (i=0; i<sizeof(SineEndHeader); i++)
    { 
        VS1053_sdi_write(vs1053,&SineEndHeader[i],1);
    }

    HAL_Delay(500);

}


/* We also want to have the VS1053b Ogg Vorbis Encoder plugin. To get more
   than one plugin included, we'll have to include it in a slightly more
   tricky way. To get the plugin included below, download the latest version
   of the VS1053 Ogg Vorbis Encoder Application from
   http://www.vlsi.fi/en/support/software/vs10xxapplications.html */
/*#define SKIP_PLUGIN_VARNAME
const uint16_t encoderPlugin[] = {
#include "venc44k2q05.plg"
};
#undef SKIP_PLUGIN_VARNAME
*/

/* VS1053b IMA ADPCM Encoder Fix, available at
   http://www.vlsi.fi/en/support/software/vs10xxpatches.html */
/*#define SKIP_PLUGIN_VARNAME
const uint16_t imaFix[] = {
#include "imafix.plg"
};
#undef SKIP_PLUGIN_VARNAME
*/

#define FILE_BUFFER_SIZE            512
#define SDI_MAX_TRANSFER_SIZE       32
#define SDI_END_FILL_BYTES_FLAC     12288
#define SDI_END_FILL_BYTES          2050
#define REC_BUFFER_SIZE             512


/* How many transferred bytes between collecting data.
   A value between 1-8 KiB is typically a good value.
   If REPORT_ON_SCREEN is defined, a report is given on screen each time
   data is collected. */
#define REPORT_INTERVAL             4096
#define REPORT_INTERVAL_MIDI        512
#if 0
#define REPORT_ON_SCREEN
#endif

/* Define PLAYER_USER_INTERFACE if you want to have a user interface in your
   player. */
#if 0
#define PLAYER_USER_INTERFACE
#endif

/* Define RECORDER_USER_INTERFACE if you want to have a user interface in your
   player. */
#if 0
#define RECORDER_USER_INTERFACE
#endif




static const uint16_t linToDBTab[5] = {36781, 41285, 46341, 52016, 58386};

/*
  Converts a linear 16-bit value between 0..65535 to decibels.
    Reference level: 32768 = 96dB (largest VS1053b number is 32767 = 95dB).
  Bugs:
    - For the input of 0, 0 dB is returned, because minus infinity cannot
      be represented with integers.
    - Assumes a ratio of 2 is 6 dB, when it actually is approx. 6.02 dB.
*/
static uint16_t LinToDB(unsigned short n) {
  int res = 96, i;

  if (!n)               /* No signal should return minus infinity */
    return 0;

  while (n < 32768U) {  /* Amplify weak signals */
    res -= 6;
    n <<= 1;
  }

  for (i=0; i<5; i++)   /* Find exact scale */
    if (n >= linToDBTab[i])
      res++;

  return res;
}




/*

  Loads a plugin.

  This is a slight modification of the LoadUserCode() example
  provided in many of VLSI Solution's program packages.

*/
void LoadPlugin(VS1053_InitTypeDef* vs1053, const uint16_t *d, uint16_t len) {
  int i = 0;

  while (i<len) {
    unsigned short addr, n, val;
    addr = d[i++];
    n = d[i++];
    if (n & 0x8000U) { /* RLE run, replicate n samples */
      n &= 0x7FFF;
      val = d[i++];
      while (n--) {
        WriteSci(vs1053, addr, val);
      }
    } else {           /* Copy run, copy n samples */
      while (n--) {
        val = d[i++];
        WriteSci(vs1053, addr, val);
      }
    }
  }
}


void Set32(uint8_t *d, uint32_t n) {
  int i;
  for (i=0; i<4; i++) {
    *d++ = (uint8_t)n;
    n >>= 8;
  }
}

void Set16(uint8_t *d, uint16_t n) {
  int i;
  for (i=0; i<2; i++) {
    *d++ = (uint8_t)n;
    n >>= 8;
  }
}
