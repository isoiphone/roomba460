#ifndef __COMMON_H__
#define __COMMON_H__

#define F_CPU 8000000UL  // 8 MHz

// Wireless Radio Addresses
#define RECEIVE_ADDRESS	0x0042
#define SEND_ADDRESS	0x0052

// Blinky indicator
#define LED_D5_RED        0x80

#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


#endif
