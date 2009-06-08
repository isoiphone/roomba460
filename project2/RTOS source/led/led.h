/**
 * @file   led.h
 * @author Scott Craig
 * @author Alexander M. Hoole
 * @date   Mon Sep 24 17:32:03 2007
 * 
 * @brief  Constants and functions for controlling the state of the onboard
 *         LEDs on the AT90USBKey.
 * 
 */

/** The flags to turn on the Green LED on LED D2. */
#define LED_D2_GREEN	_BV(PORTD5)
/** The flags to turn on the Red LED on LED D2. */
#define LED_D2_RED	_BV(PORTD4)
/** The flags to turn on the Red LED on LED D5. */
#define LED_D5_GREEN	_BV(PORTD6)
/** The flags to turn on the Green LED on LED D5. */
#define LED_D5_RED	_BV(PORTD7)
/** The flags to turn on the both LEDs on LED D2. */
#define LED_D2		(LED_D2_GREEN | LED_D2_RED)
/** The flags to turn on the both LEDs on LED D5. */
#define LED_D5		(LED_D5_GREEN | LED_D5_RED)

/** Enable the specified LEDs using the supplied mask. */
void enable_LED(unsigned int mask);
/** Initialize the LED port D2 for output. */
void init_LED_D2(void);
/** Initialize the LED port D5 for output. */
void init_LED_D5(void);
/** Initialize both LEDs ports for output. */
void init_LEDs(void);
/** Turn on some LEDs */
void on_LED(unsigned int mask);
/** Turn off some LEDs*/
void off_LED(unsigned int mask);
/** Turn off all LEDs. */
void off_all_LEDs(void);
