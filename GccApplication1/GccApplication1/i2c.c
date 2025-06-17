#include <avr/io.h>
#include <stdint.h>
#include "config.h"
#include "i2c.h"
#include "uart.h"

#define TWI_BITRATE  ((F_CPU/TWI_FREQ - 16UL)/2UL)


void twi_init(void)
{
	TWSR = 0;                                    // prescaler = 1
	TWBR = (uint8_t)(((F_CPU / 100000UL) - 16) / 2);   // 100 kHz
	TWCR = _BV(TWEN);                            // *** enable TWI ***
}

#define TWI_TIMEOUT_CNT 30000

static uint8_t twi_wait(void) {
    uint16_t cnt = 0;
    while (!(TWCR & (1<<TWINT))) {
        if (++cnt >= TWI_TIMEOUT_CNT) {
            uart_write("TWI TIMEOUT\r\n");
            return 1;
        }
    }
    return 0;
}

uint8_t twi_start(uint8_t addr_rw)
{
	// Generate START
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
	if (twi_wait()) return 0xFF;

	uint8_t st = TWSR & 0xF8;           // 0x08 = START, 0x10 = REP-START
	if (st != 0x08 && st != 0x10) {
		return st;                      // bus error – don’t continue
	}

	// Send address + R/W
	TWDR = addr_rw;
	TWCR = _BV(TWINT) | _BV(TWEN);
	if (twi_wait()) return 0xFF;

	return (TWSR & 0xF8);               // 0x18 = SLA+W ACK, 0x40 = SLA+R ACK
}

void twi_stop(void) {
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	// No esperamos TWINT aquí
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
