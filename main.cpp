/*
 * main.c
 *
 *  Created on: 5 de Mai de 2013
 *      Author: brNX
 */

#include <avr/io.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */

extern "C" {

#include "report.h"
#include "n64gc.h"
#include "USART_Int_atmega328.h"
#include "rf24/timer2.h"

}

#include "rf24/RF24.h"

static	report_t reportBuffer;

volatile char rbuffer[10];
volatile unsigned int tyup=0;

void ReadController(){
	reportBuffer.y = reportBuffer.x = reportBuffer.b1 = reportBuffer.b2 = 0;
	reportBuffer.rx = reportBuffer.ry = 0;
	reportBuffer.hat = -1;

    ReadN64GC(&reportBuffer);
}


void HardwareInit(){

	USART_Init(16); //115200 8N1 (16MHz crystal)

	timer2_setup();

	// See schmatic for connections
	DDRD	= 0;
	PORTD	= 0xFF;	// All inputs with pull-ups

	DDRB	&= ~(1<<PB0);
	PORTB	|=  (1<<PB0);	// PB0 input with pull-up

	sei();
}


int main (){


	HardwareInit();

	RF24 radio = RF24();


	while(1){
		ReadController();
		_delay_ms(1000);
	}

	return 0;
}

ISR(USART_RX_vect)
{
	char received = UDR0;
	rbuffer[tyup] = received ; // Fetch the received byte value into the variable " ByteReceived "
	tyup++;
}
