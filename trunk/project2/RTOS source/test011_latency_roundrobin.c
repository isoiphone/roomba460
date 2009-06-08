/**
 * @file   test011.c
 * @author Scott Craig and Justin Tanner
 * @date   Mon Oct 29 16:19:32 2007
 * 
 * @brief  Test 011 - test the latency of round robin tasks
 * 
 */

#include "common.h"
#include "OS/os.h"
#include "uart/uart.h"
#include "trace/trace.h"

enum { A=1, B, C, D, E, F, G };
const unsigned int PT = 0;
const unsigned char PPP[] = {};

uint16_t time = 0;
uint16_t last = 0;
EVENT* print_event;

void round_robin(void)
{
    for(;;)
    {
        time = TCNT3;
        
		add_to_trace(time - last);

        last = time;

        Task_Next();
    }
}

int main(void)
{
    /* setup the test */
	uart_init();
	uart_write((uint8_t*)"\r\nSTART\r\n", 9);
    set_test(11);

    print_event = Event_Init();

    /* Run clock at 8MHz. */
    TCCR3B = _BV(CS30);
      
    Task_Create(round_robin, 1, RR, 0);
	Task_Create(round_robin, 2, RR, 0);
	
    Event_Wait(print_event);
    print_trace();
}
