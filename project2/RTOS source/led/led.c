/**
 * @file   led.c
 * @author Scott Craig
 * @author Alexander M. Hoole
 * @date   Mon Sep 24 17:32:03 2007
 * 
 * @brief  Constants and functions for controlling the state of the onboard
 *         LEDs on the AT90USBKey.
 * 
 */

#include <avr/io.h>
#include "LED.h"

/** 
 * Initialize the LED port D2 for output. 
 */
void init_LED_D2(void)
{
	PORTD &= ~LED_D2;	//Initialize port to LOW (turn off LED)
	DDRD |= LED_D2;		//Set PORT to output 
}


/** 
 * Initialize the LED port D5 for output. 
 */
void init_LED_D5(void)
{
	PORTD &= (uint8_t)(~LED_D5);	//Initialize port to LOW (turn off LED)
	DDRD |= (uint8_t)(LED_D5);		//Set PORT to output
}


/** 
 * Initialize both LEDs ports for output. 
 */
void init_LEDs(void)
{
	PORTD &= (uint8_t)~(LED_D2 | LED_D5);
	DDRD |= (uint8_t)(LED_D2 | LED_D5);
}

	
/** 
 * Enable the specified LEDs using the supplied mask. 
 * 
 * @param mask Using the defined LED values from LED.h turn on specific LEDs.
 */
void on_LED(unsigned int mask)
{
	PORTD |= (uint8_t)(mask & (LED_D2 | LED_D5));
}


/** 
 * Disable the specified LEDs using the supplied mask. 
 * 
 * @param mask Using the defined LED values from LED.h turn off specific LEDs.
 */
void off_LED(unsigned int mask)
{
	PORTD &= (uint8_t)(~mask | ~(LED_D2 | LED_D5));
}


/** 
 * Turn off all LEDs. 
 */
void off_all_LEDs(void)
{
    PORTD &= (uint8_t)(~(LED_D2 | LED_D5));
}
