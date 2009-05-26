#include "GamepadHost.h"

TASK_LIST
{
	{ .Task = USB_USBTask          , .TaskStatus = TASK_STOP },
	{ .Task = USB_Gamepad_Host       , .TaskStatus = TASK_STOP },
};


int main(void)
{
	// Disable watchdog if enabled by bootloader/fuses
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Disable clock division
	clock_prescale_set(clock_div_1);
	
	// Hardware Initialization
	LEDs_Init();
	
	// Indicate USB not ready 
	UpdateStatus(Status_USBNotReady);
	
	// Initialize Scheduler so that it can be used
	Scheduler_Init();

	// Initialize USB Subsystem
	USB_Init();

	// Scheduling - routine never returns, so put this last in the main function
	Scheduler_Start();
}

EVENT_HANDLER(USB_DeviceAttached)
{
	UpdateStatus(Status_USBEnumerating);
	Scheduler_SetTaskMode(USB_USBTask, TASK_RUN);
}

EVENT_HANDLER(USB_DeviceUnattached)
{
	Scheduler_SetTaskMode(USB_USBTask, TASK_STOP);
	Scheduler_SetTaskMode(USB_Gamepad_Host, TASK_STOP);
	UpdateStatus(Status_USBNotReady);
}

EVENT_HANDLER(USB_DeviceEnumerationComplete)
{
	Scheduler_SetTaskMode(USB_Gamepad_Host, TASK_RUN);
	UpdateStatus(Status_USBReady);
}

EVENT_HANDLER(USB_HostError)
{
	USB_ShutDown();

	// halt forever
	UpdateStatus(Status_HardwareError);
	for(;;);
}

EVENT_HANDLER(USB_DeviceEnumerationFailed)
{
	UpdateStatus(Status_EnumerationError);
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

void ReadNextReport(void)
{
	USB_GamepadReport_Data_t GamepadReport;
	uint8_t                LEDMask = LEDS_NO_LEDS;

	// Select gamepad data pipe
	Pipe_SelectPipe(GAMEPAD_DATAPIPE);	

	#if !defined(INTERRUPT_DATA_PIPE)
	// Unfreeze keyboard data pipe
	Pipe_Unfreeze();
	#endif

	// Check to see if a packet has been received
	if (!(Pipe_IsINReceived()))
	{
		#if !defined(INTERRUPT_DATA_PIPE)
		// Refreeze HID data IN pipe
		Pipe_Freeze();
		#endif
			
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

	/* Clear the IN endpoint, ready for next data packet */
	Pipe_ClearIN();

	#if !defined(INTERRUPT_DATA_PIPE)
	// Refreeze gamepad data pipe
	Pipe_Freeze();
	#endif
}


TASK(USB_Gamepad_Host)
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

			#if defined(INTERRUPT_DATA_PIPE)			
			// Select and unfreeze gamepad data pipe
			Pipe_SelectPipe(GAMEPAD_DATAPIPE);	
			Pipe_Unfreeze();
			#endif

			USB_HostState = HOST_STATE_Ready;
			break;
		#if !defined(INTERRUPT_DATA_PIPE)
		case HOST_STATE_Ready:
			// If a report has been received, read and process it
			ReadNextReport();

			break;
		#endif
	}
}

#if defined(INTERRUPT_DATA_PIPE)
/** Interrupt handler for the Endpoint/Pipe interrupt vector. This interrupt fires each time an enabled
 *  pipe interrupt occurs on a pipe which has had that interrupt enabled.
 */
ISR(ENDPOINT_PIPE_vect, ISR_BLOCK)
{
	// Save previously selected pipe before selecting a new pipe
	uint8_t PrevSelectedPipe = Pipe_GetCurrentPipe();

	// Check to see if the gamepad data pipe has caused the interrupt
	if (Pipe_HasPipeInterrupted(GAMEPAD_DATAPIPE))
	{
		// Clear the pipe interrupt, and select the gamepad pipe
		Pipe_ClearPipeInterrupt(GAMEPAD_DATAPIPE);
		Pipe_SelectPipe(GAMEPAD_DATAPIPE);	

		// Check to see if the pipe IN interrupt has fired
		if (USB_INT_HasOccurred(PIPE_INT_IN) && USB_INT_IsEnabled(PIPE_INT_IN))
		{
			// Clear interrupt flag
			USB_INT_Clear(PIPE_INT_IN);		

			// Read and process the next report from the device
			ReadNextReport();
		}
	}

	// Restore previously selected pipe
	Pipe_SelectPipe(PrevSelectedPipe);
}
#endif
