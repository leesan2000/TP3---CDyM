#include "i2c.h"
#include "ds3231.h"

uint8_t bcd2dec(uint8_t b){
	uint8_t decena = (b >> 4) & 0x0F;
	uint8_t unidad = b & 0x0F;
	uint8_t res = decena * 10 + unidad; 
	return res;
}
uint8_t dec2bcd(uint8_t d){
	uint8_t decena = d/10;
	uint8_t unidad = d % 10;
	uint8_t res = (decena << 4) | unidad;
	return res; 
	 
}

uint8_t ds3231_read_time(rtc_time_t *t)
{
	if (twi_start(DS3231_ADDR_WRITE) != 0x18){
		return 1;
	}
	twi_write(0x00);                              // seconds reg
	if (twi_start(DS3231_ADDR_READ)  != 0x40){ 
		return 1;
	}

	t->sec   = bcd2dec(twi_read(1));
	t->min   = bcd2dec(twi_read(1));
	t->hour  = bcd2dec(twi_read(1));
	t->day   = bcd2dec(twi_read(1));
	t->date  = bcd2dec(twi_read(1));
	t->month = bcd2dec(twi_read(1));
	t->year  = bcd2dec(twi_read(0));              // NACK último byte
	twi_stop();
	return 0;
}

uint8_t ds3231_set_time(const rtc_time_t *t)
{
	if (twi_start(DS3231_ADDR_WRITE) != 0x18){ 
		return 1;
	}
	twi_write(0x00);          // seconds reg
	twi_write(dec2bcd(t->sec));
	twi_write(dec2bcd(t->min));
	twi_write(dec2bcd(t->hour));
	twi_write(dec2bcd(t->day));
	twi_write(dec2bcd(t->date));
	twi_write(dec2bcd(t->month));
	twi_write(dec2bcd(t->year));
	twi_stop();
	return 0;
}
