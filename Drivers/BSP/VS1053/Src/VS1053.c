//VS1053 STM32 Driver
#include "stm32f1xx_hal.h"
#include "Debug.h"
#include "VS1053.h"



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
    HAL_GPIO_WritePin(vs1053->RSTport,vs1053->RSTpin,GPIO_PIN_RESET);     //RESET Low
    HAL_GPIO_WritePin(vs1053->CSport, vs1053->CSpin, GPIO_PIN_RESET);     //xCS Low
    HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_RESET);     //xDCS Low
    HAL_Delay(10);

    /* Send Dummy Data on SPI bus */
    if(HAL_OK != HAL_SPI_Transmit(vs1053->hspi,&tempData,1,vs1053->timeout))
    {
        ERROR("SPI TX FAILED");
        return;
    }

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
        HAL_GPIO_WritePin(vs1053->CSport,vs1053->CSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_SET);
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->DCSpin,GPIO_PIN_RESET);
        HAL_SPI_Transmit(vs1053->hspi,txbuff,datasize,vs1053->timeout);
        HAL_GPIO_WritePin(vs1053->DCSport,vs1053->CSpin,GPIO_PIN_SET);
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
        while((HAL_GPIO_ReadPin(vs1053->DREQport,vs1053->DREQpin))); //Wait while DREQ is high
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

void VS1053_SoftReset(VS1053_InitTypeDef* vs1053)
{
    /* Newmode, Reset, No L1-2 */
    VS1053_sci_write(vs1053, SCI_MODE, 0x0804);

    /* Set clock register, doubler etc */
    VS1053_sci_write(vs1053, SCI_CLOCKF, 0x0a00);

}