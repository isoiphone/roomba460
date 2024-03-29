/**
 * @file   test107.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 107 - test that a pre-empted BRR task runs
 *                    with the same quantum the next time
 *                    it is scheduled (rather than the
 *                    "leftovers" from its last run)
 * 
 */

#include "OS/os.h"
#include "uart/uart.h"
#include "trace/trace.h"

enum { A=1 };
const unsigned char PPP[] = {IDLE, 3, A, 1};
const unsigned int PT = sizeof(PPP)/2;
EVENT* print_event;

void waste_ticks(unsigned int t)
{
    unsigned int start_t = Now();
    while (Now() - start_t < t);
}

void system_task(void)
{
    add_to_trace(1);
    Task_Sleep(2);
    add_to_trace(4);
}

void periodic_task(void)
{
    add_to_trace(6);
}

void brr_task_1(void)
{
    add_to_trace(2);
    waste_ticks(1);
    add_to_trace(3);
    waste_ticks(1);

    // should be pre-empted here by system_task

    add_to_trace(7);
    waste_ticks(1);
    add_to_trace(8);
    waste_ticks(1);
    add_to_trace(9);
    waste_ticks(1);

    // should be pre-empted here, used up its quantum

    add_to_trace(11);
    Event_Signal(print_event);
}

void brr_task_2(void)
{
    add_to_trace(5);
    waste_ticks(1);

    // should be pre-empted here by periodic_task

    add_to_trace(10);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(107);

   	Task_Create(system_task, 0, SYSTEM, 0);
   	Task_Create(periodic_task, 0, PERIODIC, A);
   	Task_Create(brr_task_1, 0, BRR, 3);
   	Task_Create(brr_task_2, 0, BRR, 1);
	
    print_event = Event_Init();    
    Event_Wait(print_event);
    print_trace();
}
