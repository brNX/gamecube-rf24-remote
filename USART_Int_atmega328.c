/*
 * USARTatmega328.c
 *
 *  Created on: Feb 5, 2011
 *      Author: bgouveia
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "USART_Int_atmega328.h"



void USART_Init(uint16_t ubrr)
{

	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;


	/*
>> Asynchronous mode
>> No Parity
>> 1 StopBit
	 */


	/* Set frame format: 8data, 1stop bit */
	UCSR0C = (1<<UCSZ01)| (1<<UCSZ00);

	/*Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);

	//Enable double transmission speed
	UCSR0A |= (1<<U2X0);

	//enable receive interrupt
	UCSR0B |= (1 << RXCIE0 );

}

//This function is used to read the available data
//from USART. This function will wait until data is
//available.
char USARTReadChar_blocking()
{
	//Wait until a data is available
	while(!(UCSR0A & (1<<RXC0))){/*Do nothing*/ }

	//Now USART has got data from host
	//and is available is buffer
	return UDR0;
}


//This function writes the given "data" to
//the USART which then transmit it via TX line
void USARTWriteChar(char data)
{
   //Wait untill the transmitter is ready
   while(!(UCSR0A & (1<<UDRE0)))
   {
      //Do nothing
   }

   //Now write the data to USART buffer

   UDR0=data;
}


void print_string(const unsigned char *data) {
	while (*data != '\0') {
		USARTWriteChar(*data++);
	}
}
