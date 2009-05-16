#include "uart.h"

static char buffer[TX_BUFSIZE];
char dataIn=-1;
int mode = 0;
int newData = 0; // if there is new data, set to 1
static int head = 0;
static int tail = 0;
static int missed = 0;
static int sent = 0;

/*This function returns the number of successfully send strings*/
int uart_getsent()
{
    return sent;
}

/*This function returns the total number of attempted send strings*/
int uart_gettotal()
{
    return missed + sent;
}

/*This function returns the number of missed strings because buffer was full*/
int uart_getmissed()
{
	return missed;
}

/*This function returns the last received character*/
char uart_getchar()
{
    if(mode == 0 || mode == 1){
		if(newData){
		   newData = 0;
		   return dataIn;
		}else{
	       return -1;
		}
	}
	return -1;
}

/*
 * Initialize the UART to 9600 Bd, tx/rx, 8N1. And set its mode to 0 or 1
 */
void uart_init(int m)
{
    CLKPR = 0x80; // clock prescaler
	CLKPR = 0x00;

	UCSR1A = (0<<U2X1); //
  	UBRR1L = 51;
	UCSR1B = (1<<RXEN1)|(1<<TXEN1)|(1<<RXCIE1)|(1<<TXCIE1)|(1<<UDRIE1)|(0<<UCSZ12);

	/* Set frame format: 8data, 3stop bit */
	UCSR1C = (0<<UMSEL11)|(0<<UMSEL10)|(0<<UPM10)|(0<<USBS1)|(1<<UCSZ10)|(1<<UCSZ11)|(0<<UCPOL1);
    mode = m;
}

/*The interrupt is generated when ever the the data register is empty
 *this interrupt will run through this function
 */
ISR( USART1_UDRE_vect )
{
    if(mode == 0){
	       UCSR1B |= (1<<RXCIE1); //Receive buffer is clear, re-enable
    }else if(mode == 1){
       if( head != tail ){
			if( head<tail ){
                uart_putchar( buffer[head] );
				head++;
			}else{
                if( head < TX_BUFSIZE ){
                    uart_putchar(buffer[head]);
				    head++;
				}else{
                    head=0;
                    uart_putchar(buffer[head]);
				    head++;
				}
			}
	   }
	}
}

ISR( USART1_TX_vect ){};

/*This is the receive interrupt handler.  It is called when ever the Receive flag is set*/
ISR( USART1_RX_vect )
{
    if(mode == 0){
	   newData = 1;
	   dataIn = UDR1;
	   uart_putchar(dataIn);
	}else if(mode == 1){
       newData = 1;
	   dataIn = UDR1;
	}
}

/*
 * Send character c down the UART Tx, wait until tx holding register
 * is empty.
 */
int uart_putchar( char c )
{
	if(mode == 0){
	     UCSR1B &= ~(_BV(RXCIE1)); //disble RX interrupt if in mode 0
	     UDR1 = c;
	}else if( mode ==1 ){
         UDR1 = c;
	}
  	return 0;
}

/*This function allows a string to inserted into the send buffer*/
void uart_putstringln(char str[])
{
	UCSR1B &= ~(_BV(UDRIE1)); //disble UDRIE interrupt
	if(mode == 1){
		int i = 0;
		int length = strlen(str);
		int bufferfree = getBufferFree();
		if(length < (bufferfree-5)){
		    sent++;
	        for(i=0;i<length;i++){
				if(tail == TX_BUFSIZE){
	                 tail = 0;
				}
				buffer[tail] = str[i];
				tail++;
			}
		}else{
            missed++;
		}
	}
	UCSR1B |= (_BV(UDRIE1)); //enable UDRIE interrupt if in mode 0
}

/*This functions determines how much space is left in the send buffer*/
int getBufferFree()
{
    if(tail>=head)
       return TX_BUFSIZE - (tail - head);
	else
	   return head - tail;
}

/*This formats the string so it acts like the printf function*/
void uart_println(char str[], ...)
{
	va_list arg_list;
    va_start (arg_list, str);
	char str1[TX_BUFSIZE];
	vsprintf(str1,str, arg_list);
	va_end (arg_list);
	uart_putstringln(str1);
}

