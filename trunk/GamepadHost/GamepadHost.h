/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

#ifndef _GAMEPAD_HOST_H_
#define _GAMEPAD_HOST_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/pgmspace.h>
		#include <avr/interrupt.h>
		#include <avr/power.h>
		#include <stdio.h>

		#include <LUFA/Version.h>                                // Library Version Information
		#include <LUFA/Drivers/Misc/TerminalCodes.h>             // ANSI Terminal Escape Codes
		#include <LUFA/Drivers/USB/USB.h>                        // USB Functionality
		#include <LUFA/Drivers/Peripheral/SerialStream.h>        // Serial stream driver
		#include <LUFA/Drivers/Board/LEDs.h>                     // LEDs driver
		#include <LUFA/Scheduler/Scheduler.h>                    // Simple scheduler for task management
		
		#include "ConfigDescriptor.h"
		
	/* Macros: */
		/** Pipe number for the gamepad data IN pipe */
		#define GAMEPAD_DATAPIPE              1
		
		/** HID Class Specific request to set the report protocol mode */
		#define REQ_SetProtocol             0x0B

	/* Type Defines: */
		/** Type define for a standard Boot Protocol Gamepad report */
		typedef struct
		{
		// format based on notes from: http://atariwiki.strotmann.de/xwiki/bin/export/MicroUSB/How+to+write+a+USB+Driver?format=pdf
			int8_t x1, y1;
			int8_t x2, y2;

			unsigned dpad : 4;
			unsigned button : 10;
			unsigned : 2;

			unsigned : 2;
			uint8_t vib : 1;
			uint8_t mode : 1;
			unsigned : 4;

			uint8_t pad;
		} USB_GamepadReport_Data_t;

	/* Task Definitions: */
		TASK(USB_Gamepad_Host);

	/* Enums: */
		/** Enum for the possible status codes for passing to the UpdateStatus() function. */
		enum GamepadHost_StatusCodes_t
		{
			Status_USBNotReady      = 0, /**< USB is not ready (disconnected from a USB device) */
			Status_USBEnumerating   = 1, /**< USB interface is enumerating */
			Status_USBReady         = 2, /**< USB interface is connected and ready */
			Status_EnumerationError = 3, /**< Software error while enumerating the attached USB device */
			Status_HardwareError    = 4, /**< Hardware error while enumerating the attached USB device */
		};
		
	/* Event Handlers: */
		HANDLES_EVENT(USB_DeviceAttached);
		HANDLES_EVENT(USB_DeviceUnattached);
		HANDLES_EVENT(USB_DeviceEnumerationComplete);
		HANDLES_EVENT(USB_HostError);
		HANDLES_EVENT(USB_DeviceEnumerationFailed);

	/* Function Prototypes: */
		void UpdateStatus(uint8_t CurrentStatus);
		void ReadNextReport(void);
		
#endif
