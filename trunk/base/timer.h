/*
 * timer.h
 *
 *  Created on: 27-Feb-2009
 *      Author: Neil MacMillan, Oliver Pirquet
 */

#ifndef TIMERDELAY_H_
#define TIMERDELAY_H_


#define TICK_LENGTH 5000	// The number of counter increments between interrupts
							// (e.g. 1000 ==> interrupt fired every 1 ms, 5000 ==> every 5 ms)

/**
 * Initialize the system timer.
 */
void Timer_Init();

/**
 * Block for the given number of milliseconds.  This may or may not be accurate (WTF?).
 */
void Timer_Delay(uint16_t milliseconds);

/**
 * Block for one millisecond.
 */
void Timer_Delay_1ms();

/**
 * Get the number of 1 ms ticks that have elapsed since the timer was initialised.  This value rolls over every
 * 65,536 ms.  If interrupts are disabled for more than 2 ms, then the tick counter will not be incremented and
 * the timer results will be wrong.
 */
uint16_t Timer_Now();

#endif /* TIMERDELAY_H_ */
