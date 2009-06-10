#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/Board/Joystick.h>
#include <avr/io.h>
#include "GamepadHost.h"
#include "OS/os.h"
#include "packet.h"
#include "radio.h"
#include "roomba_sci.h"
#include "sensor_struct.h"
#include "joy2mov.h"


// network addresses
uint8_t my_addr[5] = { 0x77, 0x77, 0x77, 0x77, 0x77 };
uint8_t roomba_addr[5] = { 0xED, 0xB7, 0xED, 0xB7, 0xED};


// timing and process communication flags
volatile uint8_t rxflag = 0;
volatile int16_t prev_distance = 0;
volatile uint16_t prev_time = 0;
volatile uint16_t previous_speed_update_ticks = 0;
volatile uint8_t LEDs = LEDS_NO_LEDS;

// gamepad state
USB_GamepadReport_Data_t gamepad_status;

enum {
	Gamepad_Disconnected=0,
	Gamepad_Enumerating,
	Gamepad_Connected,
	Gamepad_Error
};

volatile uint8_t Gamepad_State = Gamepad_Disconnected;


// the RTOS
enum {
	GPAD=1,	// gamepad task polls gamepad for a new state structure
	DRIV,		// drive creates and transmits the driving control packet
	RECV,	// reads status packets
	DISP		// calcultes and displays some speed measurements
};

// From Assignment:
// 1. Run all your tasks as PERIODIC tasks.
// 2. Run all your tasks as a mixture of SYSTEM and ROUND ROBIN tasks.
// 3. Finally, use EVENTS to coordinate your tasks in (2).
//#define TESTCASE_1	1
//#define TESTCASE_2	2
//#define TESTCASE_3	3

//#if defined(TESTCASE_3)
//EVENT* gamepad_event;
//#endif

/*
const unsigned char PPP[] = {
    GPAD, 4, IDLE, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, RECV, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, RECV, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, DRIV, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, RECV, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, RECV, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, DRIV, 4, RECV, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, RECV, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, RECV, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, DRIV, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, RECV, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, RECV, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, IDLE, 4, IDLE, 4, IDLE, 4, IDLE, 4, 
    GPAD, 4, DRIV, 4, RECV, 4, DISP, 4, IDLE, 2
};
const unsigned int PT = 100;
*/

const unsigned char PPP[] = {
    GPAD, 4,
	DRIV, 4,
	RECV, 4,
	DISP, 4,
};
const unsigned int PT = 4;

void allow_interupts(bool allow)
{
    // FIXME: should really save state, then restore it.
	if (allow)
		asm volatile ("sei"::);
	else
		asm volatile ("cli"::);
}


int16_t htons(int16_t value)
{
	int16_u output,input;
	input.value = value;
	output.bytes.low_byte = input.bytes.high_byte;
	output.bytes.high_byte = input.bytes.low_byte;
	return output.value;
}


void radio_rxhandler(uint8_t pipenumber)
{
	rxflag = 1;
}


void handle_received_radio_packet(radiopacket_t* packet)
{
	if (packet->type == SENSOR_DATA)
	{
		roomba_sensor_data_t sensor_data = packet->payload.sensors.sensors;
		
		uint16_t current_ticks = Now();
		uint16_t delta_ticks = current_ticks - previous_speed_update_ticks;

		prev_distance = sensor_data.distance.value;
		prev_time = delta_ticks * TICK;

		previous_speed_update_ticks = current_ticks;
	}
}


void task_radio_receive(void)
{
	while (1)
	{
		if (rxflag)
		{
			radiopacket_t packet;
			memset(&packet, 0, sizeof(packet));
		
		// read as many packets as we received, we are only interested in the most recent one
			while (Radio_Receive(&packet) == RADIO_RX_MORE_PACKETS) {}
			
			rxflag = 0;

			handle_received_radio_packet(&packet);
		}
		
		LEDs ^= LEDS_LED3;
		LEDs_SetAllLEDs(LEDs);
		Task_Next();
	}
}


void task_update_speed_display(void)
{
	while (1) 
	{
		int16_t speed = (100 * (uint16_t)prev_distance) / prev_time;
		//printf_P(PSTR("dist: %d dt: %2dms speed:%2d cm/s\r\n"), prev_distance, prev_time, speed);
		puts_P(PSTR("disp.\r\n"));

		LEDs ^= LEDS_LED4;
		LEDs_SetAllLEDs(LEDs);
		Task_Next();
	}
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


void task_drive(void)
{
	while (1)
	{

		uint8_t arguments[4];

		//#if defined(TESTCASE_3)
		//Event_Wait(gamepad_event);
		//#endif
		
		// map gamepad left and right joysticks positions to roomba values
		int16_t velocity, radius;
		joystick_to_movement(gamepad_status.x2, gamepad_status.y1, &velocity, &radius);

		// debug printing
		//printf_P(PSTR("DRIVE: (v=%3d,r=%3d)\r\n"), velocity, radius);

		// roomba uses different endianness
		velocity = htons(velocity);
		memcpy(&arguments[0], &velocity, 2);
		radius = htons(radius);
		memcpy(&arguments[2], &radius, 2);

		// deliver driving packet
		send_to_roomba(DRIVE, arguments, sizeof(arguments));

		LEDs ^= LEDS_LED2;
		LEDs_SetAllLEDs(LEDs);
		Task_Next();

	}
}



void initialize_all(void)
{
	// initialization
	allow_interupts(false);
		// RTOS code does not now.
		// Disable watchdog if enabled by bootloader/fuses
		MCUSR &= ~(1 << WDRF);
		wdt_disable();

		// Disable clock division
		clock_prescale_set(clock_div_1);

		// init the LUFA UART
		SerialStream_Init(9600, false);

		// init the LUFA LEDS
		LEDs_Init();
		LEDs_SetAllLEDs(LEDS_LED1|LEDS_LED2|LEDS_LED3|LEDS_LED4);
	
		// init the LUFA USB
		USB_Init();

		// init the radio
		Radio_Init();
		Radio_Configure_Rx(RADIO_PIPE_0, my_addr, ENABLE);
		Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);

		// direct messages to roomba
		Radio_Set_Tx_Addr(roomba_addr);

		previous_speed_update_ticks = Now();

		_delay_ms(10);
		puts_P(PSTR("[--- started ---]\r\n"));
	allow_interupts(true);
}


// ensure roomba is in command mode, with full control, and stopped.
void stop_roomba()
{
	uint8_t args[4] = {0,0,0,0};
	send_to_roomba(START, 0, 0);
	_delay_ms(10);
	send_to_roomba(CONTROL, 0, 0);
	_delay_ms(10);
	send_to_roomba(FULL, 0, 0);
	_delay_ms(10);
	send_to_roomba(DRIVE, args, 4);
	_delay_ms(10);
}

// Entry point (called by the RTOS)
int main(void)
{
	initialize_all();

	stop_roomba();
	puts_P(PSTR("waiting for gamepad.\r\n"));

	// wait for gamepad to be plugged in and configured
	while (Gamepad_State != Gamepad_Connected) {
		USB_USBTask();
	}


	
	LEDs_SetAllLEDs(LEDS_NO_LEDS);

	//#if defined(TESTCASE_1)
	Task_Create(task_gamepad, GPAD, PERIODIC, GPAD);
	Task_Create(task_drive, DRIV, PERIODIC, DRIV);
	Task_Create(task_radio_receive, RECV, PERIODIC, RECV);
	Task_Create(task_update_speed_display, DISP, PERIODIC, DISP);

	/*
	#elif defined(TESTCASE_2)
	Task_Create(task_gamepad, GPAD, RR, GPAD);
	Task_Create(task_drive, DRIV, SYSTEM, DRIV);
	Task_Create(task_radio_receive, RECV, SYSTEM, RECV);
	Task_Create(task_update_speed_display, DISP, RR, DISP);
	#elif defined(TESTCASE_3)
	gamepad_event = Event_Init(); 
	Task_Create(task_gamepad, GPAD, PERIODIC, GPAD);
	Task_Create(task_drive, DRIV, RR, DRIV);
	Task_Create(task_radio_receive, RECV, RR, RECV);
	Task_Create(task_update_speed_display, DISP, RR, DISP);
	#endif
	*/



	return 0;
}



void update_status(uint8_t CurrentStatus)
{
	uint8_t LEDMask = LEDS_NO_LEDS;
	
	switch (CurrentStatus)
	{
		case Status_USBNotReady:
			LEDMask = (LEDS_LED1);
			break;
		case Status_USBEnumerating:
			LEDMask = (LEDS_LED1 | LEDS_LED2);
			break;
		case Status_USBReady:
			LEDMask = (LEDS_LED2);
			break;
		case Status_EnumerationError:
		case Status_HardwareError:
			LEDMask = (LEDS_LED1 | LEDS_LED3);
			break;
	}
	
	LEDs_SetAllLEDs(LEDMask);
}


EVENT_HANDLER(USB_DeviceAttached)
{
	puts_P(PSTR("Gamepad attached...\r\n"));
	update_status(Status_USBEnumerating);

	Gamepad_State = Gamepad_Enumerating;
}

EVENT_HANDLER(USB_DeviceUnattached)
{
	puts_P(PSTR("Gamepad detached.\r\n"));
	update_status(Status_USBNotReady);

	Gamepad_State = Gamepad_Disconnected;
}

EVENT_HANDLER(USB_DeviceEnumerationComplete)
{
    puts_P(PSTR("Gamepad OK.\r\n"));
	update_status(Status_USBReady);

	Gamepad_State = Gamepad_Connected;
}

EVENT_HANDLER(USB_HostError)
{
	USB_ShutDown();

	puts_P(PSTR(ESC_BG_RED "Host Mode Error\r\n"));
	//printf_P(PSTR(" -- Error Code %d\r\n"), ErrorCode);

	Gamepad_State = Gamepad_Error;
	// halt forever
	update_status(Status_HardwareError);

	for(;;);
}

EVENT_HANDLER(USB_DeviceEnumerationFailed)
{
	puts_P(PSTR(ESC_BG_RED "Dev Enum Error\r\n"));
	//printf_P(PSTR(" -- Error Code %d\r\n"), ErrorCode);
	//printf_P(PSTR(" -- Sub Error Code %d\r\n"), SubErrorCode);
	//printf_P(PSTR(" -- In State %d\r\n"), USB_HostState);

	update_status(Status_EnumerationError);
	Gamepad_State = Gamepad_Error;
}


void read_gamepad_report(void)
{
	// Select gamepad data pipe
	Pipe_SelectPipe(GAMEPAD_DATAPIPE);	

	// Unfreeze keyboard data pipe
	Pipe_Unfreeze();

	// Check to see if a packet has been received
	if (!(Pipe_IsINReceived()))
	{
		// Refreeze HID data IN pipe
		Pipe_Freeze();
		return;
	}

	// Ensure pipe contains data before trying to read from it
	if (Pipe_IsReadWriteAllowed())
	{
		// Read in gamepad report data
		Pipe_Read_Stream_LE(&gamepad_status, sizeof(gamepad_status));				
	}

	// Clear the IN endpoint, ready for next data packet
	Pipe_ClearIN();

	// Refreeze gamepad data pipe
	Pipe_Freeze();
}


void task_gamepad(void)
{
	uint8_t ErrorCode;

	while (1) 
	{
		// jms: ensure USB is pumped periodically so it does not time out.
		USB_USBTask();
	
		// Switch to determine what user-application handled host state the host state machine is in
		switch (USB_HostState)
		{
			case HOST_STATE_Addressed:
				// Standard request to set the device configuration to configuration 1
				USB_ControlRequest = (USB_Request_Header_t)
					{
						.bmRequestType = (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE),
						.bRequest      = REQ_SetConfiguration,
						.wValue        = 1,
						.wIndex        = 0,
						.wLength       = 0,
					};

				// Select the control pipe for the request transfer
				Pipe_SelectPipe(PIPE_CONTROLPIPE);

				// Send the request, display error and wait for device detach if request fails
				if ((ErrorCode = USB_Host_SendControlRequest(NULL)) != HOST_SENDCONTROL_Successful)
				{
					// Indicate error status
					update_status(Status_EnumerationError);
				
					// Wait until USB device disconnected
					while (USB_IsConnected);
					break;
				}
			
				USB_HostState = HOST_STATE_Configured;
				break;
			case HOST_STATE_Configured:
				// Get and process the configuration descriptor data
				if ((ErrorCode = ProcessConfigurationDescriptor()) != SuccessfulConfigRead)
				{
					// Indicate error status
					update_status(Status_EnumerationError);

					// Wait until USB device disconnected
					while (USB_IsConnected);
					break;
				}
		
				// HID class request to set the gamepad protocol to the Boot Protocol
				USB_ControlRequest = (USB_Request_Header_t)
					{
						.bmRequestType = (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE),
						.bRequest      = REQ_SetProtocol,
						.wValue        = 0,
						.wIndex        = 0,
						.wLength       = 0,
					};

				// Select the control pipe for the request transfer
				Pipe_SelectPipe(PIPE_CONTROLPIPE);

				// Send the request, display error and wait for device detach if request fails
				if ((ErrorCode = USB_Host_SendControlRequest(NULL)) != HOST_SENDCONTROL_Successful)
				{
					// Indicate error status
					update_status(Status_EnumerationError);
				
					// Wait until USB device disconnected
					while (USB_IsConnected);
					break;
				}

				USB_HostState = HOST_STATE_Ready;
				break;
			case HOST_STATE_Ready:
				// If a report has been received, read and process it
				read_gamepad_report();

				break;
		}
		LEDs ^= LEDS_LED1;
		LEDs_SetAllLEDs(LEDs);

		//#if defined(TESTCASE_3)
		//	Signal_And_Next(gamepad_event);
		//#else

			Task_Next();
		//#endif
	}
}

