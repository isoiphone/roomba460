/*
 * radio.c
 *
 *  Created on: 24-Jan-2009
 *      Author: Neil MacMillan
 */
#include "radio.h"

// non-public constants and macros

#define CHANNEL 112
#define ADDRESS_LENGTH 5

// Pin definitions for chip select and chip enable on the radio module
#define RADIO_DDR	DDRB
#define RADIO_PORT	PORTB
#define RADIO_CE	PORTB4
#define RADIO_CSN	PORTB5

// Definitions for selecting and enabling the radio
#define RADIO_CSN_HIGH()	RADIO_PORT |=  (1<<RADIO_CSN);
#define RADIO_CSN_LOW()		RADIO_PORT &= ~(1<<RADIO_CSN);
#define RADIO_CE_HIGH()		RADIO_PORT |=  (1<<RADIO_CE);
#define RADIO_CE_LOW()      RADIO_PORT &= ~(1<<RADIO_CE);

// Flag which denotes that the radio is currently transmitting
volatile uint8_t transmit_lock;
// tracks the payload widths of the Rx pipes
volatile uint8_t rx_pipe_widths[6] = {32, 32, 0, 0, 0, 0};
// holds the transmit address (Rx pipe 0 is set to this address when transmitting with auto-ack enabled).
volatile uint8_t tx_address[5] = { 0xe7, 0xe7, 0xe7, 0xe7, 0xe7 };
// holds the receiver address for Rx pipe 0 (the address is overwritten when transmitting with auto-ack enabled).
volatile uint8_t rx_pipe0_address[5] = { 0xe7, 0xe7, 0xe7, 0xe7, 0xe7 };
// the driver keeps track of the success status for the last 16 transmissions
volatile uint16_t tx_history = 0xFF;

volatile RADIO_TX_STATUS tx_last_status = RADIO_TX_SUCCESS;

extern void radio_rxhandler(uint8_t pipenumber);

/**
 * Retrieve the status register.
 */
uint8_t _get_status(void)
{
	uint8_t status = 0;
	RADIO_CSN_LOW();

	status = SPI_Write_Byte(NOP);

	RADIO_CSN_HIGH();

	return status;
}
/**
 * Set a register in the radio
 * \param reg The register value defined in nRF24L01.h (e.g. CONFIG, EN_AA, &c.).
 * \param value The value to write to the given register (the whole register is overwritten).
 * \return The status register.
 */
uint8_t _set_register(radio_register_t reg, uint8_t* value, uint8_t len)
{
	uint8_t status;
	RADIO_CSN_LOW();

	status = SPI_Write_Byte(W_REGISTER | (REGISTER_MASK & reg));
	SPI_Write_Block(value, len);

	RADIO_CSN_HIGH();

	return status;
}

/**
 * Retrieve a register value from the radio.
 * \param reg The register value defined in nRF24L01.h (e.g. CONFIG, EN_AA, &c.).
 * \param buffer A contiguous memory block into which the register contents will be copied.  If the buffer is too long for the
 * 		register contents, then the remaining bytes will be overwritten with 0xFF.
 * \param len The length of the buffer.
 */
uint8_t _get_register(radio_register_t reg, uint8_t* buffer, uint8_t len)
{
	uint8_t status, i;
	for (i = 0; i < len; i++)
	{
		// If the buffer is too long for the register results, then the radio will interpret the extra bytes as instructions.
		// To remove the risk, we set the buffer elements to NOP instructions.
		buffer[i] = 0xFF;
	}
	RADIO_CSN_LOW();

	status = SPI_Write_Byte(R_REGISTER | (REGISTER_MASK & reg));
	SPI_ReadWrite_Block(NULL, buffer, len);

	RADIO_CSN_HIGH();

	return status;
}

/**
 * Send an instruction to the nRF24L01.
 * \param instruction The instruction to send (see the bottom of nRF24L01.h)
 * \param data An array of argument data to the instruction.  If len is 0, then this may be NULL.
 * \param buffer An array for the instruction's return data.  This can be NULL if the instruction has no output.
 * \param len The length of the data and buffer arrays.
 */
void send_instruction(uint8_t instruction, uint8_t* data, uint8_t* buffer, uint8_t len)
{
    RADIO_CSN_LOW();
	// send the instruction
	SPI_Write_Byte(instruction);
	// pass in args
	if (len > 0)
	{
		if (buffer == NULL)	//
			SPI_Write_Block(data, len);
		else
			SPI_ReadWrite_Block(data, buffer, len);
	}
    // resynch SPI
    RADIO_CSN_HIGH();
}

/**
 * Switch the radio to receive mode.  If the radio is already in receive mode, this does nothing.
 */
void _set_rx_mode(void)
{
	uint8_t config;
	_get_register(CONFIG, &config, 1);
	if ((config & _BV(PRIM_RX)) == 0)
	{
		config |= _BV(PRIM_RX);
		_set_register(CONFIG, &config, 1);
		// the radio takes 130 us to power up the receiver.
		_delay_us(65);
		_delay_us(65);
	}
}

/**
 * Switch the radio to transmit mode.  If the radio is already in transmit mode, this does nothing.
 */
void _set_tx_mode(void)
{
	uint8_t config;
	_get_register(CONFIG, &config, 1);
	if ((config & _BV(PRIM_RX)) != 0)
	{
		config &= ~_BV(PRIM_RX);
		_set_register(CONFIG, &config, 1);
		// The radio takes 130 us to power up the transmitter
		// You can delete this if you're sending large packets (I'm thinking > 25 bytes, but I'm not sure) because it
		// sending the bytes over SPI can take this long.
		_delay_us(65);
		_delay_us(65);
	}
}

/**
 * Reset the pipe 0 address if pipe 0 is enabled.  This is necessary when the radio is using Enhanced Shockburst, because
 * the pipe 0 address is set to the transmit address while the radio is transmitting (this is how the radio receives
 * auto-ack packets).
 */
void _reset_pipe0_address(void)
{
	if (rx_pipe_widths[RADIO_PIPE_0] != 0)
	{
		// reset the pipe 0 address if pipe 0 is enabled.
		_set_register(RX_ADDR_P0, (uint8_t*)rx_pipe0_address, ADDRESS_LENGTH);
	}
}

/**
 * Configure radio defaults and turn on the radio in receive mode.
 * This configures the radio to its max-power, max-packet-header-length settings.  If you want to reduce power consumption
 * or increase on-air payload bandwidth, you'll have to change the config.
 */
void _config(void)
{
	uint8_t value;

	SPI_Init();

	// set address width to 5 bytes.
	value = ADDRESS_LENGTH - 2;			// 0b11 for 5 bytes, 0b10 for 4 bytes, 0b01 for 3 bytes
	_set_register(SETUP_AW, &value, 1);

	// set Enhanced Shockburst retry to every 586 us, up to 5 times.  If packet collisions are a problem even with AA enabled,
	// then consider changing the retry delay to be different on the different stations so that they do not keep colliding on each retry.
	value = 0x15;
	//value = 0x10;
	_set_register(SETUP_RETR, &value, 1);

	// Set to use 2.4 GHz channel 110.
	value = CHANNEL;
	_set_register(RF_CH, &value, 1);

	// Set radio to 2 Mbps and high power.  Leave LNA_HCURR at its default.
	value = _BV(RF_DR) | _BV(LNA_HCURR);
	_set_register(RF_SETUP, &value, 1);

	// Enable 2-byte CRC and power up in receive mode.
	value = _BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP) | _BV(PRIM_RX);
	_set_register(CONFIG, &value, 1);

	send_instruction(FLUSH_TX, NULL, NULL, 0);
	send_instruction(FLUSH_RX, NULL, NULL, 0);
}

void Radio_Init(void)
{
	transmit_lock = 0;

	// disable radio during config
	RADIO_CE_LOW();

	// set as output AT90 pins connected to the radio's slave select and chip enable pins.
	RADIO_DDR |= _BV(RADIO_CSN) | _BV(RADIO_CE);

	// Enable radio interrupt.  This interrupt is triggered when data are received and when a transmission completes.
	DDRE &= ~_BV(PORTE4);
	EICRB |= _BV(ISC41);
	EICRB &= ~_BV(ISC40);
	EIMSK |= _BV(INT4);

	// Configure the radio registers that are not application-dependent.
	_config();

	// Wait for the radio to power up.
	_delay_ms(2);

	// enable radio as a receiver
	RADIO_CE_HIGH();
}

// default address for pipe 0 is 0xe7e7e7e7e7
// default address for pipe 1 is 0xc2c2c2c2c2
// default address for pipe 2 is 0xc2c2c2c2c3 (disabled)
// default address for pipe 3 is 0xc2c2c2c2c4 (disabled)
// default address for pipe 4 is 0xc2c2c2c2c5 (disabled)
// default address for pipe 5 is 0xc2c2c2c2c6 (disabled)
void Radio_Configure_Rx(RADIO_PIPE pipe, uint8_t* address, uint8_t enable)
{
	uint8_t value;
	uint8_t use_aa = 1;
	uint8_t payload_width = 32;
	if (payload_width < 1 || payload_width > 32 || pipe < RADIO_PIPE_0 || pipe > RADIO_PIPE_5) return;

	// store the pipe 0 address so that it can be overwritten when transmitting with auto-ack enabled.
	if (pipe == RADIO_PIPE_0)
	{
		rx_pipe0_address[0] = address[0];
		rx_pipe0_address[1] = address[1];
		rx_pipe0_address[2] = address[2];
		rx_pipe0_address[3] = address[3];
		rx_pipe0_address[4] = address[4];
	}

	// Set the address.  We set this stuff even if the pipe is being disabled, because for example the transmitter
	// needs pipe 0 to have the same address as the Tx address for auto-ack to work, even if pipe 0 is disabled.
	_set_register(RX_ADDR_P0 + pipe, address, pipe > RADIO_PIPE_1 ? 1 : ADDRESS_LENGTH);

	// Set auto-ack.
	_get_register(EN_AA, &value, 1);
	if (use_aa)
		value |= _BV(pipe);
	else
		value &= ~_BV(pipe);
	_set_register(EN_AA, &value, 1);

	// Set the pipe's payload width.  If the pipe is being disabled, then the payload width is set to 0.
	value = enable ? payload_width : 0;
	_set_register(RX_PW_P0 + pipe, &value, 1);
	rx_pipe_widths[pipe] = value;

	// Enable or disable the pipe.
	_get_register(EN_RXADDR, &value, 1);
	if (enable)
		value |= _BV(pipe);
	else
		value &= ~_BV(pipe);
	_set_register(EN_RXADDR, &value, 1);
}

// default transmitter address is 0xe7e7e7e7e7.
void Radio_Set_Tx_Addr(uint8_t* address)
{
	tx_address[0] = address[0];
	tx_address[1] = address[1];
	tx_address[2] = address[2];
	tx_address[3] = address[3];
	tx_address[4] = address[4];
	_set_register(TX_ADDR, address, ADDRESS_LENGTH);
}

void Radio_Configure(RADIO_DATA_RATE dr, RADIO_TX_POWER power)
{
	uint8_t value;

	if (power < RADIO_LOWEST_POWER || power > RADIO_HIGHEST_POWER || dr < RADIO_1MBPS || dr > RADIO_2MBPS) return;

	// set the address
	//Radio_Set_Tx_Addr(address);

	// set the data rate and power bits in the RF_SETUP register
	_get_register(RF_SETUP, &value, 1);

	value |= 3 << RF_PWR;			// set the power bits so that the & will mask the power value in properly.
	value &= power << RF_PWR;		// mask the power value into the RF status byte.

	if (dr)
		value |= _BV(RF_DR);
	else
		value &= ~_BV(RF_DR);

	_set_register(RF_SETUP, &value, 1);
}

uint8_t Radio_Transmit(radiopacket_t* payload, RADIO_TX_WAIT wait)
{
	//if (block && transmit_lock) while (transmit_lock);
	//if (!block && transmit_lock) return 0;
	uint8_t len = 32;

	// indicate that the driver is transmitting.
    transmit_lock = 1;

	// disable the radio while writing to the Tx FIFO.
    RADIO_CE_LOW();

	_set_tx_mode();

    // for auto-ack to work, the pipe0 address must be set to the Tx address while the radio is transmitting.
    // The register will be set back to the original pipe 0 address when the TX_DS or MAX_RT interrupt is asserted.
    _set_register(RX_ADDR_P0, (uint8_t*)tx_address, ADDRESS_LENGTH);

    // enable SPI
    RADIO_CSN_LOW();
    // send the "write transmit payload" instruction.
    SPI_Write_Byte(W_TX_PAYLOAD);
    // write the payload to the Tx FIFO
    SPI_Write_Block((uint8_t*)payload,len);
    // disable SPI
    RADIO_CSN_HIGH();

    // start the transmission.
    RADIO_CE_HIGH();

    if (wait == RADIO_WAIT_FOR_TX)
    {
    	while (transmit_lock);
    	return tx_last_status;
    }

    return RADIO_TX_SUCCESS;
}

RADIO_RX_STATUS Radio_Receive(radiopacket_t* buffer)
{
	uint8_t len = 32;
	uint8_t status;
	uint8_t pipe_number;
	uint8_t doMove = 1;
	RADIO_RX_STATUS result;

	/*if (block)	// I don't think you can send the R_RX_PAYLOAD instruction when the radio is transmitting.
	{
		//request_radio();
		while (transmit_lock);
	}
	else
	{
		if (transmit_lock) return RADIO_RX_TRANSMITTING;
	}*/

	transmit_lock = 0;

	RADIO_CE_LOW();

    status = _get_status();
	pipe_number =  (status & 0xE) >> 1;

	if (pipe_number == RADIO_PIPE_EMPTY)
	{
		result = RADIO_RX_FIFO_EMPTY;
		doMove = 0;
	}

	if (rx_pipe_widths[pipe_number] > len)
	{
		// the buffer isn't big enough, so don't copy the data.
		result = RADIO_RX_INVALID_ARGS;
		doMove = 0;
	}

	if (doMove)
	{
		// Move the data payload into the local
		send_instruction(R_RX_PAYLOAD, (uint8_t*)buffer, (uint8_t*)buffer, rx_pipe_widths[pipe_number]);

		status = _get_status();
		pipe_number =  (status & 0xE) >> 1;

		if (pipe_number != RADIO_PIPE_EMPTY)
			result = RADIO_RX_MORE_PACKETS;
		else
			result = RADIO_RX_SUCCESS;
	}

	RADIO_CE_HIGH();

	transmit_lock = 0;

	//release_radio();

	return result;
}

// This is only accurate if all the failed packets were sent using auto-ack.
uint8_t Radio_Drop_Rate(void)
{
	uint16_t wh = tx_history;
	uint8_t weight = 0;
	while (wh != 0)
	{
		if ((wh & 1) != 0) weight++;
		wh >>= 1;
	}
	wh = (16 - weight) * 100;
	wh /= 16;
	return wh;
}

// Interrupt handler
ISR(INT4_vect)
{
    uint8_t status;
    uint8_t pipe_number;
    //uint8_t data[32];

    RADIO_CE_LOW();

    status = _get_status();

    if (status & _BV(RX_DR))
    {
    	//PORTD ^= _BV(PORTD4);
    	pipe_number =  (status & 0xE) >> 1;
    	radio_rxhandler(pipe_number);
    	/*if (pipe_number > RADIO_PIPE_5) return;	// TODO: When this is integrated into the OS, make an OS_Abort() call.
        // enable SPI
        RADIO_CSN_LOW();
        // read the payload into the MCU buffer
    	SPI_Write_Byte(R_RX_PAYLOAD);
    	SPI_ReadWrite_Block(data, data, rx_pipe_widths[pipe_number]);
        // resynch SPI
        RADIO_CSN_HIGH();*/
    }
    // We can get the TX_DS or the MAX_RT interrupt, but not both.
    if (status & _BV(TX_DS))
    {
    	//PORTD ^= _BV(PORTD6);
    	//_get_register(FIFO_STATUS, &fifo_status, 1);
    	//if (fifo_status & _BV(TX_FIFO_EMPTY))
    	{
    		// if there's nothing left to transmit, switch back to receive mode.
    		transmit_lock = 0;
    		_reset_pipe0_address();
    		_set_rx_mode();
    	}
    	// indicate in the history that a packet was transmitted successfully by appending a 1.
    	tx_history <<= 1;
    	tx_history |= 1;

    	tx_last_status = RADIO_TX_SUCCESS;
    }
    else if (status & _BV(MAX_RT))
    {
    	//PORTD ^= _BV(PORTD7);

        // enable SPI
        RADIO_CSN_LOW();
        // flush the failed packet (it stays in the Tx FIFO; we could try to resend it by setting CE high)
        SPI_Write_Byte( FLUSH_TX );
        // resynch SPI
        RADIO_CSN_HIGH();

    	transmit_lock = 0;
    	_reset_pipe0_address();
		_set_rx_mode();
    	// indicate in the history that a packet was dropped by appending a 0.
    	tx_history <<= 1;

    	tx_last_status = RADIO_TX_MAX_RT;
    }

    // clear the interrupt flags.
	status = _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT);
	_set_register(STATUS, &status, 1);

    RADIO_CE_HIGH();
}

