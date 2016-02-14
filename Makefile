MCU=atmega64
CC=avr-gcc
OBJCOPY=avr-objcopy
PROJECT=main
CFLAGS=-mmcu=$(MCU) -O3 -Wall
OBJS=display.o main.o adc.o output.o output_midi.o output_normal.o input.o menu.o

all: clean $(PROJECT).hex


$(PROJECT).hex : $(PROJECT).out 
	$(OBJCOPY) -R .eeprom -O ihex $(PROJECT).out $(PROJECT).hex 

$(PROJECT).out : $(OBJS)
	$(CC) $(CFLAGS) -o $(PROJECT).out -Wl,-Map,$(PROJECT).map $(OBJS)

%.o : %.c %.h
	$(CC) $(CFLAGS) -c $<

asm : $(PROJECT).c 
	$(CC) $(CFLAGS) -S $(PROJECT).c
load: $(PROJECT).hex
	avrdude -c usbtiny -p m64 -U flash:w:$(PROJECT).hex
reset:
	avrdude -c usbtiny -p m64
clean:
	rm -f *.o *.map *.out *.hex
