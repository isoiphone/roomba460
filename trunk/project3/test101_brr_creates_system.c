/**
 * @file   test101.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 101 - test that a BRR task will be pre-empted
 *                    when it creates a system task
 * 
 */

#include "OS/os.h"
#include "uart/uart.h"
#include "trace/trace.h"

const unsigned char PPP[] = {};
const unsigned int PT = sizeof(PPP)/2;
EVENT* print_event;

void brr_task(void)
{
    add_to_trace(1);
   	Task_Create(system_task, 0, SYSTEM, 0);
    add_to_trace(3);
    Event_Signal(print_event);
}

void system_task(void)
{
    add_to_trace(2);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(101);

   	Task_Create(brr_task, 0, BRR, 0);
	
    print_event = Event_Init();    
    Event_Wait(print_event);
    print_trace();
}
