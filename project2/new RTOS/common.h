/**
 * @file   common.h
 * @author Scott Craig
 * @author Justin Tanner
 *
 *
 * @brief Common macros.
 *
 *  CSC 460/560 Real Time Operating Systems - Mantis Cheng
 *
 */


#ifndef __COMMON_H__
#define __COMMON_H__

/** Disable default prescaler to make processor speed 8 MHz. */
#define     clock8MHz()    CLKPR = (1<<CLKPCE); CLKPR = 0x00;

#define F_CPU 8000000UL  // 8 MHz

#define Disable_Interrupt()    asm volatile ("cli"::)
#define Enable_Interrupt()     asm volatile ("sei"::)

#endif
