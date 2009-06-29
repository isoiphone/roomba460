/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

#ifndef _CONFIGDESCRIPTOR_H_
#define _CONFIGDESCRIPTOR_H_

	/* Includes: */
		#include <LUFA/Drivers/USB/USB.h>                        // USB Functionality
		
		#include "GamepadHost.h"
		
	/* Macros: */
		/** Interface Class value for the Human Interface Device class */
		#define GAMEPAD_CLASS                 0x03

		/** Interface Protocol value for a Boot Protocol Gamepad compliant device */
		#define GAMEPAD_PROTOCOL              0x00 // 00 for joystick?

		/** Maximum size of a device configuration descriptor which can be processed by the host, in bytes */
		#define MAX_CONFIG_DESCRIPTOR_SIZE  512
	
	/* Enums: */
		/** Enum for the possible return codes of the ProcessConfigurationDescriptor() function. */
		enum GamepadHost_GetConfigDescriptorDataCodes_t
		{
			SuccessfulConfigRead            = 0, /**< Configuration Descriptor was processed successfully */
			ControlError                    = 1, /**< A control request to the device failed to complete successfully */
			DescriptorTooLarge              = 2, /**< The device's Configuration Descriptor is too large to process */
			InvalidConfigDataReturned       = 3, /**< The device returned an invalid Configuration Descriptor */
			NoHIDInterfaceFound             = 4, /**< A compatible HID interface was not found in the device's Configuration Descriptor */
			NoEndpointFound                 = 5, /**< A compatible HID IN endpoint was not found in the device's HID interface */
		};	

	/* Configuration Descriptor Comparison Functions: */
		DESCRIPTOR_COMPARATOR(NextGamepadInterface);
		DESCRIPTOR_COMPARATOR(NextInterfaceGamepadDataEndpoint);

	/* Function Prototypes: */
		uint8_t ProcessConfigurationDescriptor(void);	

#endif
