/**
 * @file   main_base_station.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Mon Aug 10 14:26:30 2009
 *
 * @brief  The main base station file
 *
 */

#include "OS/common.h"
#include "OS/os.h"
#include "radio.h"
#include "roomba_sci.h"
#include "uart.h"

#define DRIVE_SPEED 150
#define NUMBER_OF_TURTLES 2
#define PI 3.141592654


typedef struct {
	int16_t command;
	int16_t arg1;
	int16_t arg2;
	int16_t arg3;
} Command;


typedef struct {
    // radio address for this roomba
    uint8_t address[RADIO_ADDRESS_LENGTH];
        
    // current roomba command arguments
    uint8_t roomba_led_red;
    uint8_t roomba_led_green;
    uint8_t roomba_led_blue;
	int16_t roomba_velocity;
	int16_t roomba_radius;

    // sensor value accumulators
    int16_t angle;
    int16_t distance;

    // value we are trying to reach in this state (could use a distance or a time instead, for now we use angle)
    float angle_target;

    // strictly speaking, we dont need to keep track of state, it can be derived from
    // the current command being executed. However it is kept as a convenience
    int16_t state;

    Command* plan;
	int16_t plan_length;
    int16_t plan_index;
} Turtle;


enum { PARKED=1, ARCING, SPINNING };
enum { HALT=1, MOVE_ARC, SPIN, SET_LED };

Turtle turtles[NUMBER_OF_TURTLES];

Command plan0[] = { { SET_LED, 0x52, 0x18, 0xFA }, { MOVE_ARC, 400, 120, 0 },
					{ SET_LED, 0x48, 0x29, 0x6F }, { MOVE_ARC, 400, 120, 0 },
					{ SET_LED, 0x52, 0x18, 0xFA }, { MOVE_ARC, 400, 120, 0 },
					{ SPIN, 60, 0, 0 },
					{ SET_LED, 0x52, 0x18, 0xFA }, { MOVE_ARC, 400, 120, 0 },
					{ SET_LED, 0x48, 0x29, 0x6F }, { MOVE_ARC, 400, 120, 0 },
					{ SET_LED, 0x52, 0x18, 0xFA }, { MOVE_ARC, 400, 120, 0 },
					{ SPIN, 60, 0, 0 },
					{ SET_LED, 0x52, 0x18, 0xFA }, { MOVE_ARC, 400, 120, 0 },
					{ SET_LED, 0x48, 0x29, 0x6F }, { MOVE_ARC, 400, 120, 0 },
					{ SET_LED, 0x52, 0x18, 0xFA }, { MOVE_ARC, 400, 120, 0 },
					{ SET_LED, 0x00, 0x00, 0x00 }, { MOVE_ARC, 200, 60, 0 },
					{ SET_LED, 0xEC, 0x58, 0x00 }, { MOVE_ARC, 200, 360, 0 } };

Command plan1[] = {};

uint8_t my_addr[RADIO_ADDRESS_LENGTH] = { 0x77, 0x77, 0x77, 0x77, 0x77 };

// RTOS - Periodic project plan
enum { EXECUTE_0=1, EXECUTE_1, SENDER_0, SENDER_1 };

const unsigned char PPP[] = { EXECUTE_0, 4, EXECUTE_1, 4, SENDER_0, 21, SENDER_1, 21 };
//const unsigned char PPP[] = { EXECUTE_0, 8, SENDER_0, 42 };
const unsigned int PT = sizeof(PPP) / 2;


// Swap byte order
int16_t htons(int16_t value)
{
	int16_u output,input;
	input.value = value;
	output.bytes.low_byte = input.bytes.high_byte;
	output.bytes.high_byte = input.bytes.low_byte;
	return output.value;
}


void drive(Turtle* turtle, int16_t velocity, int16_t radius)
{
	turtle->roomba_velocity = velocity;
	turtle->roomba_radius = radius;
}


void set_led(Turtle* turtle, uint8_t led_red, uint8_t led_green, uint8_t led_blue)
{
	turtle->roomba_led_red = led_red;
	turtle->roomba_led_green = led_green;
	turtle->roomba_led_blue = led_blue;
}


// Radio receiver
EVENT* radio_receive_event;
volatile uint8_t rxflag = 0;

void radio_rxhandler(uint8_t pipenumber)
{
	rxflag = 1;
	Event_Signal(radio_receive_event);
}

void task_radio_receive(void);
void handle_received_packet(radiopacket_t* packet);
int radio_addresses_are_equal(uint8_t* address1, uint8_t* address2);
void update_sensor_data(Turtle* turtle, roomba_sensor_data_t sensor_data);

void task_radio_receive(void)
{
	for (;;)
	{
		Event_Wait(radio_receive_event);

		while (rxflag)
		{
			radiopacket_t packet;
			memset(&packet, 0, sizeof(packet));

			if (Radio_Receive(&packet) != RADIO_RX_MORE_PACKETS)
			{
				rxflag = 0;
			}

			handle_received_packet(&packet);
		}
	}
}

void handle_received_packet(radiopacket_t* packet)
{
	if (packet->type == SENSOR_DATA)
	{
        uint8_t* sender_address = packet->payload.command.sender_address;

        int i;
        for (i = 0; i < NUMBER_OF_TURTLES; ++i)
        {
            if (radio_addresses_are_equal(sender_address, turtles[i].address))
            {
                update_sensor_data(turtles + i, packet->payload.sensors.sensors);
                break;
            }
        }
	}
}

int radio_addresses_are_equal(uint8_t* address1, uint8_t* address2)
{
    int i;
    for (i = 0; i < RADIO_ADDRESS_LENGTH; ++i)
    {
        if (address1[i] != address2[i])
        {
            return 0;
        }
    }

    return 1;
}

void update_sensor_data(Turtle* turtle, roomba_sensor_data_t sensor_data)
{
    turtle->angle += sensor_data.angle.value;
    turtle->distance += sensor_data.distance.value;
}



// Radio sender
void send_to_roomba(uint8_t* address, uint8_t command, uint8_t* arguments, uint8_t num_arg_bytes)
{
	radiopacket_t packet;
	memset(&packet, 0, sizeof(packet));

	packet.type = COMMAND;
	memcpy(&packet.payload.command.sender_address, my_addr, RADIO_ADDRESS_LENGTH);
	packet.payload.command.command = command;
	packet.payload.command.num_arg_bytes = num_arg_bytes;

	if (num_arg_bytes)
		memcpy(packet.payload.command.arguments, arguments, num_arg_bytes);

	Radio_Set_Tx_Addr(address);
	Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);
}


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
		i2c_arguments[0] = turtles[index].roomba_led_red;
		i2c_arguments[1] = turtles[index].roomba_led_green;
		i2c_arguments[2] = turtles[index].roomba_led_blue;
		send_to_roomba(turtles[index].address, I2C, i2c_arguments, sizeof(i2c_arguments));

		Task_Next();
	}
}


float float_abs(float value)
{
	if (value < 0.0)
	{
		value = value * -1.0;
	}

	return value;
}


void halt_turtle(Turtle* turtle)
{
	turtle->state = PARKED;
	set_led(turtle, 0, 0, 0);
	drive(turtle, 0, 0);
}


void issueNextCommand(Turtle* turtle)
{
	turtle->plan_index++;

	if (turtle->plan_index >= turtle->plan_length)
	{
		halt_turtle(turtle);
		return;
	}

	turtle->angle = 0;
	turtle->distance = 0;
	turtle->angle_target = 0.0;

	Command* command = turtle->plan + turtle->plan_index;

	switch (command->command)
	{
		case HALT:
			halt_turtle(turtle);
			break;

		case MOVE_ARC:
			turtle->state = ARCING;
			turtle->angle_target = command->arg2;
			drive(turtle, DRIVE_SPEED, command->arg1);
			break;

		case SPIN:
			turtle->state = SPINNING;
			turtle->angle_target = command->arg1;

            if (command->arg1 <= 0)
			{
                drive(turtle, DRIVE_SPEED, 1);
			}
            else
			{
                drive(turtle, DRIVE_SPEED, -1);
			}

			break;

		case SET_LED:
			set_led(turtle, command->arg1, command->arg2, command->arg3);
			issueNextCommand(turtle);
			break;
	}
}


void task_execute_plan(void)
{
	for (;;)
	{
    	int index = Task_GetArg();
		Turtle* turtle = turtles + index;

		// in degrees, with clockwise meaning being negative
		float turned_through = (360.0 * turtle->angle) / (258.0 * PI);

		if (turtle->state == PARKED && turtle->plan_index < turtle->plan_length)
		{
			issueNextCommand(turtle);
		}
		else if (turtle->state == ARCING || turtle->state == SPINNING)
		{
			if (float_abs(turned_through) + 12.0 >= float_abs(turtle->angle_target))
			{
				issueNextCommand(turtle);
			}
		}

		Task_Next();
	}
}


void initialize_systems(void)
{
	// Disable interrupts
	uint8_t sreg = SREG;
	Disable_Interrupt();

	// Initialize peripherals
    //uart_init(UART_DEFAULT);

	Radio_Init();
	Radio_Configure_Rx(RADIO_PIPE_0, my_addr, ENABLE);
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);

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
    turtles[0].plan = plan0;
    turtles[0].plan_length = sizeof(plan0) / sizeof(Command);
	turtles[0].plan_index = -1;
	turtles[0].state = PARKED;

	turtles[1].address[0] = 0xEE;
	turtles[1].address[1] = 0xDD;
	turtles[1].address[2] = 0xCC;
	turtles[1].address[3] = 0xBB;
	turtles[1].address[4] = 0xAA;
    turtles[1].plan = plan1;
    turtles[1].plan_length = sizeof(plan1) / sizeof(Command);
	turtles[1].plan_index = -1;
	turtles[1].state = PARKED;
}

void create_tasks(void)
{
	Task_Create(task_radio_receive, 0, SYSTEM, 0);
	Task_Create(task_execute_plan, 0, PERIODIC, EXECUTE_0);
	Task_Create(task_execute_plan, 1, PERIODIC, EXECUTE_1);
	Task_Create(task_radio_send, 0, PERIODIC, SENDER_0);
	Task_Create(task_radio_send, 1, PERIODIC, SENDER_1);
}

int main(void)
{
	initialize_turtles();
	radio_receive_event = Event_Init();
	initialize_systems();

	_delay_ms(10);

	create_tasks();
}


