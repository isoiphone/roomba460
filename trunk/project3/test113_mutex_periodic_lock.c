/**
 * @file   test113.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Thu Jul 2 10:09:33 2009
 * 
 * @brief  Test 113 - test a periodic task locking a mutex
 *                    (should produce no output and crash the OS)
 * 
 */

#include "OS/os.h"
#include "uart/uart.h"
#include "trace/trace.h"

enum { A=1 };
const unsigned char PPP[] = {A, 2};
const unsigned int PT = sizeof(PPP)/2;
EVENT* print_event;
MUTEX* mutex;

void waste_ticks(unsigned int t)
{
    unsigned int start_t = Now();
    while (Now() - start_t < t);
}

void periodic_task(void)
{
    Mutex_Lock(mutex, 100);
}

int main(void)
{
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(113);

    mutex = Mutex_Init();

   	Task_Create(periodic_task, 0, PERIODIC, A);
}
