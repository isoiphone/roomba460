/**
 * @file    trace.c
 *
 * @brief   Testing module for creating traces.
 *          Used for testing an RTOS
 *          
 *          NOTE: Traces are stored in string like this
 *
 *          T001;1;2;3;1235 
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 * @author Scott Craig
 * @author Justin Tanner
 */

#ifndef __TRACE_H__
#define __TRACE_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>

#include "../uart/uart.h"
#include "../OS/os.h"

#define TRACE_ARRAY_SIZE         256
#define TRACE_BUFFER_SIZE        32

void print_trace(void);
void add_to_trace(uint16_t number);
void set_test(uint8_t number);

#endif
