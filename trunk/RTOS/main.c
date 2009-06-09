#include <LUFA/Drivers/Board/LEDs.h>
#include <avr/io.h>
#include "OS/os.h"
#include "main.h"

const unsigned char PPP[] = {1, 100};
const unsigned int PT = 1;

void some_task (void)
{
	uint8_t LEDMask = LEDS_NO_LEDS;

	while (1) {
		LEDMask ^= LEDS_LED1;
		LEDs_SetAllLEDs(LEDMask);

        Task_Next();
	}
}


int main(void)
{
	LEDs_Init();

	Task_Create(some_task, 1, PERIODIC, 1);

    return 0;
}
