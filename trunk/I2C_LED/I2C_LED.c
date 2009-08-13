#include "global.h"
#include "i2c.h"

#include <avr/interrupt.h>
#include <util/delay.h>

#define     clock8MHz()    CLKPR = _BV(CLKPCE); CLKPR = 0x00;


int main( void )
{
	clock8MHz();

	i2cInit();

	unsigned char cmd[] = {'n', 0xCC, 0x00, 0xFF};
	unsigned char cmdLen = 4;

	while (1) {
		i2cMasterSend(0x00, cmdLen, cmd);
		_delay_ms(10);
	}
}

