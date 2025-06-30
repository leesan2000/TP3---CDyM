#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>


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
	SET_ALARM,
	ALARM_NOTIFY
} state_t;

static state_t state = IDLE;
static char    cmd_buf[BUFFER_SIZE]; //buffer para comando
static uint8_t cmd_len = 0; 
static rtc_alarm_t alarm = {0, 0, 0};
static uint8_t alarm_triggered = 0;
static uint8_t alarm_count = 0;
uint8_t last_sec = 0xFF;  // valor inválido inicial


static uint8_t rx_line_ready(void) {
	while (uart_available()) {
		char c = uart_read();
		
		 char echo_buf[2] = { c, '\0' };
		 uart_write(echo_buf);;

		//borrado de caracter 
		if (c == '\b' || c == 0x7F) {
			if (cmd_len > 0) {
				cmd_len--;
			}
			continue;
		}

		//fin linea
		if (c == '\r' || c == '\n') {
			cmd_buf[cmd_len] = '\0';
			cmd_len = 0;
			uart_write("\r\n");
			return 1;
		}

		
		if (cmd_len < BUFFER_SIZE - 1 && isprint((unsigned char)c)) {
			cmd_buf[cmd_len++] = c;
		}
		
	}
	return 0;
}


void show_current_time() {
	rtc_time_t now;
	if (ds3231_read_time(&now) == 0) { //imprime cada vez que cambia 1 segundo
		if (now.sec != last_sec) {
			last_sec = now.sec;
			char out[52];
			sprintf(out,"\rFECHA: %02u/%02u/%02u HORA: %02u:%02u:%02u\r",now.date, now.month, now.year,now.hour, now.min,  now.sec);
			uart_write(out);
		}
	
		

	}
}

static uint8_t handle_set_time(const char *p) {
	if (strlen(p) == 17
	&& p[2]=='/' && p[5]=='/' && p[8]==' '
	&& p[11]==':' && p[14]==':'
	&& isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1])
	&& isdigit((unsigned char)p[3]) && isdigit((unsigned char)p[4])
	&& isdigit((unsigned char)p[6]) && isdigit((unsigned char)p[7])
	&& isdigit((unsigned char)p[9]) && isdigit((unsigned char)p[10])
	&& isdigit((unsigned char)p[12]) && isdigit((unsigned char)p[13])
	&& isdigit((unsigned char)p[15]) && isdigit((unsigned char)p[16])
	)
	{
		rtc_time_t t;
		t.date  = (p[0]-'0')*10 + (p[1]-'0');
		t.month = (p[3]-'0')*10 + (p[4]-'0');
		t.year  = (p[6]-'0')*10 + (p[7]-'0');
		t.hour  = (p[9]-'0')*10 + (p[10]-'0');
		t.min   = (p[12]-'0')*10 + (p[13]-'0');
		t.sec   = (p[15]-'0')*10 + (p[16]-'0');
		t.day   = 1;

		if (ds3231_set_time(&t) == 0) {
			uart_write("\r\nOK\r\n");
			return 1; 
		} else {
			uart_write("\r\nERROR I2C\r\n");
		}
	} else {
		uart_write("\r\nFORMATO ERRONEO\r\n");
	}
	return 0;  
}


void show_menu(){

		uart_write("\nCOMANDOS:\r\n");
		uart_write("ON\r\n");
		uart_write("OFF\r\n");
		uart_write("SET TIME DD/MM/YY HH:MM:SS\r\n");
		uart_write("SET ALARM HH:MM\r\n");
		
	
}
int main(void) {
	DDRD |= (1<<PD1);// PD1 = TX out
	DDRC  &= ~((1<<PC4)|(1<<PC5));
	PORTC |=  (1<<PC4)|(1<<PC5);
	
	uart_init_int();
	twi_init();
	sei();
	
	//menu

	
	state_t prev_state = (state_t)-1;
	state_t act_state;
	rtc_time_t now;

	while (1) {
			
		//verificacion de alarma por polling		
		if (alarm.enabled && ds3231_check_alarm()) {
			alarm_triggered = 1; //activo flag
			ds3231_clear_alarm(); //limpio flag
		}
		if(alarm.enabled && !alarm_triggered && ds3231_read_time(&now) == 0) {
			if(now.hour == alarm.hour && now.min == alarm.min && now.sec == 0) {
				// Solo activar al inicio del minuto (segundos == 0)
				alarm_triggered = 1;
				alarm_count = 0;
				state = ALARM_NOTIFY;
				ds3231_clear_alarm();
			}
		}

		//comandos	
		if (rx_line_ready()) {
			if (strcasecmp(cmd_buf, "ON") == 0) {
				state = ON;
				uart_write("HORA ON\r\n");
				
					
			}
			else if (strcasecmp(cmd_buf, "OFF") == 0) {
				state = IDLE;
				uart_write("HORA OFF\r\n");
			}
			else if ( (strncmp(cmd_buf, "SET TIME ", 9) == 0) || (strncmp(cmd_buf, "set time ", 9) == 0) )  {
				//uart_write("DEBUG: ");
				//uart_write(cmd_buf);
				//uart_write("\"\r\n");
				uart_write(cmd_buf);
				state = SET_TIME;
			}
			else if ( (strncmp(cmd_buf, "SET ALARM ", 10) == 0) || (strncmp(cmd_buf, "set alarm ", 10) == 0)        ) {
				state = SET_ALARM;
				//uart_write("DEBUG: ");
				//uart_write(cmd_buf);
				//uart_write("\"\r\n");
			}else{
				uart_write("COMANDO ERRONEO\r\n ");
				//uart_write("DEBUG: ");
				//uart_write(cmd_buf);
				//uart_write("\"\r\n");
				show_menu();
			}
			
			cmd_buf[0] = '\0';
		}
		
		
		if (state != prev_state) {
			if (state == IDLE) {
				show_menu(); //muestra el menu solamente si IDLE no fue el estado anterior (lo muestra 1 vez)
			}
			prev_state = state;
		}
		
		//estados	
		switch (state) {
			
			case IDLE:
				act_state = IDLE;
				//estado en el que esta el mcu cuando no se esta realiazndo ninguna accion
			break;

			case ON: //muestra fecha y hora
			{
				act_state = ON;
				show_current_time();
			}
			break;

			case SET_TIME: 
			{
				if (handle_set_time(cmd_buf + 9) == 1){
					state = act_state;
				}else{
					state = IDLE;
				}
			} 
			break;
		
			case SET_ALARM: 
			{
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
						
						//confirmacion de alarma
						char msg[32];
						sprintf(msg, "\r\nAlarma configurada para %02d:%02d\r\n\n",
						alarm.hour, alarm.min);
						uart_write(msg);
					} else {
						uart_write("ERROR I2C\r\n");
					}
				} else {
					uart_write("FORMATO ERRONEO. USAR HH:MM\r\n");
				}
			state = act_state;
			}
		}
		break;
		
		case ALARM_NOTIFY:
		{
			if(ds3231_read_time(&now) == 0){
				if(now.sec != last_sec){
					last_sec = now.sec;
					uart_write("\r\nALARMA!\r\n");
					alarm_count++;
					if(alarm_count >= 5){
						state = act_state;
					}
				}
			}
		}
		break;
	} 
	 _delay_ms(10);
   }
}
