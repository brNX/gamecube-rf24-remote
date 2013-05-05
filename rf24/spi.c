/*
 * spi.c
 *
 *  Created on: Apr 21, 2013
 *      Author: bgouveia
 */

#include "spi.h"

#define CS_PIN_BASE GPIO_PORTA_BASE
#define CS_PIN GPIO_PIN_3

#define CE_PIN_BASE GPIO_PORTC_BASE
#define CE_PIN GPIO_PIN_5


void spi_init(unsigned long bitrate,unsigned long datawidth){

}

void spi_cs_low()
{


}

void spi_cs_high()
{

}

void spi_ce_low()
{


}

void spi_ce_high()
{

}


uint8_t spi_transferByte(uint8_t data){

	return (0xFF);
}
