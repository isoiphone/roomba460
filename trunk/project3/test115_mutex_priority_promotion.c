/**
 * @file   test115.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 115 - test that a BRR task is promoted to SYSTEM
 *                    after locking a mutex
 * 
 */

#include "OS/os.h"
#include "uart/uart.h"
#include "trace/trace.h"

const unsigned char PPP[] = {};
const unsigned int PT = sizeof(PPP)/2;
EVENT* print_event;
MUTEX* mutex;

void waste_ticks(unsigned int t)
{
    unsigned int start_t = Now();
    while (Now() - start_t < t);
}

void system_task(void)
{
    add_to_trace(4);
}

void brr_task(void)
{
    add_to_trace(1);
    Mutex_Lock(mutex, 100);
    add_to_trace(2);
   	Task_Create(system_task, 0, SYSTEM, 0);
    add_to_trace(3);
    Mutex_Unlock(mutex);

    add_to_trace(5);
    Event_Signal(print_event);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(115);

    mutex = Mutex_Init();
	print_event = Event_Init();

   	Task_Create(brr_task, 0, BRR, 0);
    
    Event_Wait(print_event);
    print_trace();
}
