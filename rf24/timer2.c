/*
 * timer2.c
 *
 *  Created on: 5 de Mai de 2013
 *      Author: brNX
 */

#include <avr/interrupt.h>
#include <avr/io.h>

volatile unsigned int timeouttick=0;

void timer2_setup(){
	//enable interrupt TimerCounter2 compare match A
	TIMSK2 |= (1<<OCIE2A);

	//setting CTC
	TCCR2A |= (1<<WGM21);

	//top of the counters (1khz)
	OCR2A=0x7C;
}


void timer2_start(){
	TCNT2=0;
	timeouttick=0;

	//Timer2 Settings: Timer Prescaler /128,
	TCCR2B |= (1<<CS22) | (1<<CS20);
}

void timer2_stop(){

	//Timer2 Settings: Timer Prescaler /128,
	TCCR2B &= ~((1<<CS22) |(1<<CS21) |(1<<CS20));
}

unsigned int timer2_gettick(){
	return timeouttick;
}


ISR(TIMER2_COMPA_vect)
{
	timeouttick++;
}
