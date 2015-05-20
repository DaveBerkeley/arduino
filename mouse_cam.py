#!/usr/bin/python

import time

import serial

import cv, cv2
import numpy
import scipy.misc

#
#

def log(*args):
    print time.strftime("%y/%m/%d %H:%M:%S :"), 
    for arg in args:
        print arg,
    print

#
#

def init_serial():
    log("open serial '%s'" % serial_dev)
    s = serial.Serial(serial_dev, baudrate=19200, timeout=1, rtscts=True)

    log("serial opened")
    return s

#
#

def getkey():
    return cv2.waitKey(1) & 0xFF

if __name__ == "__main__":
    serial_dev = "/dev/nano"
    s = init_serial()

    cv2.namedWindow("camera", 1)

    size = 18
    scale = 20
    # frame buffer 18x18
    fb = numpy.zeros((size, size), numpy.int)

    frame = 0
    pixel = 0
    while True:
        c = s.read(1)
        if not c:
            time.sleep(0.1)
            continue
        if ord(c[0]) & 0x80: # end of frame
            print "frame", frame, pixel, size * size
            frame += 1
            #print fb
            image = scipy.misc.imresize(fb, (size * scale, size * scale))
            cv2.imshow("camera", image)
            pixel = 0

            if getkey() == 27: # ESC
                break

        else:
            if pixel >= (size * size):
                continue
            fb[pixel/18][pixel%18] = ord(c)
            pixel += 1


    cv2.destroyAllWindows()

# FIN
