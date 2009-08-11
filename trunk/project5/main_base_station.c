/**
 * @file   base_station.c
 * @author Jason Wynja and Jacob Schwartz
 * @date   Mon Aug 10 14:26:30 2009
 *
 * @brief  The main base station file
 *
 */

#include "OS/os.h"
#include "radio.h"
#include "uart.h"

// RTOS - Periodic project plan
const unsigned int PT = 0;
const unsigned char PPP[] = {};

// network addresses
uint8_t my_addr[5] = { 0x77, 0x77, 0x77, 0x77, 0x77 };
uint8_t roomba_addr[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };

// Radio receive flag
volatile uint8_t rxflag = 0;

// Radio receive handler
void radio_rxhandler(uint8_t pipenumber)
{
	rxflag = 1;
}

void initialize_systems(void)
{
	uint8_t sreg = SREG;
	Disable_Interrupt();

    uart_init();

	Radio_Init();
	Radio_Configure_Rx(RADIO_PIPE_0, my_addr, ENABLE);
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);

	// direct messages to roomba
	Radio_Set_Tx_Addr(roomba_addr);

	// Restore interrupt status
	SREG = sreg;

	_delay_ms(10);
}

int main(void)
{
	initialize_systems();
}

