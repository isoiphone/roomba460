/**
 * @file   test003.c
 * @author Scott Craig and Justin Tanner
 * @date   Mon Oct 29 16:19:32 2007
 *
 * @brief  Test 003 - can schedule RR tasks in the expected order
 *
 */

#include "OS/os.h"
#include "uart/uart.h"
#include "trace/trace.h"

enum { A=1, B, C, D, E, F, G };
const unsigned int PT = 2;
const unsigned char PPP[] = {IDLE, 10, A, 50};

extern uint16_t trace_counter;
EVENT* print_event;

void generic_task(void)
{
    int arg = 0;
    arg = Task_GetArg();

    for(;;)
    {
        add_to_trace(arg);

        if(trace_counter >= TRACE_ARRAY_SIZE)
        {
            Event_Signal(print_event);
        }

        Task_Next();
    }
}

int main(void)
{
    /* setup the test */
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(3);

    print_event = Event_Init();

    Task_Create(generic_task, 1, BRR, 1);
    Task_Create(generic_task, 2, BRR, 1);
    Task_Create(generic_task, 3, BRR, 1);
    Task_Create(generic_task, 4, BRR, 1);
    Task_Create(generic_task, 5, BRR, 1);
    Task_Create(generic_task, 6, BRR, 1);

    Event_Wait(print_event);
    print_trace();
}
