BOARD_TAG    = uno
ARDUINO_LIBS = 

include ../../Arduino.mk

upload:
	avrdude -p atmega328p -c arduino -P /dev/arduino -Uflash:w:build-uno/stepper.hex

