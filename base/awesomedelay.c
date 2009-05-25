
#include "common.h"
#include <util/delay.h>

void _delay_160ms(void)
{
	// The maximum delay supported by _delay_ms is 262/8 = 32.75 ms on an 8 MHz controller.
	_delay_ms(32); _delay_ms(32); _delay_ms(32); _delay_ms(32); _delay_ms(32);
}


void _delay_500ms()
{
	_delay_160ms();
	_delay_160ms();
	_delay_160ms();
	_delay_ms(20);
}


void _delay_1s()
{
	_delay_500ms();
	_delay_500ms();
}

