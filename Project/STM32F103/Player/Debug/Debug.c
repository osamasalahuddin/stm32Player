#include "Debug.h"
#include "stm32f1xx_nucleo.h"


/* UART handler declaration */
extern UART_HandleTypeDef UartHandle;

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
    while(1)
    {
    }
}

/* This function is used to transmit a string of characters via
 * the USART specified in USARTx.
 *
 * It takes two arguments: USARTx --> can be any of the USARTs e.g. USART1, USART2 etc.
 *                         (volatile) char *s is the string you want to send
 *
 * Note: The string has to be passed to the function as a pointer because
 *       the compiler doesn't know the 'string' data type. In standard
 *       C a string is just an array of characters
 *
 * Note 2: At the moment it takes a volatile char because the received_string variable
 *         declared as volatile char --> otherwise the compiler will spit out warnings
 * */
void USART_puts(UART_HandleTypeDef* UartHandle, volatile char *s)
{
    int size = 0;
    char *a;
    a = (char *)s;
    while(*a != (int) NULL)
    {
        size++;
        a++;
    }
    while(HAL_UART_Transmit(UartHandle, (uint8_t *)s, size, 0xFFFFFFFF)!=HAL_OK);
}

/*
 *  This function is used by printf library
*/
void mputc(void *p, char c)
{
    char* d;
    d = &c;
    USART_puts((UART_HandleTypeDef*) p, d);
}

void assert_failed(uint8_t* file, uint32_t line)
{
    sprintf(timestamp,"%d:02%d:%02d.%02d",sysTime.hr,sysTime.min,sysTime.sec,sysTime.milli);
    printf("[ASSERT_FAILED][%s]%s(%d): ",timestamp,file,line);
    while(1)
    {
        Error_Handler();
    }   /* Stay here forever */
}

/* USART2 init function */
void MX_USART2_UART_Init(int BaudRate,UART_HandleTypeDef* UartHandle)
{

    UartHandle->Instance                  = USART2;
    UartHandle->Init.BaudRate             = BaudRate;
    UartHandle->Init.WordLength           = UART_WORDLENGTH_8B;
    UartHandle->Init.StopBits             = UART_STOPBITS_1;
    UartHandle->Init.Parity               = UART_PARITY_NONE;
    UartHandle->Init.Mode                 = UART_MODE_TX_RX;
    UartHandle->Init.HwFlowCtl            = UART_HWCONTROL_NONE;
    UartHandle->Init.OverSampling         = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(UartHandle) != HAL_OK)
    {
        while(1)
        {
            HAL_Delay(500);
        }
    }
}

/* Initialize Debug Functionality */
void init_debug()
{
    MX_USART2_UART_Init(115200, &UartHandle);
}
