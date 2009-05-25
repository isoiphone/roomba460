/*
 * timer.c
 *
 *  Created on: 27-Feb-2009
 *      Author: Neil MacMillan, Oliver Pirquet
 */

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"


volatile uint16_t count = 0;

// get the value of count atomically; this will ensure that an interrupt doesn't execute in the middle of
// loading the integer's low and high bytes.
uint16_t sample_count()
{
	uint16_t cnt;
	uint8_t sreg = SREG;	// back up the status register
	cli();					// clear the interrupt bit to disable global interrupts
	cnt = count;			// copy count
	SREG = sreg;			// restore the status register
	return cnt;
}

void Timer_Init()
{
	// Note that the RTOS uses timer 1 for its scheduler, so if this is used with the RTOS
	// it should be changed to use timer 3 (but probably using this with the RTOS is dumb anyway
	// since the RTOS provides equivalent functionality).

	// set timer 1 to run at 1 MHz (prescaler is f(osc)/8)
	TCCR1B |= _BV(CS11);
	TCCR1B &= ~(_BV(CS10) | _BV(CS12));
	// Configure TOP to be whatever's in OCR1A
	//TCCR1B |= _BV(WGM12);

	// enable the interrupt and clear the flag
	TIMSK1 |= _BV(OCIE1A);
	TIFR1 |= _BV(OCF1A);

	// OC1.A reaches the TOP every TICK_LENGTH timer cycles (every TICK_LENGTH/1000 milliseconds)
	OCR1A = TCNT1 + TICK_LENGTH;
}

void Timer_Delay(uint16_t milliseconds)
{
	// this isn't totally accurate.  I'm not sure why.  It's close enough though.
	uint16_t start = sample_count();

	// wait until the timer has counted the required number of milliseconds.
	while (sample_count() - start <= milliseconds);	// count is updated by the interrupt
}

void Timer_Delay_1ms()
{
	// poll the counter until it has counted to 1000 (1 ms)
	// this isn't particularly useful, it's just an example for polling the counter.
	uint16_t start = TCNT1;
	while (TCNT1 - start <= 1000);
}

uint16_t Timer_Now()
{
	return count;
}

ISR(TIMER1_COMPA_vect)
{
	OCR1A = OCR1A + TICK_LENGTH;
	count++;
}
