#include "common.h"
#include "uart.h"
#include "radio.h"

void allow_interupts(bool allow)
{
	if (allow) {
		asm volatile ("sei"::);
	} else {
		asm volatile ("cli"::);
	}
}


void set_clock_frequency()
{
	// Disable default prescaler to make processor speed 8 MHz
	CLKPR = (1<<CLKPCE);
	CLKPR = 0x00;
}



int main(int argc, char *argv[])
{
	// initialization
	allow_interupts(false);

		set_clock_frequency();
		uart_init(1);
		radio_init(SEND_ADDRESS, TRANSMIT_MODE);

	allow_interupts(true);

	_delay_ms(10);
	uart_println("%c[2J", 0x1B);

    //PORTD ^= LED_D5_RED;
    //DDRD |= LED_D5_RED;

	int tick=0;
	while (1) {
		uart_println("tick: %d\r\n",tick);
		tick++;

		// Send the result over the radio
		//char output[5];
		//memset(output, 0, 5);
		//sprintf(output, "%d", temp);
		//radio_send(RECEIVE_ADDRESS, (uint8_t *)output);

		_delay_ms(10);
	}

	return 0;
}
