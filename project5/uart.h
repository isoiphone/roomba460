/**
 * @file   uart.h
 *
 * 
 * @brief UART Serial Connection
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 *
 * @author Scott Craig
 * @author Justin Tanner
 */

#ifndef __UAT_INT_H__
#define __UAT_INT_H__

#include <avr/interrupt.h>

#include "OS/common.h"

#define BAUD            9600

#define UART_TX_BUF_MASK    255

#if (UART_TX_BUF_MASK & (UART_TX_BUF_MASK + 1))
#error UART_TX_BUF_MASK must be 1 less than a power of 2
#endif

#define UCSR1A_CFG        (_BV(U2X1))

#define UCSR1B_CFG        ( _BV(TXEN1))

#define UCSR1C_CFG        (_BV(UCSZ11) | _BV(UCSZ10))

/** UBn = f_OSC * (1 + value(U2Xn)) / (16 * BAUD) - 1  in floating point arithmetic. */
#define UBRR1_CFG    \
(((    \
    F_CPU * (((UCSR1A_CFG & _BV(U2X1)) >> U2X1) + 1)    \
    / 8 / BAUD    \
) - 1) / 2)

#define UBRR1H_CFG        (UBRR1_CFG >> 8)
#define UBRR1L_CFG        (UBRR1_CFG & 0xFF)

#define TXIntEnable()    UCSR1B |= _BV(UDRIE1)
#define TXIntDisable()    UCSR1B &= ~(_BV(UDRIE1))
#define Is_TXIntDisabled()    (!(UCSR1B & _BV(UDRIE1)))

/** Initialize the uat dive */
void uart_init(void);
/** Write data to the uart. */
int uart_write(uint8_t* const str, int len);

#endif
