#!/usr/bin/python -u

import time

import serial

s = serial.Serial("/dev/nano", 9600)

while True:
    r = s.readline()
    if not r:
        time.sleep(1)
        continue
    print r,

# FIN
