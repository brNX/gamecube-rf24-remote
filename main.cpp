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
#include <avr/sleep.h>
#include <avr/power.h>

extern "C" {

#include "report.h"
#include "n64gc.h"
#include "USART_Int_atmega328.h"
#include "rf24/timer2.h"

}

#include "rf24/RF24.h"

static	report_t reportBuffer;

//
// Sleep declarations
//

typedef enum { wdt_16ms = 0, wdt_32ms, wdt_64ms, wdt_128ms, wdt_250ms, wdt_500ms, wdt_1s, wdt_2s, wdt_4s, wdt_8s } wdt_prescalar_e;

void setup_watchdog(uint8_t prescalar);
void do_sleep(void);



void ReadController(){
	reportBuffer.y = reportBuffer.x = reportBuffer.b1 = reportBuffer.b2 = 0;
	reportBuffer.rx = reportBuffer.ry = 0;
	reportBuffer.hat = -1;

	ReadN64GC(&reportBuffer);
}


void HardwareInit(){

	USART_Init(1); //(16)115200 8N1 (16MHz crystal)

	timer2_setup();

	// See schmatic for connections
	DDRD	= 0;
	PORTD	= 0xFF;	// All inputs with pull-ups

	DDRB	&= ~(1<<PB0);
	PORTB	|=  (1<<PB0);	// PB0 input with pull-up

	//status led
	DDRC	|= (1<<PC0);

	sei();
}




int main (){


	HardwareInit();

	RF24 radio = RF24();

	// Radio pipe addresses for the 2 nodes to communicate.
	const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

	setup_watchdog(wdt_16ms);

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

    radio.stopListening();

    unsigned long  pa = 0;

	while(1){

		PORTC |= (1<<PC0);

     	char temp[100];

		ReadController();

		//sprintf(temp,"x:%d y:%d rx:%d ry:%d b1:%d b2:%d\n",(int8_t)reportBuffer.x,(int8_t)reportBuffer.y,(int8_t)reportBuffer.rx,(int8_t)reportBuffer.ry,reportBuffer.b1,reportBuffer.b2);
		//print_string(temp);



        bool ok = radio.write( &reportBuffer, sizeof(report_t) );

		if (ok){
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
	      //print_string("o\n");
	    }
	    timer2_stop();

	    radio.stopListening();

	    PORTC &= ~(1<<PC0);

	    do_sleep();

	}

	return 0;
}


//
// Sleep helpers
//

// 0=16ms, 1=32ms,2=64ms,3=125ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec

void setup_watchdog(uint8_t prescalar)
{
  prescalar = min(9,prescalar);
  uint8_t wdtcsr = prescalar & 7;
  if ( prescalar & 8 )
    wdtcsr |= _BV(WDP3);

  MCUSR &= ~_BV(WDRF);
  WDTCSR = _BV(WDCE) | _BV(WDE);
  WDTCSR = _BV(WDCE) | wdtcsr | _BV(WDIE);
}

ISR(WDT_vect)
{
  ;
}

void do_sleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();

  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution here when watchdog timed out
}
