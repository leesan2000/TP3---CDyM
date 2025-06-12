#include <avr/io.h>
#include <stdint.h>
#include "config.h"
#include "i2c.h"

#define TWI_BITRATE ((F_CPU/TWI_FREQ - 16)/2)// --> SCL freq = F_CPU / (16 + (2xTWBRxPrescaler) )
											//TWI freq = F_CPU / ( 16 + (2xTWBR) )
											//despejando --> TWBR = ( (F_CPU/TWI freq) - 16 ) / 2

void twi_init(void)
{
	TWSR = 0; // prescaler 1
	TWBR = (uint8_t)TWI_BITRATE;
}

uint8_t twi_start(uint8_t addr_rw)
{
	TWCR = (1<<TWINT) //interrupt flag 1
	|(1<<TWSTA)//start condition 1
	|(1<<TWEN); //twi enable 1
	while(!(TWCR & (1<<TWINT))); //espera a que TWINT = 1
	TWDR = addr_rw; //se carga la direccion + bit r/w
	
	TWCR = (1<<TWINT)
	|(1<<TWEN); 
	while(!(TWCR & (1<<TWINT))); //espera que se procese byte y ack
	return (TWSR & 0xF8); //lee el registro de estado y devuelve los bits de datos (primeros 5)
}

void twi_stop(void)
{
	TWCR = (1<<TWINT)
	|(1<<TWEN)
	|(1<<TWSTO); //pone el bit de STOP en 1
}

void twi_write(uint8_t data)
{
	TWDR = data;//se carga el dato en el registro
	TWCR = (1<<TWINT)|(1<<TWEN);//seteamos el reg de control
	while(!(TWCR & (1<<TWINT)));//esperamos a que termine
}

uint8_t twi_read(uint8_t ack)
{
	uint8_t control_reg = (1<<TWINT) | (1<<TWEN);
	if(ack){ //si ack = 1 ponemos en 1 el bit de ack (TWEA)
		control_reg |= (1<<TWEA);
	}
	TWCR = control_reg;
	while(!(TWCR & (1<<TWINT))); //esperamos que termine la lectura
	return TWDR; //devuelve el dato leido
}
