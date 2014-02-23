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
#include <limits.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/eeprom.h>

extern "C" {
#include "report.h"
#include "n64gc.h"
#include "USART_Int_atmega328.h"
#include "rf24/timer2.h"
}

#include "rf24/RF24.h"

#define LED1 PB6
#define LED2 PB7

#define EEPROM_MAGIC_NUMBER 20

static	report_t reportBuffer;

#define START_PRESSED 	(reportBuffer.b2 & (1<<0))
#define Z_PRESSED 		(reportBuffer.b2 & (1<<1))
#define A_PRESSED 		(reportBuffer.b1 & (1<<0))
#define B_PRESSED 		(reportBuffer.b1 & (1<<1))
#define X_PRESSED 		(reportBuffer.b1 & (1<<3))
#define Y_PRESSED 		(reportBuffer.b1 & (1<<4))
#define L_PRESSED 		(reportBuffer.b1 & (1<<6))
#define R_PRESSED 		(reportBuffer.b1 & (1<<7))


//digital pad
#define UP_PRESSED		(reportBuffer.hat==0)
#define DOWN_PRESSED	(reportBuffer.hat==4)
#define LEFT_PRESSED	(reportBuffer.hat==6)
#define RIGHT_PRESSED	(reportBuffer.hat==2)
#define URIGHT_PRESSED	(reportBuffer.hat==1)
#define DRIGHT_PRESSED	(reportBuffer.hat==3)
#define ULEFT_PRESSED	(reportBuffer.hat==7)
#define DLEFT_PRESSED	(reportBuffer.hat==5)

#define DEADZONE 7


//
// Sleep declarations
//

typedef enum { wdt_16ms = 0, wdt_32ms, wdt_64ms, wdt_128ms, wdt_250ms, wdt_500ms, wdt_1s, wdt_2s, wdt_4s, wdt_8s } wdt_prescalar_e;

void setup_watchdog(uint8_t prescalar);
void do_sleep(void);
void ReadController();
long map(long x, long in_min, long in_max, long out_min, long out_max);
void HardwareInit();
void calibrate();
void convertAxes();
void sendData(bool askforStatus,RF24 & radio);


char maxy=CHAR_MIN,miny=CHAR_MAX,minx=CHAR_MAX,maxx=CHAR_MIN,minrx=CHAR_MAX,maxrx=CHAR_MIN,minry=CHAR_MAX,maxry=CHAR_MIN;
bool calibrated = false;

long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


int main (){


	HardwareInit();

	RF24 radio = RF24();
	// Radio pipe addresses for the 2 nodes to communicate.
	const uint64_t pipes[2] = { 0xF0F0F0F0BELL, 0xF0F0F0F0EFLL };

	setup_watchdog(wdt_64ms);

	//
	// Setup and configure rf radio
	//
	radio.begin();

	radio.setChannel(100);

	// optionally, increase the delay between retries & # of retries
	radio.setRetries(15,15);

	// optionally, reduce the payload size.  seems to
	// improve reliability
	radio.setPayloadSize(sizeof(report_t));

	radio.setDataRate(RF24_250KBPS);
	radio.openWritingPipe(pipes[0]);
	radio.openReadingPipe(1,pipes[1]);
	radio.startListening();
	radio.stopListening();

	//unsigned long  pa = 0;
	uint8_t counter = 0;


	//check eeprom
	if (eeprom_read_byte(0)==EEPROM_MAGIC_NUMBER){
		minx=(char)eeprom_read_byte((uint8_t*)1);
		maxx=(char)eeprom_read_byte((uint8_t*)2);
		miny=(char)eeprom_read_byte((uint8_t*)3);
		maxy=(char)eeprom_read_byte((uint8_t*)4);
		minrx=(char)eeprom_read_byte((uint8_t*)5);
		maxrx=(char)eeprom_read_byte((uint8_t*)6);
		minry=(char)eeprom_read_byte((uint8_t*)7);
		maxry=(char)eeprom_read_byte((uint8_t*)8);
		calibrated=true;

#ifdef DEBUG
		print_string("read eeprom magic number");
		char temp[100];
		sprintf(temp,"minx:%d maxx:%d minry:%d maxry:%d \n",minx,maxx,minry,maxry);
		print_string(temp);
		_delay_ms(2000);
#endif

	}


	while(1){

		ReadController();
#ifdef DEBUG
		char temp[100];
		print_string("before convert\n");
		sprintf(temp,"hat:%u x:%d y:%d rx:%d ry:%d b1:%u b2:%u\n",reportBuffer.hat,(int8_t)reportBuffer.x,(int8_t)reportBuffer.y,(int8_t)reportBuffer.rx,(int8_t)reportBuffer.ry,reportBuffer.b1,reportBuffer.b2);
		print_string(temp);
#endif

		convertAxes();

#ifdef DEBUG
		sprintf(temp,"hat:%u x:%d y:%d rx:%d ry:%d b1:%u b2:%u\n",reportBuffer.hat,(int8_t)reportBuffer.x,(int8_t)reportBuffer.y,(int8_t)reportBuffer.rx,(int8_t)reportBuffer.ry,reportBuffer.b1,reportBuffer.b2);
		print_string(temp);
#endif

		//Calibration
		if(A_PRESSED && B_PRESSED && Z_PRESSED && L_PRESSED && R_PRESSED){
			calibrate();
		}

		counter=(counter+1)%20;
		sendData(counter==0,radio);


		do_sleep();
	}
	//never reached
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


inline void ReadController(){
	reportBuffer.y = reportBuffer.x = reportBuffer.b1 = reportBuffer.b2 = 0;
	reportBuffer.rx = reportBuffer.ry = 0;
	reportBuffer.hat = -1;

	ReadN64GC(&reportBuffer);
}


void HardwareInit(){

#ifdef DEBUG
	USART_Init(16); //(16)115200 8N1 (16MHz crystal)
#endif

	timer2_setup();

	DDRD	= (1<<LED1) | (1<<LED2) ; //status led1 & 2 output
	PORTD	= 0;

	DDRB	&= ~(1<<PB0);
	PORTB	|=  (1<<PB0);	// PB0 input with pull-up

	sei();
}

void calibrate()
{

#ifdef DEBUG
	print_string("calibration entered\n");
#endif
	PORTD |= (1 << LED2);
	PORTD |= (1 << LED1);

	maxx=0;
	minx=255;
	maxy=0;
	miny=255;
	maxrx=0;
	minrx=255;
	maxry=0;
	minry=255;

	while (1)
	{
		ReadController();
		if (START_PRESSED)
			break;

		if (reportBuffer.x > maxx)
		{
			maxx = reportBuffer.x;
		}
		if (reportBuffer.rx > maxrx)
		{
			maxrx =  reportBuffer.rx;
		}
		if ( reportBuffer.y > maxy)
		{
			maxy =  reportBuffer.y;
		}
		if ( reportBuffer.ry > maxry)
		{
			maxry =  reportBuffer.ry;
		}
		if ( reportBuffer.x < minx)
		{
			minx =  reportBuffer.x;
		}
		if ( reportBuffer.rx < minrx)
		{
			minrx =  reportBuffer.rx;
		}
		if ( reportBuffer.y < miny)
		{
			miny =  reportBuffer.y;
		}
		if ( reportBuffer.ry < minry)
		{
			minry =  reportBuffer.ry;
		}

		PORTD ^= (1 << LED2);
		_delay_ms(64);
	}
	calibrated = true;

	if (eeprom_read_byte(0)!=EEPROM_MAGIC_NUMBER){
		eeprom_write_byte(0,EEPROM_MAGIC_NUMBER);
	}
	eeprom_write_byte((uint8_t*)1,(uint8_t)minx);
	eeprom_write_byte((uint8_t*)2,(uint8_t)maxx);
	eeprom_write_byte((uint8_t*)3,(uint8_t)miny);
	eeprom_write_byte((uint8_t*)4,(uint8_t)maxy);
	eeprom_write_byte((uint8_t*)5,(uint8_t)minrx);
	eeprom_write_byte((uint8_t*)6,(uint8_t)maxrx);
	eeprom_write_byte((uint8_t*)7,(uint8_t)minry);
	eeprom_write_byte((uint8_t*)8,(uint8_t)maxry);
	calibrated=true;

#ifdef DEBUG
		char temp[100];
		sprintf(temp,"min:%d %d max:%d %d minr;%d %d maxr: %d %d\n",minx,miny,maxx,maxy,minrx,minry,maxrx,maxry);
		print_string(temp);
#endif



	PORTD &= ~(1 << LED2);
	PORTD &= ~(1 << LED1);
}

inline char clamp(char x, char min, char max)
{
    return x < min ? min : (x > max ? max : x);
}


inline void convertAxes()
{
	//DEADZONE check
	if (reportBuffer.x >= -DEADZONE && reportBuffer.x <= DEADZONE){
		reportBuffer.x = 0;
	}
	if (reportBuffer.y >= -DEADZONE && reportBuffer.y <= DEADZONE){
		reportBuffer.y = 0;
	}
	if (reportBuffer.rx >= -DEADZONE && reportBuffer.rx <= DEADZONE){
		reportBuffer.rx = 0;
	}
	if (reportBuffer.ry >= -DEADZONE && reportBuffer.ry <= DEADZONE){
		reportBuffer.ry = 0;
	}


	if (calibrated){

		if (reportBuffer.x !=0){
			//clamp variable
			reportBuffer.x = clamp(reportBuffer.x,minx,maxx);
			//convert to range
			if (reportBuffer.x > 0){
				reportBuffer.x = (char) (map(reportBuffer.x, DEADZONE, maxx, 1, 127));
			}else{
				reportBuffer.x = (char) (map(reportBuffer.x, minx, -DEADZONE, -127, -1));
			}
		}

		if (reportBuffer.y != 0){
			//clamp variable
			reportBuffer.y= clamp(reportBuffer.y,miny,maxy);

			//convert to range
			if (reportBuffer.y > 0){
				reportBuffer.y = (char) (map(reportBuffer.y, DEADZONE, maxy, 1, 127));
			}else{
				reportBuffer.y = (char) (map(reportBuffer.y, miny, -DEADZONE, -127, -1));
			}
		}

		if (reportBuffer.rx != 0){
			//clamp variable
			reportBuffer.rx = clamp(reportBuffer.rx,minrx,maxrx);

			//convert to range
			if (reportBuffer.rx > 0){
				reportBuffer.rx = (char) (map(reportBuffer.rx, DEADZONE, maxrx, 1, 127));
			}else{
				reportBuffer.rx = (char) (map(reportBuffer.rx, minrx, -DEADZONE, -127, -1));
			}
		}

		if (reportBuffer.ry != 0){
			//clamp variable
			reportBuffer.ry = clamp(reportBuffer.ry,minry,maxry);

			//convert to range
			if (reportBuffer.ry > 0){
				reportBuffer.ry = (char) (map(reportBuffer.ry, DEADZONE, maxry, 1, 127));
			}else{
				reportBuffer.ry = (char) (map(reportBuffer.ry, minry, -DEADZONE, -127, -1));
			}
		}

	}
}

inline void sendData(bool askforStatus,RF24 & radio){
	reportBuffer.reportid=askforStatus?1:0;
	bool ok = radio.write( &reportBuffer, sizeof(report_t) );

	if (ok){
		//status led off
		PORTD	&= ~(1<<LED1);
	}
	else{
		//status led on
		PORTD	|= (1<<LED1);
#ifdef DEBUG
		print_string("pa error\n");
#endif
		radio.startListening();
		radio.stopListening();
		return;
	}

	radio.startListening();

	if (askforStatus){

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
			//status led on
			PORTD	|= (1<<LED1);
#ifdef DEBUG
			print_string("Failed, response timed out.\n\r");
#endif
		}
		else
		{
			uint8_t response;
			radio.read( &response, sizeof(uint8_t) );
			//status led off
			PORTD	&= ~(1<<LED1);

			if (response){
				//battery led on
#ifdef DEBUG
				print_string("led on\n");
#endif
				PORTD	|= (1<<LED2);
			}else{
				//battery led off
				PORTD	&= ~(1<<LED2);
#ifdef DEBUG
				print_string("led off\n");
#endif
			}
		}

		timer2_stop();
	}

	radio.stopListening();
}
