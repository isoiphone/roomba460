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


//void init_joystick()
//{
	//DDRE &= ~_BV(PORTE5);
	//PORTE |= _BV(PORTE5);
//}


//void poll_joystick()
//{
	//if (PINE & _BV(PORTE5)) {
		//uart_putchar('1');
	//} else {
		//uart_putchar('0');
	//}
//}


volatile uint8_t rxflag = 0;


void radio_rxhandler(uint8_t pipenumber)
{
	rxflag = 1;
//	PORTD ^= _BV(PD5);


	set_leds(true,true,true,true);
	_delay_ms(10);
	set_leds(false,true,false,true);
	_delay_ms(10);
}


uint8_t my_addr[5] = { 0x77, 0x77, 0x77, 0x77, 0x77 };
//uint8_t roomba_addr[5] = { 0x98, 0x76, 0x54, 0x32, 0x10 };
uint8_t roomba_addr[5] = { 0xED, 0xB7, 0xED, 0xB7, 0xED};

#define ROOMBA_START	128		// start the Roomba's serial command interface
#define ROOMBA_BAUD	129		// set the SCI's baudrate (default on full power cycle is 57600
#define ROOMBA_CONTROL	130		// enable control via SCI
#define ROOMBA_SAFE	131		// enter safe mode
#define ROOMBA_FULL	132		// enter full mode
#define ROOMBA_POWER	133		// put the Roomba to sleep
#define ROOMBA_SPOT	134		// start spot cleaning cycle
#define ROOMBA_CLEAN	135		// start normal cleaning cycle
#define ROOMBA_MAX		136		// start maximum time cleaning cycle
#define ROOMBA_DRIVE	137		// control wheels
#define ROOMBA_MOTORS	138		// turn cleaning motors on or off
#define ROOMBA_LEDS	139		// activate LEDs
#define ROOMBA_SONG	140		// load a song into memory
#define ROOMBA_PLAY	141		// play a song that was loaded using SONG
#define ROOMBA_SENSORS	142		// retrieve one of the sensor packets
#define ROOMBA_DOCK	143		// force the Roomba to seek its dock.


int main(int argc, char *argv[])
{
	set_leds(true,false,true,false); // red, red

	// initialization
	allow_interupts(false);
		set_clock_frequency();
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

	//const char* bwoop = "Bwoop!";

	radiopacket_t packet;

	// direct messages to roomba
	Radio_Set_Tx_Addr(roomba_addr);

	uint16_t tick=0;
	bool flashLed = false;
	while (1) {
		//poll_joystick();
		uart_println("tick: %d\r\n",tick);
		tick++;

		// listen
		while (rxflag) {
			memset(&packet, 0, sizeof(packet));
			if (Radio_Receive(&packet) != RADIO_RX_MORE_PACKETS) {
				rxflag = 0;
			}
			
			//if (packet.type == MESSAGE) {
				uart_println("received something.\r\n");
			//}
		}

		// create something interesting to send
		memset(&packet, 0, sizeof(packet));
		memcpy(&packet.payload.command.sender_address, my_addr, 5);

		/*
		// spin me right round baby
		packet.payload.command.command = ROOMBA_DRIVE;
		packet.payload.command.num_arg_bytes = 4;
		packet.payload.command.arguments[0] = 0;
		packet.payload.command.arguments[1] = 200;
		packet.payload.command.arguments[2] = 255;
		packet.payload.command.arguments[3] = 255;
		*/




		
		/*
		69 A 440.0
		70 A# 466.2
		71 B 493.9
		72 C 523.3
		73 C# 554.4
		74 D 587.3
		75 D# 622.3
		76 E 659.3
		77 F 698.5
		78 F# 740.0
		79 G 784.0

		// CC, GG, AA, G, FF, EE, DD, C, GG, FF, EE, DD, GG, FF, EE, D, CC, G, C, AA, G, FF, EE, DD, C
		*/
		const uint8_t twinkle[16] = {	1, // program to slot 1
								7, 		// number of notes

								72, 4,
								72, 4,

								79, 4,
								79, 4,

								69, 4,
								69, 4,

								79, 4 };
								
		packet.payload.command.command = ROOMBA_SONG;
		packet.payload.command.num_arg_bytes = 16;
		memcpy(packet.payload.command.arguments, twinkle, 16);
		Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);

		for (int i=0; i<10; ++i) {
			_delay_ms(10);
		}

		packet.payload.command.command = ROOMBA_PLAY;
		packet.payload.command.num_arg_bytes = 1;
		packet.payload.command.arguments[0] = 1;

		Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);

		flashLed = !flashLed;
		set_leds(flashLed,true,false,true);

		for (int i=0; i<100; ++i) {
			_delay_ms(100);
		}
	}

	return 0;
}
