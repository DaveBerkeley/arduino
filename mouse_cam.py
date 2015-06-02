#!/usr/bin/python

import time
import optparse
import math

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
    baud = 9600
    log("open serial '%s'" % serial_dev)
    s = serial.Serial(serial_dev, baudrate=baud, timeout=1, rtscts=True)

    log("serial opened @ %d" % baud)
    return s

#
#

def getkey():
    return cv2.waitKey(1) & 0xFF

def video(opts):
    s = init_serial()

    cv2.namedWindow("mouse", 1)

    size = 18
    scale = 20
    # frame buffer 18x18
    fb = numpy.zeros((size, size), numpy.uint8)

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
            image = scipy.misc.imresize(fb, (size * scale, size * scale))
            cv2.imshow("mouse", image)
            pixel = 0

            if getkey() == 27: # ESC
                break

        else:
            if pixel >= (size * size):
                continue
            if opts.test:
                c = chr(pixel%256)
            # remap to match mouse mapping
            fb[(size-1)-(pixel%size)][pixel/size] = ord(c)
            pixel += 1

    cv2.destroyAllWindows()

#
#

def frame(path):
    cv2.namedWindow("mouse", 1)

    image = cv2.imread(path)

    size = 18
    scale = 20
    image = cv2.resize(image, (size*scale, size*scale))

    centre = int(9.5 * size), int(9.5 * size)
    white = 255, 255, 255
    grey = 128, 128, 128
    r1 = 3
    cv2.circle(image, centre, r1*size, grey)
    r2 = 9
    cv2.circle(image, centre, r2*size, grey)

    # Grid
    for x in range(size):
        cv2.line(image, (x*scale, 0), (x*scale, size*scale), grey, 1)
    for y in range(size):
        cv2.line(image, (0, y*scale), (scale*size, y*scale), grey, 1)

    # Segments
    def xy(angle, r):
        x = int(math.sin(angle) * r * size)
        y = int(math.cos(angle) * r * size)
        return centre[0] + x, centre[1] + y
    segs = 32
    for s in range(segs):
        angle = s * 2 * math.pi / segs
        cv2.line(image, xy(angle, r1), xy(angle, r2), white, 1)

    def show_seg(image, s):
        red = 0, 0, 255
        angle = -s * 2 * math.pi / segs
        cv2.line(image, xy(angle, r1), xy(angle, r2), red, 1)
        s += 1
        angle = -s * 2 * math.pi / segs
        cv2.line(image, xy(angle, r1), xy(angle, r2), red, 1)

    s = 0
    while True:
        im = image.copy()
        show_seg(im, s)
        cv2.imshow("mouse", im)

        if getkey() == 27: # ESC
            break

        time.sleep(1)
        s += 1
        if s >= segs:
            s = 0

    cv2.destroyAllWindows()

#
#

if __name__ == "__main__":
    p = optparse.OptionParser()
    p.add_option("-d", "--dev", dest="dev", default="/dev/nano")
    p.add_option("-t", "--test", dest="test", action="store_true")
    p.add_option("-v", "--video", dest="video", action="store_true")
    p.add_option("-f", "--frame", dest="frame")
    opts, args = p.parse_args()

    serial_dev = opts.dev

    if opts.video:
        video(opts)
    elif opts.frame:
        frame(opts.frame)
    
# FIN
