/**
 * @file   uart.c
 *
 * @brief   UART Serial Connection
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 * @author Scott Craig
 * @author Justin Tanner
 */

#include "uart.h"
#include <string.h>

static uint8_t uart_TX_buf[UART_TX_BUF_MASK + 1];


/** The position in the array of the next byte to be consumed. */
static int head = 0;
/** The position in the array of the next byte to inserted. */
static int tail = 0;
/** boolean to indicate buffer not empty */
static int non_empty = 0;


/**
 * Initialize UART
 *
 */
void uart_init(void)
{
    UCSR1A = UCSR1A_CFG;
    UCSR1B = UCSR1B_CFG;
    UCSR1C = UCSR1C_CFG;

    UBRR1H = UBRR1H_CFG;
    UBRR1L = UBRR1L_CFG;

    head = 0;
    tail = 0;
    non_empty = 0;
}


/**
 * Copies a string into a buffer for the Uart
 * interrupt handler to write to the terminal.
 * If overflow occurs, older data is overwritten.
 *
 * @param str The string you want to send to the UART hyperterminal.
 * @param len length of the string
 *
 * @return -1 if overflow occurred.
 */
int uart_write(uint8_t* const str, int len)
{
    int overflow = 0;
    int i;
    uint8_t sreg;

    sreg = SREG;
    Disable_Interrupt();

    overflow = 0;
    for(i = 0; i < len; ++i)
    {
        if(non_empty && head == tail) overflow = -1;

        uart_TX_buf[tail] = str[i];

        ++tail;
        tail &= UART_TX_BUF_MASK;
    }

    if(overflow) head = tail;

    if(len > 0)
    {
        TXIntEnable();
        non_empty = 1;
    }

    SREG = sreg;

    return overflow;
}


/**
 * Interrupt service routine for the UART transmission.
 */
ISR(USART1_UDRE_vect)
{
    UDR1 = uart_TX_buf[head];

    ++head;
    head &= UART_TX_BUF_MASK;

    /* Last byte was written? */
    if (head == tail)
    {
        non_empty = 0;
        TXIntDisable();
    }
}


