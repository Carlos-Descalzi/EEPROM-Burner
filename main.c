#include <avr/io.h>
#include <util/delay.h>
#include "serial.h"

/* PINOUT

	VPP			PB0 +-------+ PA0		D0
	CE			PB1 +       + PA1		D1
	PGM			PB2 +       + PA2		D2
	OE			PB3 +       + PA3		D3
					PB4 +       + PA4		D4
					PB5 +       + PA5		D5
					PB6 +       + PA6		D6
					PB7 +       + PA7		D7
					RST +       + AREF
					VCC +       + GND
					GND +       + AVCC
					XL2 +       + PC7		ADDRL7
					XL1 +       + PC6		ADDRL6
			RX	PD0 +       + PC5		ADDRL5
			TX	PD1 +       + PC4		ADDRL4
	ADDRH0	PD2 +       + PC3		ADDRL3
	ADDRH1	PD3 +       + PC2		ADDRL2
	ADDRH2	PD4 +       + PC1		ADDRL1
	ADDRH3	PD5 +       + PC0		ADDRL0
	ADDRH4	PD6 +-------+ PD7		ADDRH5
*/

#define ADDRL_D	DDRC
#define ADDRH_D	DDRD
#define DATA_D	DDRA
#define CTRL_D	DDRB

#define ADDRL			PORTC
#define ADDRH			PORTD
#define DATA_OUT	PORTA
#define CTRL			PORTB

#define DATA_IN		PINA

#define PIN_VPP	0x01		// PB0
#define PIN_CE	0x02		// PB1
#define PIN_PGM	0x04		// PB2
#define PIN_OE	0x08		// PB3


static uint16_t address;
static uint16_t length;
static uint16_t counter;

#define BUFFERSIZE	512

static uint8_t buffer[BUFFERSIZE];

#define chip_enable(){\
	CTRL &= ~PIN_CE;\
}

#define chip_disable(){\
	CTRL |= PIN_CE;\
}

static void wr_strobe(){
	CTRL &= ~PIN_PGM;
	_delay_us(95);		// By specification do not change !
	CTRL |= PIN_PGM;
}

static void set_write_mode(){
	CTRL |= PIN_VPP;
	DATA_D = 0xFF;
}

static void set_read_mode(){
	CTRL &= ~PIN_VPP;
	DATA_OUT = 0x00;
	DATA_D = 0x00;
	DATA_OUT = 0xFF;
}

static void set_address(uint16_t addr){
	uint8_t addrl = addr & 0xFF;
	uint8_t addrh = addr >> 8;
	ADDRL = addrl;
	ADDRH = addrh << 2;
}

static void init_state(){
	SFIOR &= ~(1 << PUD);
	CTRL |= (PIN_CE | PIN_PGM | PIN_OE);
	set_address(0x0000);
	address = 0;
	counter = 0;
}

uint8_t do_write(uint16_t write_address,uint8_t data){
	uint8_t vdata = ~data;
	uint8_t try_count;
	
	set_address(write_address);

	_delay_us(10);

	for (try_count = 0;try_count < 20 && data != vdata;try_count++){
		DATA_OUT = data;

		_delay_us(10);
		
		wr_strobe();
		
		DATA_D = 0x00;
		DATA_OUT = 0xFF;
		
		CTRL &= ~PIN_OE;
		
		_delay_us(1);
		
		vdata = DATA_IN;
		
		CTRL |= PIN_OE;	
		
		//_delay_us(10);
		
		DATA_OUT = 0x00;
		DATA_D = 0xFF;
	}
	return vdata;
}

uint8_t do_read(){
	uint8_t data;
	uint16_t real_address = address + counter;
	set_address(real_address);

	CTRL &= ~PIN_OE;

	_delay_us(2);

	data = DATA_IN;
	
	CTRL |= PIN_OE;
	_delay_us(2);

	return data;
}

void do_read_address(){
	address = serial_read_wait();
	address |= serial_read_wait() << 8;
	serial_write(0x01);
}

void do_read_length(){
	length = serial_read_wait();
	length |= serial_read_wait() << 8;
	serial_write(0x01);
}

#define min(a,b) (a < b ? a : b)

static void read_to_buffer(uint8_t *crc,uint16_t* bufread){
	uint16_t remain = length - counter;
	uint16_t total = min(BUFFERSIZE,remain);
	*crc = 0;
	for (uint16_t i=0;i<total;i++){
		buffer[i] = serial_read_wait();
		*crc ^= buffer[i];
	}
	*bufread = total;
}

static void do_write_buffer(uint8_t *crc,uint16_t buflen){
	*crc = 0;
	for (uint16_t i=0;i<buflen;i++){
		*crc^= do_write(address + counter + i,buffer[i]);
	}
}

void do_write_data(){
	uint8_t data;
	uint8_t crc;
	uint16_t bufread;
	serial_write(0x01);
	set_write_mode();

	chip_enable();
	CTRL |= PIN_OE;

	counter = 0;
	bufread = 0;
	while(counter < length){
		
		read_to_buffer(&crc,&bufread);
		
		serial_write(0x01);
		serial_write(crc);

		data = serial_read_wait();

		if (data == 0xFF){
			break;
		}

		do_write_buffer(&crc,bufread);

		serial_write(0x01);
		serial_write(crc);

		data = serial_read_wait();

		if (data == 0xFF){
			break;
		}
		
		counter+=bufread;
	}
	
	chip_disable();
	set_read_mode();
}

void do_read_rom(){
	uint8_t data;
	uint8_t crc;
	
	counter = 0;
	
	set_read_mode();
	chip_enable();
	
	crc = 0;
	while(counter < length){
		data = do_read();

		crc^=data;

		serial_write(data);
		counter++;
	}
	
	chip_disable();
	serial_write(crc);
	
}

void read_and_inc(){
	
	set_read_mode();
	chip_enable();
	
	uint8_t data = do_read();
	
	serial_write(data);
	
	counter++;

	chip_disable();
	
}

void do_test_1(){

	set_read_mode();
	
	set_address(0x0000);
	DATA_D = 0xFF;
	DATA_OUT = 0x00;
	//CTRL |= (PIN_CE  | PIN_PGM | PIN_OE);
	//CTRL &= ~PIN_VPP;
}


void do_test_2(){
	set_write_mode();

	set_address(0xFFFF);
	CTRL |= PIN_OE;
	DATA_D = 0xFF;
	DATA_OUT = 0xFF;
	CTRL &= ~PIN_PGM;
}

int main(){

	serial_init(9600);

	CTRL_D = 0x0F;
	DATA_D = 0xFF;
	ADDRL_D = 0xFF;
	ADDRH_D = 0xFC;
	DATA_OUT = 0x00;
	
	init_state();
	set_read_mode();
	
	while(1){
		uint8_t data = serial_read_wait();
		if (data == 's'){
			do_read_address();
		} else if (data == 'l'){
			do_read_length();
		} else if (data == 'd'){
			do_write_data();
		} else if (data == 'r'){
			do_read_rom();
		} else if (data == '1'){
			do_test_1();
		} else if (data == '2'){
			do_test_2();
		} else if (data == '0'){
			init_state();
		} else if (data == 'x'){
			// aborting whatever was running
		} else if (data == '+'){
			read_and_inc();
		}
	}
}
