/**
 * @file   radio.c
 *
 * @brief  QKits TXRX24G transceiver module
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 *
 * @author Scott Craig
 * @author Justin Tanner
 * @author Leanne Ross
 * @author Mantis Cheng
 *
 * @section version_history Version History
 *
 * 1.0 initial design by Scott Craig
 * 1.1 removed radio_print()
 * 1.2 clean up and refactoring
 */

#include "radio.h"
#include "common.h"

/* static foward declarations */
static inline void delay_125ns(void);
static inline void delay_500ns(void);
static inline void delay_1us(void);
static void put_byte(uint8_t rfbyte);
static void set_config_mode(void);
static void set_rxtx_mode(void);
static int radio_write(uint8_t * const arr, int num_bytes);

/** Configuration data defined in radio.h */
static uint8_t radio_cfg[CFG_VECT_LEN] =
{
   DATA2_W,    /* byte 0 */
   DATA1_W,    /* byte 1 */
   ADDR2,      /* bytes 2 to 6 */
   ADDR1,      /* bytes 7 to 11 */
   ADDRW_CRC,  /* byte 12 */
   MODES,      /* byte 13 */
   RFCH_RXEN   /* byte 14 */
};

/**
 * @brief Delay. Assumes CPU frequency is 8 MHz
 */
static inline void delay_125ns(void)
{
   __asm__ volatile ("nop\n\t"::);
} 

/**
 * @brief Delay. Assumes CPU frequency is 8 MHz
 */
static inline void delay_500ns(void)
{
   __asm__ volatile (
      "nop\n\t"
      "nop\n\t"
      "nop\n\t"
      "nop\n\t"
      ::
                     );
} 

/**
 * @brief Delay. Assumes CPU frequency is 8 MHz
 */
static inline void delay_1us(void)
{
   __asm__ volatile (
      "nop\n\t"
      "nop\n\t"
      "nop\n\t"
      "nop\n\t"
      "nop\n\t"
      "nop\n\t"
      "nop\n\t"
      "nop\n\t"
      ::
                     );
} 

/**
 * @brief Write one byte into the payload of the radio packet
 * one bit at a time. This is the lowest level write function.
 *
 * @param rfbyte Byte to be loaded into payload for transmission.
 */
static void put_byte(uint8_t rfbyte)
{
   int shift;

   for(shift = 7; shift >= 0; --shift)
   {
      if(rfbyte & _BV(shift))
      {
         DATA_HIGH();
      }
      else
      {
         DATA_LOW();
      }
      CLK1_HIGH();
      delay_500ns();
      CLK1_LOW();
   }
} 

/**
 * @brief Read one byte from the payload of the radio packet
 * one bit at a time. This is the lowest level read function.
 *
 * @return single byte from the radio
 */
uint8_t radio_get_byte(void)
{
   int shift;
   unsigned char rfbyte = 0x0;

   for(shift = 7; shift >= 0; --shift)
   {
      CLK1_HIGH();
      if (DATA_PIN & _BV(DATA_PINNUM))
      {
         rfbyte |= _BV(shift);
      }
      CLK1_LOW();
   }
   return rfbyte;
} 

/**
 * @brief Switch the radio into standby mode (ie radio is off).
 */
void set_standby_mode(void)
{
   /* Disable external interrupt for INT4 */
   EIMSK &= ~_BV(INT4);

   CS_LOW();
   CE_LOW();
   CLK1_LOW();
   DATA_LOW();
} 


/**
 * @brief Switch the radio into configuration mode.
 *
 * This is necessary when initializing the radio or when changing
 * between radio RX (receive) and TX (transmit) modes.
 */
static void set_config_mode(void)
{
   CE_LOW();

   /* Set DATA pin to output */
   DATA_DDR |= _BV(DATA_PINNUM);

   DATA_LOW();
   CLK1_LOW();
   delay_125ns();     /* Td = 50 ns */
   CS_HIGH();
   _delay_us(5);     /* Tcs2data = 5 us */
} 

/**
 * @brief Switch the radio into active mode.
 */
static void set_rxtx_mode(void)
{
   CS_LOW();
   delay_125ns(); /* Td = 50 ns */
   DATA_LOW();
   CLK1_LOW();
   CE_HIGH();
   _delay_us(5); /* Tce2data = 5 us */
} 

/**
 * @brief This function is called to configure the radio. Once this
 * function has been completed, the radio can be put in transmit or
 * receive mode to start communicating with other devices.
 *
 * @param address A 16-bit integer specifying the address of 'this'
 *                radio transceiver. For example, the main program
 *                would declare: uint8_t addr[] = {0xAB,0xBA};
 *
 * @param rx_enable A 1-bit integer specificying the mode of the radio;
 *			0 for transmit, anything else for receive.
 */
int radio_init(uint16_t address, uint8_t rx_enable)
{
   uint8_t* txbyte = radio_cfg;
   uint8_t count = CFG_VECT_LEN;

   /* Set up config vector */
   radio_cfg[10] = (uint8_t)(address >> 8);
   radio_cfg[11] = (uint8_t) address;
   radio_cfg[14] = (uint8_t)((RADIO_CHANNEL << 1) | rx_enable);

   if (rx_enable)
   {
      /* Rising edge trigger on INT4 (1,1) for DATA pin of radio */
      EICRB |= (_BV(ISC41) | _BV(ISC40));
   }

   /* Set up output pins */
   CLK1_DDR |= _BV(CLK1_PINNUM);
   CE_DDR    |= _BV(CE_PINNUM);
   CS_DDR |= _BV(CS_PINNUM);
   DATA_DDR |= _BV(DATA_PINNUM);

   _delay_ms(3); /* Tpd2sby = 3 ms (power-on-to-standby waiting time) */

   set_config_mode();

   /* Store the configuration data in the radio packet */
   while (count)
   {
      put_byte(*txbyte);
      ++txbyte;
      --count;
   }

   set_standby_mode();

   /* Set DATA pin to input or output */
   if (rx_enable)
   {
      DATA_DDR &= ~(_BV(DATA_PINNUM));
   }
   else
   {
      DATA_DDR |= _BV(DATA_PINNUM);
   }

   set_rxtx_mode();

   /* Enable external interrupt on INT4 when in RX mode */
   if (rx_enable)
   {
      EIMSK |= _BV(INT4);
   }

   return 0;
} 

/**
 * @brief Switch the radio into transmit mode.
 */
void radio_set_transmit(void)
{
   set_config_mode();
   put_byte(RADIO_CHANNEL << 1);
   set_standby_mode();

   /* Set DATA pin to output */
   DATA_DDR |= _BV(DATA_PINNUM);

   /* Disable external interrupt on INT4 */
   EIMSK &= ~(_BV(INT4));

   set_rxtx_mode();
} 

/**
 * @brief Switch the radio into receive mode.
 */
void radio_set_receive(void)
{
   int i;

   set_config_mode();
   put_byte((RADIO_CHANNEL << 1) | 0x1);
   set_standby_mode();

   /* Set DATA pin to input */
   DATA_DDR &= ~_BV(DATA_PINNUM);

   /* Enable external interrupt on INT4 */
   EIMSK |= _BV(INT4);

   set_rxtx_mode();

   for (i=0; i<250; ++i)
   {
      delay_1us(); // Tsby2rx = 202 us
   }
} 

/**
 * @brief This function is called to write the payload.
 * This is the mid-level write routine. It is intentionally static
 * because you should use the radio_send() function for transmitting
 * data. Send takes care of first setting the destination address
 * of the payload.
 *
 * @param arr The character array to be used for populating the payload.
 *
 * @param num_bytes The number of bytes to write into the payload, padding
 *     with zeros if less than PAYLOAD_BYTES. This value cannot be greater
 *     than PAYLOAD_BYTES!
 */
static int radio_write(uint8_t * const arr, int num_bytes)
{
   int i;

   set_rxtx_mode();

   /* Check size of the payload. Cannot be more than PAYLOAD_BYTES */
   if(num_bytes > PAYLOAD_BYTES)
   {
      num_bytes = PAYLOAD_BYTES;
   }

   /* Copy array into payload */
   for(i = 0; i < num_bytes; ++i)
   {
      put_byte( arr[i] );
   }

   return 0;
} 

/**
 * @brief This function is called to transmit a radio packet.
 * This is the high-level send routine. It writes the destination
 * address followed by the desired data (payload) and then clocks
 * it out using shockburst transmission.
 *
 * @param addr_destination A pointer to an array of length 2 (2-bytes)
 *    specifying the address of the destination radio transceiver. For
 *    example, the main program would declare:
 *    uint8_t dest_addr[] = {0xAB,0xBA};
 *
 * @param arr The character array to be used for populating the payload.
 *
 * @param num_bytes The number of bytes message will occupy in payload.
 *      This value cannot be greater than PAYLOAD_BYTES!
 */
void radio_send(uint16_t const addr_destination, uint8_t * const arr)
{
   /* Set radio to transmit mode */
   radio_set_transmit();

   CE_HIGH();
   _delay_ms(1);

   /* Write the high 8 bits of the address and then the low 8 bits */
   put_byte((uint8_t)(addr_destination >> 8));
   put_byte((uint8_t)addr_destination);

   /* Write the payload data */
   radio_write(arr, PAYLOAD_BYTES);

   /* Start shockburst transmission */
   CE_LOW();

   _delay_ms(3);         // Tsby2txSB+Toa

   set_standby_mode();
}


