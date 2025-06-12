#ifndef UART_H_
#define UART_H_

#include <stdint.h>

void uart_init_int(void);
void uart_write(const char *s);//Copia a la cola TX
uint8_t uart_available(void);//Caracteres esperando en cola RX 
char uart_read(void);//POP de un character de la cola RX

#endif
