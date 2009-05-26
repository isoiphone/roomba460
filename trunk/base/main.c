
#include "common.h"
#include "packet.h"
#include "radio.h"
#include "roomba_sci.h"
#include "sensor_struct.h"
#include "timer.h"
#include "uart.h"


#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(*(x)))
#define TICK_LENGTH_IN_MS (TICK_LENGTH / 1000)

uint8_t my_addr[5] = { 0x77, 0x77, 0x77, 0x77, 0x77 };
uint8_t roomba_addr[5] = { 0xED, 0xB7, 0xED, 0xB7, 0xED};


typedef struct
{
	void (*callback)();
    uint16_t period;
    uint16_t time_waited;
	bool is_running;
} task_t;


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


int16_t htons(int16_t value)
{
	int16_u input;
	int16_u output;

	input.value = value;

	output.bytes.low_byte = input.bytes.high_byte;
	output.bytes.high_byte = input.bytes.low_byte;

	return output.value;
}


volatile uint8_t rxflag = 0;

void radio_rxhandler(uint8_t pipenumber)
{
	rxflag = 1;
}


volatile int16_t prev_distance = 0;
volatile uint16_t prev_time = 0;
volatile uint16_t previous_speed_update_ticks = 0;

void handle_received_radio_packet(radiopacket_t* packet)
{
	if (packet->type == SENSOR_DATA)
	{
		roomba_sensor_data_t sensor_data = packet->payload.sensors.sensors;
		
		uint16_t current_ticks = Timer_Now();
		uint16_t delta_ticks = current_ticks - previous_speed_update_ticks;

		prev_distance = sensor_data.distance.value;
		prev_time = delta_ticks * TICK_LENGTH_IN_MS;

		previous_speed_update_ticks = current_ticks;
	}
}

void task_radio_receive()
{
	while (rxflag)
	{
		radiopacket_t packet;
		memset(&packet, 0, sizeof(packet));

		while (Radio_Receive(&packet) == RADIO_RX_MORE_PACKETS) {}
		
		rxflag = 0;

		handle_received_radio_packet(&packet);
	}
}

void task_update_speed_display()
{
	int16_t speed = (100 * (uint16_t)prev_distance) / prev_time;
	uart_println("dist: %d dt: %2dms speed:%2d cm/s\r\n", prev_distance, prev_time, speed);
}


void send_to_roomba(uint8_t command, uint8_t* arguments, uint8_t num_arg_bytes)
{
	radiopacket_t packet;
	memset(&packet, 0, sizeof(packet));

	packet.type = COMMAND;
	memcpy(&packet.payload.command.sender_address, my_addr, 5);
	packet.payload.command.command = command;
	packet.payload.command.num_arg_bytes = num_arg_bytes;

	if (num_arg_bytes)
		memcpy(packet.payload.command.arguments, arguments, num_arg_bytes);

	Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);
}


void task_drive()
{
	uint8_t arguments[4];
	int16_t velocity = htons(100);
	int16_t radius = htons(1);

	memcpy(&arguments[0], &velocity, 2);
	memcpy(&arguments[2], &radius, 2);

	send_to_roomba(DRIVE, arguments, sizeof(arguments));
}


volatile task_t tasks[] = {
	{ task_radio_receive, 100, 0, true },
	{ task_update_speed_display, 1000, 0, true },
	{ task_drive, 1000, 0, true }
};

#define TASK_RADIO_RECEIVE 0
#define TASK_UPDATE_SPEED_DISPLAY 1
#define TASK_DRIVE 2

void start_task(uint16_t task_number)
{
	tasks[task_number].is_running = true;
}

void stop_task(uint16_t task_number)
{
	tasks[task_number].is_running = false;
}


void initialize_all()
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
	//uart_println("%c[2J", 0x1B); // clear terminal screen
	uart_println("------\r\n"); // clear terminal screen

	// green, green
	set_leds(false,true,false,true);

	// direct messages to roomba
	Radio_Set_Tx_Addr(roomba_addr);

	previous_speed_update_ticks = Timer_Now();
}

void run_event_loop()
{
	uint16_t previous_ticks = Timer_Now();

	while (1)
	{
		uint16_t current_ticks = Timer_Now();
		uint16_t delta_ticks = current_ticks - previous_ticks;
		uint16_t delta_time = delta_ticks * TICK_LENGTH_IN_MS;

		previous_ticks = current_ticks;

		int i;

		for (i = 0; i < ARRAY_LENGTH(tasks); ++i)
		{
			if (tasks[i].is_running)
			{
				tasks[i].time_waited += delta_time;

				if (tasks[i].time_waited >= tasks[i].period)
				{
					tasks[i].time_waited = 0;
					tasks[i].callback();
				}
			}
		}

		_delay_ms(1);
	}
}


int main(int argc, char *argv[])
{
	initialize_all();

	send_to_roomba(START, 0, 0);
	_delay_ms(20);
	send_to_roomba(CONTROL, 0, 0);
	_delay_ms(20);
	send_to_roomba(FULL, 0, 0);
	_delay_ms(20);

	run_event_loop();
	return 0;
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

