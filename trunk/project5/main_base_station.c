/**
 * @file   base_station.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Mon Aug 10 14:26:30 2009
 *
 * @brief  The main base station file
 *
 */

#include "OS/common.h"
#include "OS/os.h"
#include "radio.h"
#include "uart.h"

#define NUMBER_OF_TURTLES 2



// Network address
uint8_t my_addr[5] = { 0x77, 0x77, 0x77, 0x77, 0x77 };

// RTOS - Periodic project plan
enum { SENDER_0=1, SENDER_1 };

const unsigned char PPP[] = { IDLE, 5, SENDER_0, 5, SENDER_1, 5 };
const unsigned int PT = sizeof(PPP) / 2;



typedef struct {
	int16_t command;
	int16_t arg1;
	int16_t arg2;
} Command;



typedef struct {
    // radio address for this roomba
    uint8_t address[5];
        
    // sensor value accumulators
    int16_t angle;
    int16_t distance;

    // value we are trying to reach in this state (could use a distance or a time instead, for now we use angle)
    int16_t angle_target;

    // current roomba command arguments
    int16_t roomba_led;
	int16_t roomba_velocity;
	int16_t roomba_radius;

    // strictly speaking, we dont need to keep track of state, it can be derived from
    // the current command being executed. However it is kept as a convenience
    int16_t state;

    //int16_t plan = ( Command(LED,1), Command(ARC,1000,90), Command(ARC,100,90), Command(SPIN,-90), Command(ARC,100,90), Command(ARC,1000,180) )
    Command* plan;
	int16_t plan_length;
    int16_t plan_index;
} Turtle;

Turtle turtles[NUMBER_OF_TURTLES];



// Swap byte order
int16_t htons(int16_t value)
{
	int16_u output,input;
	input.value = value;
	output.bytes.low_byte = input.bytes.high_byte;
	output.bytes.high_byte = input.bytes.low_byte;
	return output.value;
}



// Radio receiver
EVENT* radio_receive_event;

void radio_rxhandler(uint8_t pipenumber)
{
	Event_Signal(radio_receive_event);
}

void handle_received_packet(radiopacket_t* packet)
{
	if (packet->type == SENSOR_DATA)
	{
		/*roomba_sensor_data_t sensor_data = packet->payload.sensors.sensors;
		
		uint16_t current_ticks = Now();
		uint16_t delta_ticks = current_ticks - previous_speed_update_ticks;

		prev_distance = sensor_data.distance.value;
		prev_time = delta_ticks * TICK;

		previous_speed_update_ticks = current_ticks;*/
	}
}

void task_radio_receive(void)
{
	for (;;)
	{
		Event_Wait(radio_receive_event);

		RADIO_RX_STATUS rx_status = RADIO_RX_MORE_PACKETS;

		while (rx_status == RADIO_RX_MORE_PACKETS)
		{
			radiopacket_t packet;
			memset(&packet, 0, sizeof(packet));

			rx_status = Radio_Receive(&packet);
			handle_received_packet(&packet);
		}
	}
}



void send_to_roomba(uint8_t* address, uint8_t command, uint8_t* arguments, uint8_t num_arg_bytes)
{
	radiopacket_t packet;
	memset(&packet, 0, sizeof(packet));

	packet.type = COMMAND;
	memcpy(&packet.payload.command.sender_address, my_addr, 5);
	packet.payload.command.command = command;
	packet.payload.command.num_arg_bytes = num_arg_bytes;

	if (num_arg_bytes)
		memcpy(packet.payload.command.arguments, arguments, num_arg_bytes);

	Radio_Set_Tx_Addr(address);
	Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);
}



// Radio sender
void task_radio_send(void)
{
	for (;;)
	{
    	int index = Task_GetArg();

		int16_t roomba_velocity = htons(turtles[index].roomba_velocity);
		int16_t roomba_radius = htons(turtles[index].roomba_radius);

		uint8_t drive_arguments[4];
		memcpy(&drive_arguments[0], &roomba_velocity, 2);
		memcpy(&drive_arguments[2], &roomba_radius, 2);
		send_to_roomba(turtles[index].address, DRIVE, drive_arguments, sizeof(drive_arguments));

		uint8_t i2c_arguments[3];

		if (turtles[index].roomba_led == 0)
		{
			i2c_arguments[0] = 0;
			i2c_arguments[1] = 0;
			i2c_arguments[2] = 0;
		}
		else
		{
			i2c_arguments[0] = 0xFF;
			i2c_arguments[1] = 0xFF;
			i2c_arguments[2] = 0xFF;
		}

		send_to_roomba(turtles[index].address, I2C, i2c_arguments, sizeof(i2c_arguments));

		Task_Next();
	}
}



void initialize_systems(void)
{
	// Disable interrupts
	uint8_t sreg = SREG;
	Disable_Interrupt();

	// Initialize peripherals
    uart_init(UART_DEFAULT);

	Radio_Init();
	Radio_Configure_Rx(RADIO_PIPE_0, my_addr, ENABLE);
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);

	// Initialize variables
	radio_receive_event = Event_Init();

	// Restore interrupt status
	SREG = sreg;
}

void initialize_turtles(void)
{
	turtles[0].address[0] = 0xAA;
	turtles[0].address[1] = 0xBB;
	turtles[0].address[2] = 0xCC;
	turtles[0].address[3] = 0xDD;
	turtles[0].address[4] = 0xEE;

	turtles[1].address[0] = 0xEE;
	turtles[1].address[1] = 0xDD;
	turtles[1].address[2] = 0xCC;
	turtles[1].address[3] = 0xBB;
	turtles[1].address[4] = 0xAA;
}

void create_tasks(void)
{
	Task_Create(task_radio_receive, 0, SYSTEM, 0);
	Task_Create(task_radio_send, 0, PERIODIC, SENDER_0);
	Task_Create(task_radio_send, 1, PERIODIC, SENDER_1);
}

int main(void)
{
	initialize_systems();
	initialize_turtles();

	_delay_ms(10);

	create_tasks();
}


