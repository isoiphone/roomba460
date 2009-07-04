/**
 * @file   test117.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 117 - test the absence of barging
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

void Mutex_Unlock_Wrapper(MUTEX* m)
{
    /** create the task which attempts to barge */
   	Task_Create(system_task_3, 0, SYSTEM, 0);
    Mutex_Unlock(m);
    Task_Next();
}

void system_task_1(void)
{
    add_to_trace(1);
    Mutex_Lock(mutex, 100);
    add_to_trace(2);
    Task_Next();

    add_to_trace(4);
    Mutex_Unlock_Wrapper(mutex);

    add_to_trace(8);
}

void system_task_2(void)
{
    add_to_trace(3);
    Mutex_Lock(mutex, 100);

    add_to_trace(6);
    Mutex_Unlock(mutex);
    add_to_trace(7);
}

void system_task_3(void)
{
    add_to_trace(5);

    /** attempts to barge, but blocks on the mutex */
    Mutex_Lock(mutex, 100);

    add_to_trace(9);
    Mutex_Unlock(mutex);
    add_to_trace(10);
    Event_Signal(print_event);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(117);
	print_event = Event_Init();

    mutex = Mutex_Init();

   	Task_Create(system_task_1, 0, SYSTEM, 0);
   	Task_Create(system_task_2, 0, SYSTEM, 0);
	
    Event_Wait(print_event);
    print_trace();
}
