#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>

#include "i2c.h"
#include "config.h"
#include "uart.h"
#include "ds3231.h"

#define BUFFER_SIZE 32

typedef enum {
	IDLE,       
	ON,    
	SET_TIME    
} state_t;

static state_t state = IDLE;
static char    cmd_buf[BUFFER_SIZE]; //buffer para comando
static uint8_t cmd_len = 0; 

//Acumula bytes hasta CR/LF --> devuelve 1 cuando termino la linea en el bufferASAS
static uint8_t rx_line_ready(void) {
	while (uart_available()) { //mientras haya datos en el buffer
		char c = uart_read(); //lee caracter
		if (c=='\r' || c=='\n') { //verifica si es fin de linea
			cmd_buf[cmd_len] = '\0';
			cmd_len = 0;
			return 1;
		}
		if (cmd_len < BUFFER_SIZE-1) { //verifica que no este lleno
			cmd_buf[cmd_len] = c;
			cmd_len++;
		}
	}
	return 0;
}

int main(void) {
	DDRD |= (1<<PD1);// PD1 = TX out
	uart_init_int();
	twi_init();
	
	
	uart_write("\r\n*** RTC ALARM CLOCK ***\r\n");
	uart_write("ON\r\nOFF\r\nSET TIME\r\n");
	
	rtc_time_t now;
	uint16_t   ms_count = 0;

	while (1) {
		//Menu de opciones
		if (rx_line_ready()) {
			if      (strcasecmp(cmd_buf, "ON") == 0)        state = ON;
			else if (strcasecmp(cmd_buf, "OFF") == 0)       state = IDLE;
			else if (strncmp(cmd_buf, "SET TIME ", 9) == 0) state = SET_TIME;
			cmd_buf[0] = '\0';
			ms_count = 0;
		}

		switch (state) {
			
			case IDLE:
			_delay_ms(10);
			break;

			case ON:
			_delay_ms(10);
			if ((ms_count += 10) >= 1000) {
				ms_count = 0;
				if (!ds3231_read_time(&now)) {
					char out[48];
					sprintf(out,"\rFECHA:%02u/%02u/%02u HORA:%02u:%02u:%02u    ",now.date, now.month, now.year,now.hour, now.min,  now.sec);uart_write(out);
				}
			}
			break;

			case SET_TIME:
			{
				rtc_time_t t;
				if (sscanf(cmd_buf+9, "%hhu/%hhu/%hhu %hhu:%hhu:%hhu",&t.date, &t.month, &t.year,&t.hour, &t.min,  &t.sec) == 6){
					t.day = 1;
					if (!ds3231_set_time(&t)){ 
						uart_write("OK\r\n");
					}else{                      
						uart_write("ERROR I2C\r\n");
					}
				} else {
					uart_write("FORMATO ERRONEO\r\n");
				}
				state = ON;
			}
			break;
		}
	}
}
