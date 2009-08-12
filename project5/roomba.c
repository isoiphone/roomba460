/*
 * roomba.c
 *
 *  Created on: 4-Feb-2009
 *      Author: nrqm
 */

#include <util/delay.h>
#include "uart.h"
#include "roomba.h"
#include "roomba_sci.h"

void Roomba_Init()
{
	// At 8 MHz, the AT90 generates a 57600 bps signal with a framing error rate of over 2%, which means that more than
	// 1 out of every 50 bits is wrong.  The fastest bitrate with a low error rate that the Roomba supports is
	// 38400 bps (0.2% error rate, or 1 bit out of every 500).

	// Try 57.6 kbps to start (this is the Roomba's default baud rate after the battery is installed).
	uart_init(UART_57600);

	// Try to start the SCI
	uart_putchar(START);
	_delay_ms(20);

	// change the baud rate to 38400 bps.  Have to wait for 100 ms after changing the baud rate.
	uart_putchar(BAUD);
	uart_putchar(ROOMBA_38400BPS);
	_delay_ms(100);		// this delay will not work on old versions of WinAVR (new versions will see reduced but
						// still acceptable resolution; see _delay_ms definition)

	// change the AT90's UART clock
	uart_init(UART_38400);

	// start the SCI again in case the first start didn't go through.
	uart_putchar(START);
	_delay_ms(20);

	// finally put the Roomba into safe mode.
	uart_putchar(CONTROL);
	_delay_ms(20);
}

void Roomba_UpdateSensorPacket(ROOMBA_SENSOR_GROUP group, roomba_sensor_data_t* sensor_packet)
{
	uart_putchar(SENSORS);
	uart_putchar(group);
	switch(group)
	{
	case EXTERNAL:
		// environment sensors
		while (uart_bytes_received() != 10);
		sensor_packet->bumps_wheeldrops = uart_get_byte(0);
		sensor_packet->wall = uart_get_byte(1);
		sensor_packet->cliff_left = uart_get_byte(2);
		sensor_packet->cliff_front_left = uart_get_byte(3);
		sensor_packet->cliff_front_right = uart_get_byte(4);
		sensor_packet->cliff_right = uart_get_byte(5);
		sensor_packet->virtual_wall = uart_get_byte(6);
		sensor_packet->motor_overcurrents = uart_get_byte(7);
		sensor_packet->dirt_left = uart_get_byte(8);
		sensor_packet->dirt_right = uart_get_byte(9);
		break;
	case CHASSIS:
		// chassis sensors
		while (uart_bytes_received() != 6);
		sensor_packet->remote_opcode = uart_get_byte(0);
		sensor_packet->buttons = uart_get_byte(1);
		sensor_packet->distance.bytes.high_byte = uart_get_byte(2);
		sensor_packet->distance.bytes.low_byte = uart_get_byte(3);
		sensor_packet->angle.bytes.high_byte = uart_get_byte(4);
		sensor_packet->angle.bytes.low_byte = uart_get_byte(5);
		break;
	case INTERNAL:
		// internal sensors
		while (uart_bytes_received() != 10);
		sensor_packet->charging_state = uart_get_byte(0);
		sensor_packet->voltage.bytes.high_byte = uart_get_byte(1);
		sensor_packet->voltage.bytes.low_byte = uart_get_byte(2);
		sensor_packet->current.bytes.high_byte = uart_get_byte(3);
		sensor_packet->current.bytes.low_byte = uart_get_byte(4);
		sensor_packet->temperature = uart_get_byte(5);
		sensor_packet->charge.bytes.high_byte = uart_get_byte(6);
		sensor_packet->charge.bytes.low_byte = uart_get_byte(7);
		sensor_packet->capacity.bytes.high_byte = uart_get_byte(8);
		sensor_packet->capacity.bytes.low_byte = uart_get_byte(9);
		break;
	}
	uart_reset_receive();
}

void Roomba_Drive( int16_t velocity, int16_t radius )
{
	uart_putchar(DRIVE);
	uart_putchar(HIGH_BYTE(velocity));
	uart_putchar(LOW_BYTE(velocity));
	uart_putchar(HIGH_BYTE(radius));
	uart_putchar(LOW_BYTE(radius));
}
