#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "uart.h"

#define BUFSZ  64 //tamaño de buffer
static volatile char rx_buf[BUFSZ]; //buffer recepcion
static volatile uint8_t rx_ini, rx_fin;

static volatile char tx_buf[BUFSZ]; //bufer transmision
static volatile uint8_t tx_ini, tx_fin;

static inline uint8_t buf_inc(uint8_t p) {
	uint8_t sig = p + 1; //suma 1 al indice
	if(sig >= BUFSZ){ //si el buffer se lleno se vuelve al inicio
		sig = 0;
	}
	return sig; 
}

void uart_init_int(void)
{
	UBRR0H = (uint8_t)((F_CPU/16/BAUD - 1) >> 8);
	UBRR0L = (uint8_t)((F_CPU/16/BAUD - 1));
	UCSR0B = (1<<RXEN0)//habilito receptor
	| (1<<TXEN0)//habilito transmisor
	| (1<<RXCIE0);
	UCSR0C = (1<<UCSZ01) 
	| (1<<UCSZ00);//formato --> 8 bits - sin bit de paridad - 1 stop bit
	sei();
}

ISR(USART_RX_vect)
{
	uint8_t sig = buf_inc(rx_ini);
	rx_buf[rx_ini] = UDR0;
	if (sig != rx_fin){ 
		rx_ini = sig; 
	}   
}

ISR(USART_UDRE_vect)
{
	if (tx_ini == tx_fin) {//cola vacia--> paro interrupcion
		UCSR0B &= ~(1<<UDRIE0);
	} else {
		UDR0 = tx_buf[tx_fin];
		tx_fin = buf_inc(tx_fin);
	}
}

void uart_write(const char *s)
{
	while (*s) {
		uint8_t nxt = buf_inc(tx_ini);
		while (nxt == tx_fin) ;         /* wait if queue full           */
		tx_buf[tx_ini] = *s++;
		tx_ini = nxt;
	}
	UCSR0B |= (1<<UDRIE0);               /* kick TX ISR                  */
}

uint8_t uart_available(void) { return (BUFSZ + rx_ini - rx_fin) & (BUFSZ-1); }

char uart_read(void)
{
	char c = 0;
	if (rx_ini != rx_fin) {
		c = rx_buf[rx_fin];
		rx_fin = buf_inc(rx_fin);
	}
	return c;
}
