/**
 * @file   uart.c
 * @author Justin Tanner
 * @date   Sat Nov 22 21:32:03 2008
 *
 * @brief  UART Driver targetted for the AT90USB1287
 *
 */

#ifndef __UART_H__
#define __UART_H__

#include <avr/interrupt.h>
#include <stdint.h>

typedef enum _uart_bps
{
	UART_19200,
	UART_38400,
	UART_57600,
	UART_DEFAULT,
} UART_BPS;

#define UART_BUFFER_SIZE    32

#define UART_BAUD           57600

#define UCSR1A_CFG          (_BV(U2X1))
#define UCSR1B_CFG          (_BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1))
#define UCSR1C_CFG          (_BV(UCSZ11) | _BV(UCSZ10))

/// UBRRn = f_OSC * (1 + value(U2Xn)) / (16 * BAUD) - 1  in floating point
#define UBRR1_CFG	\
(((	\
	F_CPU * (((UCSR1A_CFG & _BV(U2X1)) >> U2X1) + 1)	\
	/ 8 / UART_BAUD	\
) - 1) / 2)

#define UBRR1H_CFG          (UBRR1_CFG >> 8)
#define UBRR1L_CFG          (UBRR1_CFG & 0xFF)

void uart_init(UART_BPS bitrate);
void uart_putchar(uint8_t byte);
uint8_t uart_get_byte(int index);
uint8_t uart_bytes_received(void);
void uart_reset_receive(void);

#endif
