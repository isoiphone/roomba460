/**
 * @file   uart.c
 * @author Justin Tanner
 * @date   Sat Nov 22 21:32:03 2008
 *
 * @brief  UART Driver targetted for the AT90USB1287
 *
 */
#include "uart.h"

static volatile uint8_t uart_buffer[UART_BUFFER_SIZE];
static volatile uint8_t uart_buffer_index;

/**
 * Initalize UART
 *
 */
void uart_init(UART_BPS bitrate)
{
	UCSR1A = UCSR1A_CFG;
	UCSR1B = UCSR1B_CFG;
	UCSR1C = UCSR1C_CFG;

	//UBRR1H = UBRR1H_CFG;
	//UBRR1L = UBRR1L_CFG;

	UBRR1H = 0;
	// see AT90 hardware manual p205, for table of UBRR1 values.
	switch (bitrate)
	{
	case UART_19200:
		UBRR1L = 51;
		break;
	case UART_38400:
		UBRR1L = 25;
		break;
	case UART_57600:
		UBRR1L = 16;
		break;
	default:
		UBRR1L = 16;
	}

    uart_buffer_index = 0;
}

/**
 * Transmit one byte
 * NOTE: This function uses busy waiting
 *
 * @param byte data to trasmit
 */
void uart_putchar(uint8_t byte)
{
    /* wait for empty transmit buffer */
    while (!( UCSR1A & (1 << UDRE1)));

    /* Put data into buffer, sends the data */
    UDR1 = byte;
}

/**
 * Receive a single byte from the receive buffer
 *
 * @param index
 *
 * @return
 */
uint8_t uart_get_byte(int index)
{
    if (index < UART_BUFFER_SIZE)
    {
        return uart_buffer[index];
    }
    return 0;
}

/**
 * Get the number of bytes received on UART
 *
 * @return number of bytes received on UART
 */
uint8_t uart_bytes_received(void)
{
    return uart_buffer_index;
}

/**
 * Prepares UART to receive another payload
 *
 */
void uart_reset_receive(void)
{
    uart_buffer_index = 0;
}

/**
 * UART receive byte ISR
 */
ISR(USART1_RX_vect)
{
    uart_buffer[uart_buffer_index] = UDR1;
    uart_buffer_index = (uart_buffer_index + 1) % UART_BUFFER_SIZE;
}





