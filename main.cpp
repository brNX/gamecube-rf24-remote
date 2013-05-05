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
#include "psx.h"
#include "USART_Int_atmega328.h"

}

#include "rf24/RF24.h"

static	report_t reportBuffer;

volatile char rbuffer[10];
volatile unsigned int tyup=0;

void ReadController(){
	reportBuffer.y = reportBuffer.x = reportBuffer.b1 = reportBuffer.b2 = 0;
	reportBuffer.rx = reportBuffer.ry = 0;
	reportBuffer.hat = -1;

	//ReadPSX(&reportBuffer);
	ReadN64GC(&reportBuffer);
}


void HardwareInit(){

	USART_Init(16); //115200 8N1 (16MHz crystal)


	// See schmatic for connections
	DDRD	= 0b00000000;
	PORTD	= 0b11111001;	// All inputs with pull-ups except USB D+/D-

	DDRB	= 0b00000000;
	PORTB	= 0b00111111;	// All inputs with pull-ups except xtal

	DDRC	= 0b00000000;
	PORTC	= 0b00111111;	// All inputs except unused bits

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
