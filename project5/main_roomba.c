/*
 * main_roomba.c
 *
 *  Created on: 4-Feb-2009
 *      Author: nrqm
 */


#include "roomba.h"
#include "roomba_sci.h"
#include "uart.h"
#include <util/delay.h>
#include "avr/interrupt.h"
#include "radio.h"

#include "I2C/i2c.h"

#define     clock8MHz()    CLKPR = _BV(CLKPCE); CLKPR = 0x00;


uint8_t roomba_addr[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
//uint8_t roomba_addr[5] = { 0xEE, 0xDD, 0xCC, 0xBB, 0xAA };
volatile uint8_t rxflag = 0;
radiopacket_t packet;


int main()
{
	uint16_t i;
	clock8MHz();

	cli();

	DDRC = 0x00;
	PORTC = 0xFF;
	DDRD = 0xFF;
	PORTD = 0x00;

	Roomba_Init();

	// init EYE TWO SEE ELL EE DEE
	i2cInit();
	_delay_ms(10);

	// stop any light script
	unsigned char cmdStop = 'o';
	i2cMasterSend(0x00, 1, &cmdStop);

	// set fade duration to 90 ticks, ~3 seconds
	unsigned char cmdFadeSpeed[] = {'f', 90};
	i2cMasterSend(0x00, 2, cmdFadeSpeed);

	Radio_Init();
	Radio_Configure_Rx(RADIO_PIPE_0, roomba_addr, ENABLE);
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);

	sei();

	for (;;)
	{
		if (rxflag)
		{
			_delay_ms(20);
			// Copy the received packet into the radio packet structure.  If there are no more packets,
			// then clear the rxflag so that the interrupt will set it next time a packet is received.
			if (Radio_Receive(&packet) != RADIO_RX_MORE_PACKETS)
			{
				rxflag = 0;
			}

			// If the packet is not a command, blink an error and don't do anything.
			if (packet.type != COMMAND)
			{
				PORTD ^= _BV(PD7);
				continue;
			}

			if (packet.payload.command.command == START ||
				packet.payload.command.command == BAUD ||
				packet.payload.command.command == CONTROL ||
				packet.payload.command.command == SAFE ||
				packet.payload.command.command == FULL ||
				packet.payload.command.command == SENSORS)
			{
				// Don't pass the listed commands to the Roomba.
				continue;
			}

			if (packet.payload.command.command == I2C)
			{
				uint8_t red = packet.payload.command.arguments[0];
				uint8_t green = packet.payload.command.arguments[1];
				uint8_t blue = packet.payload.command.arguments[2];

				// fade to new color
				unsigned char cmd[] = {'c', red, green, blue};
				i2cMasterSend(0x00, 4, cmd);
			}
			else
			{
				// Output the command to the Roomba, followed by its arguments.
				uart_putchar(packet.payload.command.command);
				for (i = 0; i < packet.payload.command.num_arg_bytes; i++)
				{
					uart_putchar(packet.payload.command.arguments[i]);
				}

				// Set the radio's destination address to be the remote station's address
				Radio_Set_Tx_Addr(packet.payload.command.sender_address);

				// Update the Roomba sensors into the packet structure that will be transmitted.
				Roomba_UpdateSensorPacket(1, &packet.payload.sensors.sensors);
				Roomba_UpdateSensorPacket(2, &packet.payload.sensors.sensors);
				Roomba_UpdateSensorPacket(3, &packet.payload.sensors.sensors);

				// send the sensor packet back to the remote station.
				packet.type = SENSOR_DATA;
				memcpy(&packet.payload.command.sender_address, roomba_addr, RADIO_ADDRESS_LENGTH);

				if (Radio_Transmit(&packet, RADIO_WAIT_FOR_TX) == RADIO_TX_MAX_RT)
				{
					PORTD ^= _BV(PD4);	// flash red if the packet was dropped
				}
				else
				{
					PORTD ^= _BV(PD5);	// flash green if the packet was received correctly
				}
			}
		}
	}

	return 0;
}

void radio_rxhandler(uint8_t pipenumber)
{
	rxflag = 1;
	PORTD ^= _BV(PD7);
}
