#include "global.h"

#include "i2csw.h"

#include <util/twi.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define     clock8MHz()    CLKPR = _BV(CLKPCE); CLKPR = 0x00;

int main( void )
{
	BYTE addr = 0x09;
	BYTE msg[] = {0xFF, 0xFF, 0xFF}; 

	clock8MHz();

	i2cInit();

	while (1) {
		i2cSend(addr, 'n', 3, msg); 
		_delay_ms(10);
	}
}

