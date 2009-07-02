/**
 * @file   test114.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 114 - test mutex lock exceeding its time limit
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
    Mutex_Lock(mutex, 1);
    Task_Sleep(5);
}

void system_task_2(void)
{
    Mutex_Lock(mutex, 100);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(114);

    mutex = Mutex_Init();

   	Task_Create(system_task_1, 0, SYSTEM, 0);
   	Task_Create(system_task_2, 0, SYSTEM, 0);
}
