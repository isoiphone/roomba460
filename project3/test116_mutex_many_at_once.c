/**
 * @file   test116.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 116 - test the usage of multiple mutexes
 * 
 */

#include "OS/os.h"
#include "uart/uart.h"
#include "trace/trace.h"

const unsigned char PPP[] = {};
const unsigned int PT = sizeof(PPP)/2;
EVENT* print_event;
MUTEX* mutex1;
MUTEX* mutex2;

void waste_ticks(unsigned int t)
{
    unsigned int start_t = Now();
    while (Now() - start_t < t);
}

void system_task_1(void)
{
    add_to_trace(1);
    Mutex_Lock(mutex1, 100);
    add_to_trace(2);
    Task_Sleep(5);

    add_to_trace(6);
    Mutex_Unlock(mutex1);
    add_to_trace(7);
}

void system_task_2(void)
{
    add_to_trace(3);
    Mutex_Lock(mutex2, 100);
    add_to_trace(4);
    Task_Sleep(10);

    add_to_trace(10);
    Mutex_Unlock(mutex2);
    add_to_trace(11);
}

void system_task_3(void)
{
    add_to_trace(5);
    Mutex_Lock(mutex1, 100);

    add_to_trace(8);
    Mutex_Unlock(mutex1);
    add_to_trace(9);
    Mutex_Lock(mutex2, 100);

    add_to_trace(12);
    Mutex_Unlock(mutex2);
    add_to_trace(13);
    Event_Signal(print_event);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(116);
	print_event = Event_Init();

    mutex1 = Mutex_Init();
    mutex2 = Mutex_Init();

   	Task_Create(system_task_1, 0, SYSTEM, 0);
   	Task_Create(system_task_2, 0, SYSTEM, 0);
   	Task_Create(system_task_3, 0, SYSTEM, 0);
	
    Event_Wait(print_event);
    print_trace();
}
