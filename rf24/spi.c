/*
 * spi.c
 *
 *  Created on: Apr 21, 2013
 *      Author: bgouveia
 */

#include "spi.h"

#define CS_PIN_PORT PORTB
#define CS_PIN PB1

#define CE_PIN_PORT PORTB
#define CE_PIN PB2

#define SCK PB5
#define MOSI PB3

  // When the SS pin is set as OUTPUT, it can be used as
  // a general purpose output port (it doesn't influence
  // SPI operations).
  //pinMode(SS, OUTPUT);


void spi_init(unsigned long bitrate,unsigned long datawidth){

	spi_cs_high();
	DDRB |=  (1<<PB2);


	// Enable SPI, Master, set clock rate fck/2 (maximum)
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR = (1<<SPI2X);

	DDRB 	|=  (1<<PB1) | (1<<SCK) | (1<<MOSI); // PB1(CE) ,  SCK and MOSI as output
}

void spi_cs_low()
{
	CS_PIN_PORT &= ~(1<<CS_PIN);
}

void spi_cs_high()
{
	CS_PIN_PORT |= (1<<CS_PIN);
}

void spi_ce_low()
{
	CE_PIN_PORT &= ~(1<<CE_PIN);
}

void spi_ce_high()
{
	CE_PIN_PORT |= (1<<CE_PIN);
}


uint8_t spi_transferByte(uint8_t _data){

	  SPDR = _data;
	  while (!(SPSR & _BV(SPIF)))
	    ;
	  return SPDR;
}
