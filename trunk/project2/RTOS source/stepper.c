/** @author Scott Craig
 *
 *  @brief Stepper motor sample application.
 */
#include <avr/io.h>
#include "OS/os.h"
#include "stepper.h"

const unsigned char PPP[] = {1, 2};
const unsigned int PT = 1;

const uint8_t step_bits[] =
{
    STEPPER_RED |   STEPPER_WHITE,
                    STEPPER_WHITE,
    STEPPER_BLUE |  STEPPER_WHITE,
    STEPPER_BLUE,
    STEPPER_BLUE |  STEPPER_YELLOW,
                    STEPPER_YELLOW,
    STEPPER_RED |   STEPPER_YELLOW,
    STEPPER_RED
};

void step (void)
{
    static uint8_t step_index = 0;

    STEPPER_PORT_DDR |= STEPPER_MASK;

	for(;;)
	{
        STEPPER_PORT = (STEPPER_PORT & (uint8_t)(~STEPPER_MASK)) | step_bits[step_index];    
		
        Task_Next();

        step_index += STEP;
        if(step_index >= 8) step_index = 0;
	}
}


int main(void)
{
	Task_Create(step, 1, PERIODIC, 1);

    return 0;
}
