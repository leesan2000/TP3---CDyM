#ifndef DS3231_H_
#define DS3231_H_
#define DS3231_ALARM1_ADDR 0x07
#define DS3231_CONTROL_ADDR 0x0E
#define DS3231_STATUS_ADDR 0x0F
#include <stdint.h>

// 7bits <<1 para R/W
#define DS3231_SLA    0x68
#define DS3231_ADDR_WRITE  ((DS3231_SLA<<1) | 0)   // 0xD0
#define DS3231_ADDR_READ   ((DS3231_SLA<<1) | 1)   // 0xD1

//BCD - decimal
uint8_t bcd2dec(uint8_t b);
uint8_t dec2bcd(uint8_t d);

//fecha y hora
typedef struct {
	uint8_t sec, min, hour;
	uint8_t day, date, month, year;   // day = 1…7 (lunes = 1)
} rtc_time_t;

//alarma (fecha)
typedef struct {
	uint8_t hour;
	uint8_t min;
	uint8_t enabled;
} rtc_alarm_t;

//func auxiliares
uint8_t ds3231_read_time(rtc_time_t *t);
uint8_t ds3231_set_time(const rtc_time_t *t);
uint8_t ds3231_check_alarm(void);
void ds3231_clear_alarm(void);



#endif 