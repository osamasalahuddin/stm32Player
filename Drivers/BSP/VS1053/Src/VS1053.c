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


void VS1053_configure(VS1053_InitTypeDef* vs1053,SPI_HandleTypeDef* hspi,
                      GPIO_TypeDef* DREQport,uint16_t DREQpin,
                      GPIO_TypeDef* CSport,uint16_t CSpin,
                      GPIO_TypeDef* DCSport,uint16_t DCSpin,
                      GPIO_TypeDef* RSTport,uint16_t RSTpin,
                      uint32_t timeout)
{
    uint8_t tempData    = 0xFF;

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

    /* Set clock register, doubler etc */
    VS1053_sci_write(vs1053, SCI_CLOCKF, 0x0a00);

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


#define min(a,b) (((a)<(b))?(a):(b))



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


/*
  Read 32-bit increasing counter value from addr.
  Because the 32-bit value can change while reading it,
  read MSB's twice and decide which is the correct one.
*/
uint32_t ReadVS10xxMem32Counter(VS1053_InitTypeDef* vs1053, uint16_t addr) 
{
  uint16_t msbV1, lsb, msbV2;
  uint32_t res;

  WriteSci(vs1053, SCI_WRAMADDR, addr+1);
  msbV1 = ReadSci(vs1053, SCI_WRAM);
  WriteSci(vs1053, SCI_WRAMADDR, addr);
  lsb = ReadSci(vs1053, SCI_WRAM);
  msbV2 = ReadSci(vs1053, SCI_WRAM);
  if (lsb < 0x8000U) {
    msbV1 = msbV2;
  }
  res = ((uint32_t)msbV1 << 16) | lsb;
  
  return res;
}


/*
  Read 32-bit non-changing value from addr.
*/
uint32_t ReadVS10xxMem32(VS1053_InitTypeDef* vs1053, uint16_t addr) {
  uint16_t lsb;
  WriteSci(vs1053, SCI_WRAMADDR, addr);
  lsb = ReadSci(vs1053, SCI_WRAM);
  return lsb | ((uint32_t)ReadSci(vs1053, SCI_WRAM) << 16);
}


/*
  Read 16-bit value from addr.
*/
uint16_t ReadVS10xxMem(VS1053_InitTypeDef* vs1053, uint16_t addr) {
  WriteSci(vs1053, SCI_WRAMADDR, addr);
  return ReadSci(vs1053, SCI_WRAM);
}


/*
  Write 16-bit value to given VS10xx address
*/
void WriteVS10xxMem(VS1053_InitTypeDef* vs1053, uint16_t addr, uint16_t data) {
  WriteSci(vs1053, SCI_WRAMADDR, addr);
  WriteSci(vs1053, SCI_WRAM, data);
}

/*
  Write 32-bit value to given VS10xx address
*/
void WriteVS10xxMem32(VS1053_InitTypeDef* vs1053, uint16_t addr, uint32_t data) {
  WriteSci(vs1053, SCI_WRAMADDR, addr);
  WriteSci(vs1053, SCI_WRAM, (uint16_t)data);
  WriteSci(vs1053, SCI_WRAM, (uint16_t)(data>>16));
}




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









enum PlayerStates {
  psPlayback = 0,
  psUserRequestedCancel,
  psCancelSentToVS10xx,
  psStopped
} playerState;





/*

  This function plays back an audio file.

  It also contains a simple user interface, which requires the following
  funtions that you must provide:
  void SaveUIState(void);
  - saves the user interface state and sets the system up
  - may in many cases be implemented as an empty function
  void RestoreUIState(void);
  - Restores user interface state before exit
  - may in many cases be implemented as an empty function
  int GetUICommand(void);
  - Returns -1 for no operation
  - Returns -2 for cancel playback command
  - Returns any other for user input. For supported commands, see code.

*/
void VS1053PlayFile(VS1053_InitTypeDef* vs1053, FIL *readFp) {
  static uint8_t playBuf[FILE_BUFFER_SIZE];
  uint32_t bytesInBuffer;        // How many bytes in buffer left
  uint32_t pos=0;                // File position
  int endFillByte = 0;          // What byte value to send after file
  int endFillBytes = SDI_END_FILL_BYTES; // How many of those to send
  int playMode = ReadVS10xxMem(vs1053, PAR_PLAY_MODE);
  long nextReportPos=0; // File pointer where to next collect/report
  int i;
#ifdef PLAYER_USER_INTERFACE
  static int earSpeaker = 0;    // 0 = off, other values strength
  int volLevel = ReadSci(vs1053, SCI_VOL) & 0xFF; // Assume both channels at same level
  int c;
  static int rateTune = 0;      // Samplerate fine tuning in ppm
#endif /* PLAYER_USER_INTERFACE */

#ifdef PLAYER_USER_INTERFACE
  SaveUIState();
#endif /* PLAYER_USER_INTERFACE */

  playerState = psPlayback;             // Set state to normal playback

  WriteSci(vs1053, SCI_DECODETIME, 0);         // Reset DECODE_TIME


  /* Main playback loop */

  f_read(readFp, playBuf, FILE_BUFFER_SIZE, &bytesInBuffer);
  while ((bytesInBuffer > 0) &&
         playerState != psStopped) {
    uint8_t *bufP = playBuf;

    while (bytesInBuffer && playerState != psStopped) {

      if (!(playMode & PAR_PLAY_MODE_PAUSE_ENA)) {
        int t = min(SDI_MAX_TRANSFER_SIZE, bytesInBuffer);

        // This is the heart of the algorithm: on the following line
        // actual audio data gets sent to VS10xx.
        WriteSdi(vs1053, bufP, t);

        bufP += t;
        bytesInBuffer -= t;
        pos += t;
      }

      /* If the user has requested cancel, set VS10xx SM_CANCEL bit */
      if (playerState == psUserRequestedCancel) {
        unsigned short oldMode;
        playerState = psCancelSentToVS10xx;
        printf("\nSetting SM_CANCEL at file offset %ld\n", pos);
        oldMode = ReadSci(vs1053, SCI_MODE);
        WriteSci(vs1053, SCI_MODE, oldMode | SM_CANCEL);
      }

      /* If VS10xx SM_CANCEL bit has been set, see if it has gone
         through. If it is, it is time to stop playback. */
      if (playerState == psCancelSentToVS10xx) {
        unsigned short mode = ReadSci(vs1053, SCI_MODE);
        if (!(mode & SM_CANCEL)) {
          printf("SM_CANCEL has cleared at file offset %ld\n", pos);
          playerState = psStopped;
        }
      }


      /* If playback is going on as normal, see if we need to collect and
         possibly report */
      if (playerState == psPlayback && pos >= nextReportPos) {
#ifdef REPORT_ON_SCREEN
        uint16_t sampleRate;
        uint32_t byteRate;
        uint16_t h1 = ReadSci(vs1053, SCI_HDAT1);
#endif

        nextReportPos += (audioFormat == afMidi || audioFormat == afUnknown) ?
          REPORT_INTERVAL_MIDI : REPORT_INTERVAL;
        /* It is important to collect endFillByte while still in normal
           playback. If we need to later cancel playback or run into any
           trouble with e.g. a broken file, we need to be able to repeatedly
           send this byte until the decoder has been able to exit. */
        endFillByte = ReadVS10xxMem(vs1053, PAR_END_FILL_BYTE);

#ifdef REPORT_ON_SCREEN
        if (h1 == 0x7665) {
          audioFormat = afRiff;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x4154) {
          audioFormat = afAacAdts;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x4144) {
          audioFormat = afAacAdif;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x574d) {
          audioFormat = afWma;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x4f67) {
          audioFormat = afOggVorbis;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x664c) {
          audioFormat = afFlac;
          endFillBytes = SDI_END_FILL_BYTES_FLAC;
        } else if (h1 == 0x4d34) {
          audioFormat = afAacMp4;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x4d54) {
          audioFormat = afMidi;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if ((h1 & 0xffe6) == 0xffe2) {
          audioFormat = afMp3;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if ((h1 & 0xffe6) == 0xffe4) {
          audioFormat = afMp2;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if ((h1 & 0xffe6) == 0xffe6) {
          audioFormat = afMp1;
          endFillBytes = SDI_END_FILL_BYTES;
        } else {
          audioFormat = afUnknown;
          endFillBytes = SDI_END_FILL_BYTES_FLAC;
        }

        sampleRate = ReadSci(vs1053, SCI_AUDATA);
        byteRate = ReadVS10xxMem(vs1053, PAR_BYTERATE);
        /* FLAC:   byteRate = bitRate / 32
           Others: byteRate = bitRate /  8
           Here we compensate for that difference. */
        if (audioFormat == afFlac)
          byteRate *= 4;

        printf("\r%ldKiB "
               "%1ds %1.1f"
               "kb/s %dHz %s %s"
               " %04x   ",
               pos/1024,
               ReadSci(vs1053, SCI_DECODE_TIME),
               byteRate * (8.0/1000.0),
               sampleRate & 0xFFFE, (sampleRate & 1) ? "stereo" : "mono",
               afName[audioFormat], h1
               );
          
        fflush(stdout);
#endif /* REPORT_ON_SCREEN */
      }
      f_read(readFp, playBuf, FILE_BUFFER_SIZE, &bytesInBuffer);
    } /* if (playerState == psPlayback && pos >= nextReportPos) */
  


    /* User interface. This can of course be completely removed and
       basic playback would still work. */

#ifdef PLAYER_USER_INTERFACE
    /* GetUICommand should return -1 for no command and -2 for CTRL-C */
    c = GetUICommand();
    switch (c) {

      /* Volume adjustment */
    case '-':
      if (volLevel < 255) {
        volLevel++;
        WriteSci(SCI_VOL, volLevel*0x101);
      }
      break;
    case '+':
      if (volLevel) {
        volLevel--;
        WriteSci(SCI_VOL, volLevel*0x101);
      }
      break;

      /* Show some interesting registers */
    case '_':
      {
        uint32_t mSec = ReadVS10xxMem32Counter(PAR_POSITION_MSEC);
        printf("\nvol %1.1fdB, MODE %04x, ST %04x, "
               "HDAT1 %04x HDAT0 %04x\n",
               -0.5*volLevel,
               ReadSci(SCI_MODE),
               ReadSci(SCI_STATUS),
               ReadSci(SCI_HDAT1),
               ReadSci(SCI_HDAT0));
        printf("  sampleCounter %lu, ",
               ReadVS10xxMem32Counter(0x1800));
        if (mSec != 0xFFFFFFFFU) {
          printf("positionMSec %lu, ", mSec);
        }
        printf("config1 0x%04x", ReadVS10xxMem(PAR_CONFIG1));
        printf("\n");
      }
      break;

      /* Adjust play speed between 1x - 4x */
    case '1':
    case '2':
    case '3':
    case '4':
      /* FF speed */
      printf("\nSet playspeed to %dX\n", c-'0');
      WriteVS10xxMem(PAR_PLAY_SPEED, c-'0');
      break;

      /* Ask player nicely to stop playing the song. */
    case 'q':
      if (playerState == psPlayback)
        playerState = psUserRequestedCancel;
      break;

      /* Forceful and ugly exit. For debug uses only. */
    case 'Q':
      RestoreUIState();
      printf("\n");
      exit(EXIT_SUCCESS);
      break;

      /* EarSpeaker spatial processing adjustment. */
    case 'e':
      earSpeaker = (earSpeaker+1) & 3;
      {
        uint16_t t = ReadSci(SCI_MODE) & ~(SM_EARSPEAKER_LO|SM_EARSPEAKER_HI);
        if (earSpeaker & 1)
          t |= SM_EARSPEAKER_LO;
        if (earSpeaker & 2)
          t |= SM_EARSPEAKER_HI;
        WriteSci(SCI_MODE, t);
      }
      printf("\nSet earspeaker to %d\n", earSpeaker);
      break;

      /* Toggle mono mode. Implemented in the VS1053b Patches package */
    case 'm':
      playMode ^= PAR_PLAY_MODE_MONO_ENA;
      printf("\nMono mode %s\n",
             (playMode & PAR_PLAY_MODE_MONO_ENA) ? "on" : "off");
      WriteVS10xxMem(PAR_PLAY_MODE, playMode);
      break;

      /* Toggle differential mode */
    case 'd':
      {
        uint16_t t = ReadSci(SCI_MODE) ^ SM_DIFF;
        printf("\nDifferential mode %s\n", (t & SM_DIFF) ? "on" : "off");
        WriteSci(SCI_MODE, t);
      }
      break;

      /* Adjust playback samplerate finetuning, this function comes from
         the VS1053b Patches package. Note that the scale is different
         in VS1053b and VS1063a! */
    case 'r':
      if (rateTune >= 0) {
        rateTune = (rateTune*0.95);
      } else {
        rateTune = (rateTune*1.05);
      }
      rateTune -= 1;
     if (rateTune < -160000)
        rateTune = -160000;
      WriteVS10xxMem(0x5b1c, 0);                 /* From VS105b Patches doc */
      WriteSci(SCI_AUDATA, ReadSci(SCI_AUDATA)); /* From VS105b Patches doc */
      WriteVS10xxMem32(PAR_RATE_TUNE, rateTune);
      printf("\nrateTune %d ppm*2\n", rateTune);
      break;
    case 'R':
      if (rateTune <= 0) {
        rateTune = (rateTune*0.95);
      } else {
        rateTune = (rateTune*1.05);
      }
      rateTune += 1;
      if (rateTune > 160000)
        rateTune = 160000;
      WriteVS10xxMem32(PAR_RATE_TUNE, rateTune);
      WriteVS10xxMem(0x5b1c, 0);                 /* From VS105b Patches doc */
      WriteSci(SCI_AUDATA, ReadSci(SCI_AUDATA)); /* From VS105b Patches doc */
      printf("\nrateTune %d ppm*2\n", rateTune);
      break;
    case '/':
      rateTune = 0;
      WriteVS10xxMem(SCI_WRAMADDR, 0x5b1c);      /* From VS105b Patches doc */
      WriteVS10xxMem(0x5b1c, 0);                 /* From VS105b Patches doc */
      WriteVS10xxMem32(PAR_RATE_TUNE, rateTune);
      printf("\nrateTune off\n");
      break;

      /* Show help */
    case '?':
      printf("\nInteractive VS1053 file player keys:\n"
             "1-4\tSet playback speed\n"
             "- +\tVolume down / up\n"
             "_\tShow current settings\n"
             "q Q\tQuit current song / program\n"
             "e\tSet earspeaker\n"
             "r R\tR rateTune down / up\n"
             "/\tRateTune off\n"
             "m\tToggle Mono\n"
             "d\tToggle Differential\n"
             );
      break;

      /* Unknown commands or no command at all */
    default:
      if (c < -1) {
        printf("Ctrl-C, aborting\n");
        fflush(stdout);
        RestoreUIState();
        exit(EXIT_FAILURE);
      }
      if (c >= 0) {
        printf("\nUnknown char '%c' (%d)\n", isprint(c) ? c : '.', c);
      }
      break;
    } /* switch (c) */
#endif /* PLAYER_USER_INTERFACE */
  } /* while ((bytesInBuffer = fread(...)) > 0 && playerState != psStopped) */


  
#ifdef PLAYER_USER_INTERFACE
  RestoreUIState();
#endif /* PLAYER_USER_INTERFACE */

  printf("\nSending %d footer %d's... ", endFillBytes, endFillByte);
  fflush(stdout);

  /* Earlier we collected endFillByte. Now, just in case the file was
     broken, or if a cancel playback command has been given, write
     lots of endFillBytes. */
  memset(playBuf, endFillByte, sizeof(playBuf));
  for (i=0; i<endFillBytes; i+=SDI_MAX_TRANSFER_SIZE) {
    WriteSdi(vs1053, playBuf, SDI_MAX_TRANSFER_SIZE);
  }

  /* If the file actually ended, and playback cancellation was not
     done earlier, do it now. */
  if (playerState == psPlayback) {
    unsigned short oldMode = ReadSci(vs1053, SCI_MODE);
    WriteSci(vs1053, SCI_MODE, oldMode | SM_CANCEL);
    printf("ok. Setting SM_CANCEL, waiting... ");
    fflush(stdout);
    while (ReadSci(vs1053, SCI_MODE) & SM_CANCEL)
      WriteSdi(vs1053, playBuf, 2);
  }

  /* That's it. Now we've played the file as we should, and left VS10xx
     in a stable state. It is now safe to call this function again for
     the next song, and again, and again... */
  printf("ok\n");
}









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


/*

  Hardware Initialization for VS1053.

  
*/
int VSTestInitHardware(void) {
  /* Write here your microcontroller code which puts VS10xx in hardware
     reset anc back (set xRESET to 0 for at least a few clock cycles,
     then to 1). */
  return 0;
}



/* Note: code SS_VER=2 is used for both VS1002 and VS1011e */
const uint16_t chipNumber[16] = {
  1001, 1011, 1011, 1003, 1053, 1033, 1063, 1103,
  0, 0, 0, 0, 0, 0, 0, 0
};

/*

  Software Initialization for VS1053.

  Note that you need to check whether SM_SDISHARE should be set in
  your application or not.
  
*/
int VSTestInitSoftware(VS1053_InitTypeDef* vs1053) {
  uint16_t ssVer;

  /* Start initialization with a dummy read, which makes sure our
     microcontoller chips selects and everything are where they
     are supposed to be and that VS10xx's SCI bus is in a known state. */
  ReadSci(vs1053, SCI_MODE);

  /* First real operation is a software reset. After the software
     reset we know what the status of the IC is. You need, depending
     on your application, either set or not set SM_SDISHARE. See the
     Datasheet for details. */
  WriteSci(vs1053, SCI_MODE, SM_SDINEW|SM_TESTS|SM_RESET);

  /* A quick sanity check: write to two registers, then test if we
     get the same results. Note that if you use a too high SPI
     speed, the MSB is the most likely to fail when read again. */
  WriteSci(vs1053, SCI_AICTRL1, 0xABAD);
  WriteSci(vs1053, SCI_AICTRL2, 0x7E57);
  if (ReadSci(vs1053, SCI_AICTRL1) != 0xABAD || ReadSci(vs1053, SCI_AICTRL2) != 0x7E57) {
    printf("There is something wrong with VS10xx SCI registers\n");
    return 1;
  }
  WriteSci(vs1053, SCI_AICTRL1, 0);
  WriteSci(vs1053, SCI_AICTRL2, 0);

  /* Check VS10xx type */
  ssVer = ((vs1053, ReadSci(vs1053, SCI_STATUS) >> 4) & 15);
  if (chipNumber[ssVer]) {
    TRACE2("Chip is VS%d\n", chipNumber[ssVer]);
    if (chipNumber[ssVer] != 1053) {
      ERROR("Incorrect chip\n");
      return 1;
    }
  } else {
    TRACE2("Unknown VS10xx SCI_MODE field SS_VER = %d\n", ssVer);
    return 1;
  }

  /* Set the clock. Until this point we need to run SPI slow so that
     we do not exceed the maximum speeds mentioned in
     Chapter SPI Timing Diagram in the Datasheet. */
  WriteSci(vs1053, SCI_CLOCKF,
           HZ_TO_SC_FREQ(12288000) | SC_MULT_53_35X | SC_ADD_53_10X);


  /* Now when we have upped the VS10xx clock speed, the microcontroller
     SPI bus can run faster. Do that before you start playing or
     recording files. */

  /* Set up other parameters. */
  WriteVS10xxMem(vs1053, PAR_CONFIG1, PAR_CONFIG1_AAC_SBR_SELECTIVE_UPSAMPLE);

  /* Set volume level at -6 dB of maximum */
  WriteSci(vs1053, SCI_VOL, 0x0c0c);

  /* Now it's time to load the proper patch set. */
  LoadPlugin(vs1053, plugin, sizeof(plugin)/sizeof(plugin[0]));

  /* We're ready to go. */
  return 0;
}





/*
  Main function that activates either playback or recording.
*/
int VSTestHandleFile(VS1053_InitTypeDef* vs1053, const char *fileName, int record) {
  if (!record) {
    FIL file;
    FRESULT res;
    res = f_open(&file, fileName, FA_OPEN_EXISTING | FA_READ);
    printf("Play file %s\n", fileName);
    if (!res) {
      VS1053PlayFile(vs1053, &file);
    } else {
      printf("Failed opening %s for reading\n", fileName);
      return -1;
    }
  } else {
  }
  return 0;
}
