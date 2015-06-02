#!/usr/bin/python -u

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

#
#   Bresenham's Line Drawing Algorithm
#   see: http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

def line(plot, p0, p1):
    x0, y0 = p0
    x1, y1 = p1
    steep = abs(y1 - y0) > abs(x1 - x0)
    if steep:
        x0, y0 = y0, x0
        x1, y1 = y1, x1
    if x0 > x1:
        x0, x1 = x1, x0
        y0, y1 = y1, y0
    delta_x = x1 - x0
    delta_y = abs(y1 - y0)
    error = delta_x // 2
    y = y0
    if y0 < y1:
        y_step = 1
    else:
        y_step = -1

    for x in range(x0, x1+1):
        if steep:
            plot(y, x)
        else:
            plot(x, y)
        error -= delta_y
        if error < 0:
            y += y_step
            error += delta_x

#
#

def generate_c(lut):

    print "// Auto generated. Do not edit"
    print "//"

    print
    print "typedef struct { int x, y; } point;"
    print

    def print_seg(seg, pixels):
        print
        print "// segment %d" % seg
        print "const static point seg_%d[] = {" % seg
        for x, y in pixels:
            print "\t{ %d, %d }," % (x, y)
        print "\t{ 0xFF, 0xFF },"
        print "};"

    for seg, pixels in lut.items():
        print_seg(seg, pixels)

    print
    print "// Segment arrays"
    print
    print "const static point* segs[] = {";
    for seg in lut.keys():
        print "\tseg_%d," % seg
    print "\t0,"
    print "};"

#
#

def asint(data):
    p = []
    for d in data:
        p.append(int(d))
    return tuple(p)

def mul(m, data):
    p = []
    for d in data:
        p.append(d * m)
    return asint(p)

assert asint([ 2.3, 4.5 ] ) == (2, 4)

def frame(path, opts):
    cv2.namedWindow("mouse", 1)

    dial = cv2.imread(path)

    size = 18
    scale = 20
    image = numpy.zeros((size,size), dtype=numpy.uint8)
    image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
    image = cv2.resize(image, (size*scale, size*scale))

    def getpixel(x, y):
        return dial[size-x-1][size-y-1]

    centre = mul(size, [ opts.x0, opts.y0 ] )
    white = 255, 255, 255
    grey = 128, 128, 128
    red = 0, 0, 255
    blue = 255, 0, 0

    r1 = opts.r1
    r2 = opts.r2

    # Segments
    def xy(angle, r, as_int=True):
        x = math.sin(angle) * r * size
        y = math.cos(angle) * r * size
        xx, yy = centre[0] + x, centre[1] + y
        if as_int:
            xx, yy = int(xx), int(yy)
        return xx, yy
    def s_to_angle(s):
        angle = s * 2 * math.pi / segs
        return angle

    segs = 64

    def make_seg(s):
        a1 = s_to_angle(-s)
        a2 = s_to_angle(-(s+1))
        poly = [
            xy(a1, r1, 0), xy(a1, r2, 0),
            xy(a2, r2, 0), xy(a2, r1, 0), 
        ]
        poly.append(poly[0])
        return poly

    def show_seg(image, s):
        poly = make_seg(s)
        for i in range(len(poly)-1):
            p1, p2 = poly[i], poly[i+1]
            p1 = int(p1[0]), int(p1[1])
            p2 = int(p2[0]), int(p2[1])
            cv2.line(image, p1, p2, red, 1)

    # show pixels
    def show_pixels(im, pixels, outline=False):
        for x, y in pixels:
            pt = mul(scale, (x+0.5, y+0.5))
            #print x, y
            pixel = getpixel(x, y)
            colour = tuple([ int(a) for a in pixel])
            cv2.circle(im, pt, int(scale/2), colour, -1)
            if outline:
                cv2.circle(im, pt, int(scale/2), red, 1)

    def seg_to_pixels(s):
        poly = make_seg(s)

        class Plot:
            def __init__(self):
                self.data = {}
            def plot(self, x, y):
                self.data[mul(1.0/scale, [x, y])] = True
            def get(self):
                return self.data.keys()
        p = Plot()
        for i in range(len(poly)-1):
            p1 = int(poly[i][0]), int(poly[i][1])
            p2 = int(poly[i+1][0]), int(poly[i+1][1])
            line(p.plot, p1, p2)

        return p.get()

    lut = {}
    for s in range(segs):
        lut[s] = seg_to_pixels(s)

    def av_pixels(pixels):
        s = 0
        for x, y in pixels:
            p = getpixel(x, y)
            p = p[1] # green pixel
            s += p
        s /= float(len(pixels))
        return int(s)

    if opts.c:
        # generate C code for the pixel blocks
        generate_c(lut)
        return

    if 1:
        for x in range(size):
            for y in range(size):
                pixels = [ (x, y) ]
                show_pixels(image, pixels)

    if 1:
        cv2.circle(image, centre, int(r1*size), white)
        cv2.circle(image, centre, int(r2*size), white)

    # Grid
    if 0:
        for x in range(size):
            cv2.line(image, (x*scale, 0), (x*scale, size*scale), grey, 1)
        for y in range(size):
            cv2.line(image, (0, y*scale), (scale*size, y*scale), grey, 1)

    for s in range(segs):
        angle = s_to_angle(s)
        cv2.line(image, xy(angle, r1), xy(angle, r2), white, 1)

    s = 0
    paused = False
    while True:
        if not paused:
            im = image.copy()

            show_pixels(im, lut[s], True)

            print av_pixels(lut[s]),

            cv2.imshow("mouse", im)

        key = cv2.waitKey(500)
        if key == 27:
            break
        if key == ord('p'):
            paused = not paused

        s += 1
        if s >= segs:
            s = 0
            print

    cv2.destroyAllWindows()

#
#

if __name__ == "__main__":
    p = optparse.OptionParser()
    p.add_option("-d", "--dev", dest="dev", default="/dev/nano")
    p.add_option("-t", "--test", dest="test", action="store_true")
    p.add_option("-v", "--video", dest="video", action="store_true")
    p.add_option("-c", "--c", dest="c", action="store_true")
    p.add_option("-f", "--frame", dest="frame")
    p.add_option("-r", "--r1", dest="r1", type="float", default=5.0)
    p.add_option("-R", "--r2", dest="r2", type="float", default=8.0)
    p.add_option("-x", "--x0", dest="x0", type="float", default=9.5)
    p.add_option("-y", "--y0", dest="y0", type="float", default=9.5)
    opts, args = p.parse_args()

    serial_dev = opts.dev

    if opts.video:
        video(opts)
    elif opts.frame:
        frame(opts.frame, opts)
    
# FIN
