/*
 * main.c
 *
 *  Created on: 5 de Mai de 2013
 *      Author: brNX
 */

#include <avr/io.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <stdio.h>
#include <stdbool.h>

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

	// Radio pipe addresses for the 2 nodes to communicate.
	const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

	print_string("main\n");


	//
	// Setup and configure rf radio
	//
	radio.begin();

	// optionally, increase the delay between retries & # of retries
	radio.setRetries(15,15);

	// optionally, reduce the payload size.  seems to
	// improve reliability
	radio.setPayloadSize(8);

	radio.setDataRate(RF24_2MBPS);

    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);

    radio.startListening();

    unsigned long  pa = 0;

	while(1){

		ReadController();

		// First, stop listening so we can talk.
		radio.stopListening();


		pa++;
		bool ok = radio.write( &pa, sizeof(unsigned long) );

		if (ok){
			print_string("pa sent\n");
		}
		else{
			print_string("pa error\n");
		}


		radio.startListening();

	    // Wait here until we get a response, or timeout (250ms)
	    bool timeout = false;
	    timer2_start();
	    while ( ! radio.available() && ! timeout ){
	      if (timer2_gettick() > 200 )
	        timeout = true;
	    }

	    // Describe the results
	    if ( timeout )
	    {
	      print_string("Failed, response timed out.\n\r");
	    }
	    else
	    {
	      // Grab the response, compare, and send to debugging spew
	      unsigned long got_time;
	      radio.read( &got_time, sizeof(unsigned long) );

	      // Spew it
	      char debug[40];
	      sprintf(debug,"Got response %lu\n\r",got_time);
	      print_string(debug);
	    }
	    timer2_stop();

		_delay_ms(1000);
	}

	return 0;
}
