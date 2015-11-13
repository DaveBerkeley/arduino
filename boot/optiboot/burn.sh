#!/bin/bash

DUDE_ARGS='-p atmega328p -c stk500v1 -P /dev/ttyUSB1 -b 19200'

avrdude $DUDE_ARGS -v

# FIN
