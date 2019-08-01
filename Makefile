CC=avr-gcc
LD=avr-gcc
MCU=atmega32
F_CPU=12000000L
OPTLEVEL=1
#CFLAGS=-mmcu=$(MCU) -std=gnu99 -O$(OPTLEVEL) -DF_CPU=$(F_CPU) -Wall -DUSE_SERIAL_STDIO -DDEBUGMSG
CFLAGS=-mmcu=$(MCU) -std=gnu99 -O$(OPTLEVEL) -DF_CPU=$(F_CPU) -Wall 
LDFLAGS=-mmcu=$(MCU) 
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
FORMAT=ihex
OBJS=main.o serial.o

all: flash lst

flash: flash.hex

lst: flash.lst

clean:
	rm -rf *.hex *.elf *.o *.lst

%.hex: %.elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

flash.elf: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o flash.elf

.o: .c
	$(CC) $(CFLAGS) -c $<

burn: flash
	avrdude -V -c usbasp -p $(MCU) -U flash:w:flash.hex 
fuses: flash
	avrdude -V -c usbasp -p $(MCU) -U lfuse:w:0xff:m -U hfuse:w:0xdf:m

