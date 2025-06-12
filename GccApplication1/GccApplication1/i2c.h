#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

void    twi_init(void);
uint8_t twi_start(uint8_t addr_rw); 
void    twi_write(uint8_t data);
uint8_t twi_read(uint8_t ack);
void    twi_stop(void);

#endif
