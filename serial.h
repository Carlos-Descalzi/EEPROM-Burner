#ifndef _SERIAL_H_
#define _SERIAL_H_

#ifdef USE_SERIAL_STDIO
#include <stdio.h>
extern FILE serial_out;
#endif

void serial_init		(unsigned long baud_rate);
void serial_write		(char data);
char serial_read		(void);
char serial_read_wait	(void);
char serial_is_data		(void);
void serial_write_str	(const char* str);

#endif
