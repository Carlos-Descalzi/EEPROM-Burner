#include "serial.h"
#include <avr/io.h>
#include <stddef.h>
#include <string.h>

#ifdef USE_SERIAL_STDIO
static int serial_putchar(char c, FILE* stream){
	if (c == '\n'){
		serial_write('\r');
		serial_write('\n');
	} else {
		serial_write(c);
	}
	return 0;
}
FILE serial_out = FDEV_SETUP_STREAM(serial_putchar,NULL,_FDEV_SETUP_WRITE);
#endif

void serial_init(unsigned long baud_rate){
    unsigned int bittimer = (F_CPU / ( baud_rate * 16 )) - 1;
    UBRRH = (unsigned char) (bittimer >> 8);
    UBRRL = (unsigned char) bittimer;
    UCSRC = (1<< URSEL) | (1 << UCSZ0) | (1 << UCSZ1);
    UCSRB = (1 << RXEN) | (1 << TXEN);
}
char serial_read(){
	return UDR;
}
char serial_read_wait(){
	while (!(UCSRA & (1 << RXC)));
	return UDR;
}
void serial_write(char data){
	while ( !(UCSRA & (1 << UDRE)) );
	UDR = data;
}
char serial_is_data(){
	return	(UCSRA & (1 << RXC));
}

void serial_write_str(const char* str){
	for (int i=0;str[i];i++){
		serial_write(str[i]);
	}
}
