/*
 * USARTatmega328.h
 *
 *  Created on: Feb 5, 2011
 *      Author: bgouveia
 */


void USART_Init(uint16_t ubrr);

//This function is used to read the available data
//from USART. This function will wait until data is
//available.
char USARTReadChar_blocking();

//This function writes the given "data" to
//the USART which then transmit it via TX line
void USARTWriteChar(char data);


void print_string(const unsigned char *data);
