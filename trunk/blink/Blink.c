#include <avr/io.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	DDRB = 0x00;
	PORTB = 0xff;
	DDRE = 0x00;
	PORTE = 0xff;
	DDRD = 0xff;



	PORTD = 0xff;
	

	while (1) {
		PORTD &= (PINB & PINE);

		if (PORTD == 0x0f)
			PORTD = 0xff;
	}

	return 0;
}
