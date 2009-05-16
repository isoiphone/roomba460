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

// see 2.4.3 of Using the AT90USBKey document
void set_leds(bool R1, bool G1, bool R2, bool G2)
{
	// Set the high nibble of port D to output
	DDRD |= 0xF0;

	// set all LEDs to off.
	PORTD &= ~0xF0;

	// set specific LED pins
	if (R1)
		PORTD |= _BV(PORTD4);
	if (G1)
		PORTD |= _BV(PORTD5);

	if (R2)
		PORTD |= _BV(PORTD7);
	if (G2)
		PORTD |= _BV(PORTD6);
}


int main(int argc, char *argv[])
{
	// red, red
	set_leds(true,false,true,false);

	// initialization
	allow_interupts(false);
		set_clock_frequency();
		uart_init(1);
		// amber, red
		set_leds(true,true,true,false);
		radio_init(SEND_ADDRESS, TRANSMIT_MODE);
		// amber, amber
		set_leds(true,true,true,true);

	allow_interupts(true);

	_delay_ms(10);
	uart_println("%c[2J", 0x1B);

	// green, green
	set_leds(false,true,false,true);

	char outBuffer[PAYLOAD_BYTES];
	memset(outBuffer, 0, PAYLOAD_BYTES);
	strncpy(outBuffer, "test", PAYLOAD_BYTES-1);

	int tick=0;
	bool flashLed = false;
	while (1) {
		uart_println("tick: %d\r\n",tick);
		tick++;

		radio_send(RECEIVE_ADDRESS, (uint8_t *)outBuffer);

		flashLed = !flashLed;
		set_leds(flashLed,true,false,true);

		_delay_ms(10);
	}

	return 0;
}
