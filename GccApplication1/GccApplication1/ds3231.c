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


uint8_t ds3231_set_alarm1(const rtc_alarm_t *a) {
	if (twi_start(DS3231_ADDR_WRITE) != 0x18) return 1;
	
	twi_write(DS3231_ALARM1_ADDR); // Dirección del Alarm 1
	
	// Configurar alarma (modo hora/minuto coincidente)
	twi_write(dec2bcd(a->min) | (1 << 7));  // A1M1 = 1 (ignorar segundos)
	twi_write(dec2bcd(a->hour) | (1 << 7)); // A1M2 = 1 (ignorar fecha/día)
	twi_write(1 << 7);                      // A1M3 = 1 (ignorar día/fecha)
	twi_write(0x80);                        // A1M4 = 1 (siempre activo)
	
	twi_stop();
	
	// Habilitar interrupción de alarma
	if (twi_start(DS3231_ADDR_WRITE) != 0x18) return 1;
	twi_write(DS3231_CONTROL_ADDR);
	twi_write(0x05); // Habilitar Alarm 1, convertir temp cada 64s
	twi_stop();
	
	return 0;
}

uint8_t ds3231_check_alarm(void) {
	if (twi_start(DS3231_ADDR_WRITE) != 0x18) return 0;
	twi_write(DS3231_STATUS_ADDR);
	twi_stop();
	
	if (twi_start(DS3231_ADDR_READ) != 0x40) return 0;
	uint8_t status = twi_read(0); // Leer con NACK
	twi_stop();
	
	return (status & 0x01); // Retorna 1 si la alarma 1 ha ocurrido
}

void ds3231_clear_alarm(void) {
	if (twi_start(DS3231_ADDR_WRITE) != 0x18) return;
	twi_write(DS3231_STATUS_ADDR);
	twi_write(0x00); // Limpiar flags de alarma
	twi_stop();
}
