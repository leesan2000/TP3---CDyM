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
#define ALARM_REPEAT 5

//Definimos estados
typedef enum {
	IDLE,       
	ON,    
	SET_TIME,  
	SET_ALARM  
} state_t;

static state_t state = IDLE;
static char    cmd_buf[BUFFER_SIZE]; //buffer para comando
static uint8_t cmd_len = 0; 
static rtc_alarm_t alarm = {0, 0, 0};
static uint8_t alarm_triggered = 0;
static uint8_t alarm_count = 0;
static uint8_t last_minute = 0xFF;

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

void show_current_time() {
	rtc_time_t now;
	if (!ds3231_read_time(&now)) {
		char out[48];
		sprintf(out, "\rFECHA:%02u/%02u/%02u HORA:%02u:%02u:%02u    ",
		now.date, now.month, now.year, now.hour, now.min, now.sec);
		uart_write(out);
		
		// Verificación de alarma basada en comparación de tiempo
		if (alarm.enabled && !alarm_triggered &&
		now.hour == alarm.hour && now.min == alarm.min &&
		now.min != last_minute) {
			alarm_triggered = 1;
			alarm_count = 0;
			uart_write("\r\nALARMA ACTIVADA!\r\n");
		}
		last_minute = now.min;
	}
}
int main(void) {
	DDRD |= (1<<PD1);// PD1 = TX out
	uart_init_int();
	twi_init();
	sei();
	
	// Mensaje de bienvenida
	uart_write("\r\n*** RTC ALARM CLOCK ***\r\n");
	uart_write("COMANDOS:\r\n");
	uart_write("ON - Mostrar hora\r\n");
	uart_write("OFF - Ocultar hora\r\n");
	uart_write("SET TIME DD/MM/YY HH:MM:SS\r\n");
	uart_write("SET ALARM HH:MM\r\n");
	uart_write("ALARM OFF - Desactivar alarma\r\n");
	

	uint16_t   ms_count = 0;

	while (1) {
		
		//Verificar alarma		
		rtc_time_t now;
		if (alarm.enabled && ds3231_check_alarm()) {
			alarm_triggered = 1; //activo flag
			ds3231_clear_alarm(); //limpio flag
		}
		if(alarm.enabled && !alarm_triggered && ds3231_read_time(&now) == 0) {
			if(now.hour == alarm.hour && now.min == alarm.min && now.sec == 0) {
				// Solo activar al inicio del minuto (segundos == 0)
				alarm_triggered = 1;
				alarm_count = 0;
				uart_write("\r\n¡ALARMA ACTIVADA!\r\n");
				ds3231_clear_alarm();
			}
		}

		//Procesamiento de comandos	
		if (rx_line_ready()) {
			if (strcasecmp(cmd_buf, "ON") == 0) {
				state = ON;
				uart_write("Visualizacion activada\r\n");
			}
			else if (strcasecmp(cmd_buf, "OFF") == 0) {
				state = IDLE;
				uart_write("Visualizacion desactivada\r\n");
			}
			else if (strncmp(cmd_buf, "SET TIME ", 9) == 0) {
				state = SET_TIME;
			}
			else if (strncmp(cmd_buf, "SET ALARM ", 10) == 0) {
				state = SET_ALARM;
			}
			else if (strcasecmp(cmd_buf, "ALARM OFF") == 0) {
				alarm.enabled = 0;
				alarm_triggered = 0;
				uart_write("Alarma desactivada\r\n");
			}
			
			cmd_buf[0] = '\0';
			ms_count = 0;
		}
		
		//Menu de opciones	
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
		
		 case SET_ALARM: {
			 if (sscanf(cmd_buf+10, "%hhu:%hhu", &alarm.hour, &alarm.min) == 2) {
				 // Configurar alarma para que coincida con hora y minuto específicos
				 uint8_t alarm_settings[4] = {
					 dec2bcd(alarm.min),       // Minutos (A1M1=0 - comparar minutos)
					 dec2bcd(alarm.hour),       // Horas (A1M2=0 - comparar horas)
					 0x80,                      // Día/Fecha (A1M3=1 - ignorar)
					 0x80                       // A1M4=1 (siempre activo)
				 };
				
				// 3. Escribir configuración de alarma
				if(twi_start(DS3231_ADDR_WRITE) == 0x18) {
					twi_write(0x07);  // Registro de alarma 1
					for(uint8_t i=0; i<4; i++) {
						twi_write(alarm_settings[i]);
					}
					twi_stop();
					
					// 4. Habilitar interrupción de alarma
					if(twi_start(DS3231_ADDR_WRITE) == 0x18) {
						twi_write(0x0E);  // Registro de control
						twi_write(0x05);  // Habilitar Alarma 1
						twi_stop();
						
						alarm.enabled = 1;
						alarm_triggered = 0;
						alarm_count = 0;
						
						// Mostrar confirmación
						char msg[32];
						sprintf(msg, "Alarma configurada para %02d:%02d\r\n",
						alarm.hour, alarm.min);
						uart_write(msg);
					}
					} else {
					uart_write("Error de comunicación I2C\r\n");
				}
				} else {
				uart_write("Formato incorrecto. Use HH:MM\r\n");
			}
			state = ON;
		}
		 break;
	  
	}
	// Manejo de la notificación de alarma
	    if (alarm_triggered && alarm_count < 5) {
		    if ((ms_count % 1000) == 0) { // Cada segundo
			    uart_write("ALARMA!\r\n");
			    alarm_count++;
			    if (alarm_count == 5) {
				    alarm_triggered = 0;
			    }
		    }
	    }
	   
	 _delay_ms(10);
	 ms_count += 10;
   }
  //return 0;
}
