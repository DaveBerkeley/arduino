#!/bin/bash

DUDE_ARGS='-p atmega328p -c stk500v1 -P /dev/ttyUSB1 -b 19200'

#avrdude $DUDE_ARGS -v

# original settings
#avrdude $DUDE_ARGS -e -Ulock:w:0x3F:m -Uefuse:w:0x05:m -Uhfuse:w:0xde:m -Ulfuse:w:0xff:m
# for larger boot section
avrdude $DUDE_ARGS -e -Ulock:w:0x3F:m -Uefuse:w:0x05:m -Uhfuse:w:0xd8:m -Ulfuse:w:0xff:m

avrdude $DUDE_ARGS -Uflash:w:optiboot_atmega328.hex:i -Ulock:w:0x0F:m

# FIN
