#ifndef __UART_H__
#define __UART_H__

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <rd@uvic.ca> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Ron Desmarais
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, UART declarations
 *
 */

#include "common.h"

#define TX_BUFSIZE 80


ISR( USART1_TX_vect );
ISR( USART1_RX_vect );
ISR( USART1_UDRE_vect );


void uart_putstringln(char str[]);

void uart_println(char str[], ...);

int getBufferFree( void );
/*
 * Perform UART startup initialization.
 */
void uart_init(int m);

/*
 * Send one character to the UART.
 */
int	uart_putchar(char c);

/*
 * Size of internal line buffer used by uart_getchar().
 */


/*
 * Receive one character from the UART.  The actual reception is
 * line-buffered, and one character is returned from the buffer at
 * each invokation.
 */
char uart_getchar( void );

/*
 * Get a count of the number of missed messages that the uart missed
 * due to the message buffer being full
 */
int uart_getmissed( void );

int uart_getsent( void );

int uart_gettotal( void );

#endif

