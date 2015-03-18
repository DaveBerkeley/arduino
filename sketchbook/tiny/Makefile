
#CPU = attiny85
CPU = attiny84
CC = avr-g++

CFLAGS = -mmcu=$(CPU) -Wall -Os 

#ARDUINO_BASE = /home/dave/arduino/arduino-1.0
ARDUINO_BASE = /home/dave/arduino/arduino-1.0.2
JEELIB = $(ARDUINO_BASE)/libraries/JeeLib
#ARDUINO = $(ARDUINO_BASE)/hardware/arduino/cores/arduino/
TINYLIB = $(ARDUINO_BASE)/hardware/tiny/cores/tiny
#ARDLIB = $(TINYLIB)

DEFINES += ARDUINO=100
#DEFINES += F_CPU=20000000UL
DEFINES += F_CPU=10000000UL

INCLUDES = .
INCLUDES += $(JEELIB)
#INCLUDES += $(ARDUINO)
INCLUDES += $(TINYLIB)

CFLAGS += $(INCLUDES:%=-I%)
CFLAGS += $(DEFINES:%=-D%)

all: tiny.hex

clean:
	rm -f *.elf *.hex *.o *.lst *.map

ARD_OBJS = wiring_digital.o pins_arduino.o
OBJS = $(ARD_OBJS) tiny.o RF12.o 

tiny.o: tiny.cpp Makefile
	$(CC) $(CFLAGS) -c -o $@ $< 

wiring_digital.o: $(TINYLIB)/wiring_digital.c
	$(CC) $(CFLAGS) -c -o $@ $< 

pins_arduino.o: $(TINYLIB)/pins_arduino.c
	$(CC) $(CFLAGS) -c -o $@ $< 

RF12.o: $(JEELIB)/RF12.cpp
	$(CC) $(CFLAGS) -c -o $@ $< 

tiny.elf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

tiny.hex: tiny.elf
	avr-objcopy -j .text -j .data -O ihex tiny.elf tiny.hex
	avr-objdump -t -h -S tiny.elf >tiny.lst

burn: tiny.hex
	avrdude -p $(CPU) -c avrisp -P /dev/jeenode -b 19200 -u -U flash:w:tiny.hex

# FIN
