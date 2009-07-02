/**
 * @file   test103.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 103 - test that a BRR task with quantum 0
 *                    is put back on the front of the queue
 *                    when it is pre-empted
 * 
 */

#include "OS/os.h"
#include "uart/uart.h"
#include "trace/trace.h"

const unsigned char PPP[] = {};
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
    Task_Sleep(5);
    add_to_trace(3);
}

void fcfs_task_1(void)
{
    add_to_trace(2);
    waste_ticks(10);
    add_to_trace(4);
}

void fcfs_task_2(void)
{
    add_to_trace(5);
    Event_Signal(print_event);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(103);

   	Task_Create(system_task, 0, SYSTEM, 0);
   	Task_Create(fcfs_task_1, 0, BRR, 0);
   	Task_Create(fcfs_task_2, 0, BRR, 0);
	
    print_event = Event_Init();    
    Event_Wait(print_event);
    print_trace();
}
