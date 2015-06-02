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
    red = 0, 0, 255
    blue = 255, 0, 0

    r1 = 4.0
    cv2.circle(image, centre, int(r1*size), grey)
    r2 = 8.5
    cv2.circle(image, centre, int(r2*size), grey)

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
    def s_to_angle(s):
        angle = s * 2 * math.pi / segs
        return angle

    segs = 16

    for s in range(segs):
        angle = s_to_angle(s)
        cv2.line(image, xy(angle, r1), xy(angle, r2), grey, 1)

    def show_seg(image, s):
        a1 = s_to_angle(-s)
        a2 = s_to_angle(-(s+1))
        poly = [
            xy(a1, r1), xy(a1, r2),
            xy(a2, r2), xy(a2, r1), 
        ]
        poly.append(poly[0])
        for i in range(len(poly)-1):
            p1, p2 = poly[i], poly[i+1]
            cv2.line(image, p1, p2, red, 1)

    # show pixels
    def show_pixels(pixels):
        for x, y in pixels:
            p = int((x+0.5) * scale), int((y+0.5) * scale)
            cv2.circle(image, p, int(scale/2), blue)

    pixels = []
    for x in range(size):
        for y in range(size):
            pixels.append((x, y))
    show_pixels(pixels)

    s = 0
    while True:
        im = image.copy()
        show_seg(im, s)
        cv2.imshow("mouse", im)

        if cv2.waitKey(500) == 27:
            break

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
