/**
 * @file   test106.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 106 - test that if no BRR tasks can fit into the
 *                    idle time in the PPP, the idle task is run
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

void periodic_task(void)
{
    add_to_trace(1);
}

void brr_task_1(void)
{
    add_to_trace(2);
}

void brr_task_2(void)
{
    add_to_trace(3);
    Event_Signal(print_event);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(106);

   	Task_Create(periodic_task, 0, PERIODIC, A);
   	Task_Create(brr_task_1, 0, BRR, 4);
   	Task_Create(brr_task_2, 0, BRR, 4);
	
    print_event = Event_Init();    
    Event_Wait(print_event);
    print_trace();
}
