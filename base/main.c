#include "common.h"
#include "awesomedelay.h"
#include "uart.h"
#include "radio.h"
#include "roomba_sci.h"
#include "timer.h"

void allow_interupts(bool allow)
{
	if (allow)
	{
		asm volatile ("sei"::);
	}
	else
	{
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


/*
void init_joystick()
{
	DDRE &= ~_BV(PORTE5);
	PORTE |= _BV(PORTE5);
}


void poll_joystick()
{
	if (PINE & _BV(PORTE5))
	{
		uart_putchar('1');
	}
	else
	{
		uart_putchar('0');
	}
}
*/


volatile uint8_t rxflag = 0;


void radio_rxhandler(uint8_t pipenumber)
{
	rxflag = 1;
	//PORTD ^= _BV(PD5);

	set_leds(true,true,true,true);
	_delay_ms(10);
	set_leds(false,true,false,true);
	_delay_ms(10);
}


uint8_t my_addr[5] = { 0x77, 0x77, 0x77, 0x77, 0x77 };
uint8_t roomba_addr[5] = { 0xED, 0xB7, 0xED, 0xB7, 0xED};

int main(int argc, char *argv[])
{
	set_leds(true,false,true,false); // red, red

	// initialization
	allow_interupts(false);
		set_clock_frequency();
		Timer_Init();
		uart_init(1);
		
		set_leds(true,true,true,false); // amber, red

		Radio_Init();
		Radio_Configure_Rx(RADIO_PIPE_0, my_addr, ENABLE);
		Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);

		set_leds(true,true,true,true); // amber, amber
		//init_joystick();
	allow_interupts(true);

	_delay_ms(10);
	uart_println("%c[2J", 0x1B);

	// green, green
	set_leds(false,true,false,true);

	// direct messages to roomba
	Radio_Set_Tx_Addr(roomba_addr);

	uint16_t tick=0;
	bool flashLed = false;

	while (1)
	{
/*
		//poll_joystick();
		uart_println("tick: %d\r\n",tick);
		tick++;

		radiopacket_t packet;

		// listen
		while (rxflag)
		{
			memset(&packet, 0, sizeof(packet));

			if (Radio_Receive(&packet) != RADIO_RX_MORE_PACKETS)
			{
				rxflag = 0;
			}
			
			uart_println("received something.\r\n");
		}

		// create something interesting to send
		memset(&packet, 0, sizeof(packet));
		packet.type = COMMAND;
		memcpy(&packet.payload.command.sender_address, my_addr, 5);

		// spin me right round baby
		packet.payload.command.command = DRIVE;
		packet.payload.command.num_arg_bytes = 4;
		packet.payload.command.arguments[0] = 0;
		packet.payload.command.arguments[1] = 200;
		packet.payload.command.arguments[2] = 255;
		packet.payload.command.arguments[3] = 255;

		Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);

		flashLed = !flashLed;
		set_leds(flashLed,true,false,true);

		_delay_160ms();
*/
		uint16_t time1 = Timer_Now();
		_delay_ms(160);
		uint16_t time2 = Timer_Now();
		
		uart_println("160ms delaz: %d\r\n",time2 - time1);
	}

	return 0;
}
