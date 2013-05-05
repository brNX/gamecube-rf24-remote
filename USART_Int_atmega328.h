/*
 * USARTatmega328.h
 *
 *  Created on: Feb 5, 2011
 *      Author: bgouveia
 */
#ifndef UART_ATMEGA
#define UART_ATMEGA
#include <stdio.h>

void USART_Init(uint16_t ubrr);

//This function is used to read the available data
//from USART. This function will wait until data is
//available.
char USARTReadChar_blocking();

//This function writes the given "data" to
//the USART which then transmit it via TX line
void USARTWriteChar(char data);


void print_string(const char *data);

void usart_putchar(char data);

char usart_getchar(void);

unsigned char usart_kbhit(void);

void usart_pstr(char *s);

// this function is called by printf as a stream handler
int usart_putchar_printf(char var, FILE *stream);



#endif
