/**
 * @file   test109.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 109 - test the simplest use of a mutex
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

void system_task_1(void)
{
    add_to_trace(1);
    Mutex_Lock(mutex, 100);
    add_to_trace(2);
    Task_Sleep(5);

    add_to_trace(4);
    Mutex_Unlock(mutex);
    add_to_trace(5);
}

void system_task_2(void)
{
    add_to_trace(3);
    Mutex_Lock(mutex, 100);

    add_to_trace(6);
    Mutex_Unlock(mutex);
    add_to_trace(7);
    Event_Signal(print_event);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(109);

    mutex = Mutex_Init();

   	Task_Create(system_task_1, 0, SYSTEM, 0);
   	Task_Create(system_task_2, 0, SYSTEM, 0);
	
    print_event = Event_Init();    
    Event_Wait(print_event);
    print_trace();
}
