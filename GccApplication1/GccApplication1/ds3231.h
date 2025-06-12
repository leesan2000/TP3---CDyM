#ifndef DS3231_H_
#define DS3231_H_

#include <stdint.h>

// 7bits <<1 para R/W
#define DS3231_ADDR_WRITE 0xD0
#define DS3231_ADDR_READ  0xD1

//BCD - decimal
uint8_t bcd2dec(uint8_t b);
uint8_t dec2bcd(uint8_t d);

//fecha y hora
typedef struct {
	uint8_t sec, min, hour;
	uint8_t day, date, month, year;   // day = 1…7 (lunes = 1)
} rtc_time_t;

//func auxiliares
uint8_t ds3231_read_time(rtc_time_t *t);
uint8_t ds3231_set_time(const rtc_time_t *t);

#endif 