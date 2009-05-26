#include "GamepadHost.h"
#include "packet.h"
#include "radio.h"
#include "roomba_sci.h"
#include "sensor_struct.h"
#include "timer.h"


#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(*(x)))
#define TICK_LENGTH_IN_MS (TICK_LENGTH / 1000)

uint8_t my_addr[5] = { 0x77, 0x77, 0x77, 0x77, 0x77 };
uint8_t roomba_addr[5] = { 0xED, 0xB7, 0xED, 0xB7, 0xED};


typedef struct
{
	void (*callback)(void);
    uint16_t period;
    uint16_t time_waited;
	bool is_running;
} task_t;


void allow_interupts(bool allow)
{
	if (allow)
		asm volatile ("sei"::);
	else
		asm volatile ("cli"::);
}


// FIXME
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

void task_radio_receive(void)
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

void task_update_speed_display(void)
{
	int16_t speed = (100 * (uint16_t)prev_distance) / prev_time;
	printf_P(PSTR("dist: %d dt: %2dms speed:%2d cm/s\r\n"), prev_distance, prev_time, speed);
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

USB_GamepadReport_Data_t GamepadReport;

void task_drive(void)
{
	uint8_t arguments[4];

#define	DEADZONE	10

	int16_t velocity = 0; // 0 = don't move!
	if (GamepadReport.y1 < (128-DEADZONE) || GamepadReport.y1 > (128+DEADZONE))
	{
		velocity = (int16_t)((255-GamepadReport.y1)*39)/10 - 500; // 0 -> 994, then -500 -> +494 (excluding values in deadzone)

		// velocity -500 -> +500. Positive is forward. Endianness!
	}

	int16_t radius = htons(32768); // 32768 = straight.
	if (GamepadReport.x2 < (128-DEADZONE) || GamepadReport.x2 > (128+DEADZONE)) {
		radius = (int16_t)(GamepadReport.x2*15) - 2000; // 0 -> 3825, then -2000 -> +1825 (excluding values in deadzone)

		// radius -2000 -> +2000. Positive is counter-clockwise.
		
	}

	printf_P(PSTR("y1: %d x2: %d\r\n"), GamepadReport.y1, GamepadReport.x2);
	printf_P(PSTR("DRIVE: %d %d\r\n"), velocity, radius);

	velocity = htons(velocity);
	memcpy(&arguments[0], &velocity, 2);
	radius = htons(radius);
	memcpy(&arguments[2], &radius, 2);

	send_to_roomba(DRIVE, arguments, sizeof(arguments));
}

void task_gamepad(void);

// FIXME
volatile task_t tasks[] = {
	{ task_radio_receive, 100, 0, true },
	{ task_update_speed_display, 1000, 0, true },
	{ task_drive, 1000, 0, false },
	{ task_gamepad, 100, 0, false },
	{ USB_USBTask, 1, 0, false }
	
};

#define TASK_RADIO_RECEIVE 			0
#define TASK_UPDATE_SPEED_DISPLAY	1
#define TASK_DRIVE					2
#define TASK_GAMEPAD				3
#define TASK_USB					4

void start_task(uint16_t task_number)
{
	tasks[task_number].is_running = true;
}

void stop_task(uint16_t task_number)
{
	tasks[task_number].is_running = false;
}


void initialize_all(void)
{
	// initialization
	allow_interupts(false);
		// Disable watchdog if enabled by bootloader/fuses
		MCUSR &= ~(1 << WDRF);
		wdt_disable();

		// Disable clock division
		clock_prescale_set(clock_div_1);

		// timer!
		Timer_Init();

		// init the LUFA UART
		SerialStream_Init(9600, false);

		// init LUFA LEDs
		LEDs_Init();

		// Initialize USB Subsystem
		USB_Init();

		Radio_Init();
		Radio_Configure_Rx(RADIO_PIPE_0, my_addr, ENABLE);
		Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);

		//init_joystick();
	allow_interupts(true);

	_delay_ms(10);
	//uart_println("%c[2J", 0x1B); // clear terminal screen
	puts_P(PSTR("------\r\n"));

	// direct messages to roomba
	Radio_Set_Tx_Addr(roomba_addr);

	previous_speed_update_ticks = Timer_Now();
}

void run_event_loop(void)
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

	memset(&GamepadReport, 0, sizeof(USB_GamepadReport_Data_t));

	run_event_loop();
	return 0;
}


void UpdateStatus(uint8_t CurrentStatus)
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
	puts_P(PSTR("Device Attached.\r\n"));
	UpdateStatus(Status_USBEnumerating);
	start_task(TASK_USB);
}

EVENT_HANDLER(USB_DeviceUnattached)
{
	stop_task(TASK_USB);
	stop_task(TASK_DRIVE);
	stop_task(TASK_GAMEPAD);

	puts_P(PSTR("Device Unattached.\r\n"));
	UpdateStatus(Status_USBNotReady);
}

EVENT_HANDLER(USB_DeviceEnumerationComplete)
{
	puts_P(PSTR("Enumeration Complete.\r\n"));
	start_task(TASK_GAMEPAD);
	start_task(TASK_DRIVE);
	UpdateStatus(Status_USBReady);
}

EVENT_HANDLER(USB_HostError)
{
	USB_ShutDown();

	puts_P(PSTR(ESC_BG_RED "Host Mode Error\r\n"));
	printf_P(PSTR(" -- Error Code %d\r\n"), ErrorCode);

	// halt forever
	UpdateStatus(Status_HardwareError);
	for(;;);
}

EVENT_HANDLER(USB_DeviceEnumerationFailed)
{
	puts_P(PSTR(ESC_BG_RED "Dev Enum Error\r\n"));
	printf_P(PSTR(" -- Error Code %d\r\n"), ErrorCode);
	printf_P(PSTR(" -- Sub Error Code %d\r\n"), SubErrorCode);
	printf_P(PSTR(" -- In State %d\r\n"), USB_HostState);

	UpdateStatus(Status_EnumerationError);
}


void ReadNextReport(void)
{
	uint8_t                LEDMask = LEDS_NO_LEDS;

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
		Pipe_Read_Stream_LE(&GamepadReport, sizeof(GamepadReport));				

		// Alter status LEDs according to gamepad X movement
		if (GamepadReport.x1 > 0)
		  LEDMask |= LEDS_LED1;
		else if (GamepadReport.x1 < 0)
		  LEDMask |= LEDS_LED2;
			
		// Alter status LEDs according to gamepad Y movement
		if (GamepadReport.y1 > 0)
		  LEDMask |= LEDS_LED3;
		else if (GamepadReport.y1 < 0)
		  LEDMask |= LEDS_LED4;

		// Alter status LEDs according to gamepad button position
//		if (GamepadReport.Button)
//		  LEDMask  = LEDS_ALL_LEDS;
		
		LEDs_SetAllLEDs(LEDMask);
		
	}

	// Clear the IN endpoint, ready for next data packet
	Pipe_ClearIN();

	// Refreeze gamepad data pipe
	Pipe_Freeze();
}


void task_gamepad(void)
{
	uint8_t ErrorCode;

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
				UpdateStatus(Status_EnumerationError);
				
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
				UpdateStatus(Status_EnumerationError);

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
				UpdateStatus(Status_EnumerationError);
				
				// Wait until USB device disconnected
				while (USB_IsConnected);
				break;
			}

			USB_HostState = HOST_STATE_Ready;
			break;
		case HOST_STATE_Ready:
			// If a report has been received, read and process it
			ReadNextReport();

			break;
	}
}
