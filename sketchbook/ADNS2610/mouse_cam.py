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

size = 18
scale = 20

white = 255, 255, 255
grey = 128, 128, 128
red = 0, 0, 255
blue = 255, 0, 0

#
#

def init_serial():
    baud = 9600
    log("open serial '%s'" % serial_dev)
    s = serial.Serial(serial_dev, baudrate=baud, timeout=1, rtscts=True)

    log("serial opened @ %d" % baud)
    return s

#
#   Convert between mouse x,y mapping and index into array

def xy2i(x, y):
    idx = y * size
    idx += (size-1) - x
    return idx

def i2xy(idx):
    x, y = (size-1)-(idx%size), idx/size
    return x, y

assert xy2i(3, 7) == 140

#
#

def getkey():
    return cv2.waitKey(1) & 0xFF

def video(opts):
    s = init_serial()
    s.write('v')

    cv2.namedWindow("video", 1)

    detect = Detect(opts)

    # frame buffer 
    fb = numpy.zeros((size, size), numpy.float)
    prev = fb.copy()
    image = make_blank_image()

    lut = detect.make_lut()

    frame = 0
    pixel = 0
    while True:
        c = s.read(1)
        if not c:
            time.sleep(0.1)
            s.write('v')
            continue
        if ord(c[0]) & 0x80: # end of frame
            print "frame", frame, pixel, size * size
            frame += 1

            if opts.filter:
                filt = 0.9
                b = (filt * prev) + ((1 - filt) * fb)
                #prev = fb
                fb = b
                prev = b

            if opts.autogain:
                m = numpy.max(fb)
                if m:
                    fb = fb * int(255.0 / m)

            def getpixel(x, y):
                p = int(fb[y][x])
                return p, p, p
            im = image.copy()
            show_all_pixels(im, getpixel)

            if opts.detect:
                detect.draw_segs(im)
                score = []
                for seg in range(detect.segs):
                    score.append(av_pixels(lut[seg], getpixel))

                m = min(score)
                for seg, a in enumerate(score):
                    if a == m:
                        print "*",
                        hit = seg
                    print a,
                print ":", hit
                show_pixels(im, lut[hit], getpixel, red)

            cv2.imshow("video", im)
            pixel = 0

            if getkey() == 27: # ESC
                break

        else:
            if pixel >= (size * size):
                continue
            if opts.test:
                c = chr(pixel%0x3F)
            x, y = i2xy(pixel)
            fb[x][y] = ord(c) * 4
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

    def print_seg(seg, pixels):
        print
        print "// segment %d" % seg
        print "const static int seg_%d[] = {" % seg
        print "   ",
        for x, y in pixels:
            print "%d," % xy2i(x, y),
        print "-1,"
        print "};"

    for seg, pixels in lut.items():
        print_seg(seg, pixels)

    print
    print "// Segment arrays"
    print
    print "const static int* segs[] = {";
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

#
#

class Detect:

    def __init__(self, opts):
        self.segs = opts.segments
        self.centre = mul(size, [ opts.x0, opts.y0 ] )
        self.r1 = opts.r1
        self.r2 = opts.r2

    def s_to_angle(self, s):
        angle = s * 2 * math.pi / self.segs
        return angle

    def xy(self, angle, r, as_int=True):
        x = math.sin(angle) * r * size
        y = math.cos(angle) * r * size
        xx, yy = self.centre[0] + x, self.centre[1] + y
        if as_int:
            xx, yy = int(xx), int(yy)
        return xx, yy

    def make_seg(self, s):
        a1 = self.s_to_angle(-s)
        a2 = self.s_to_angle(-(s+1))
        poly = [
            self.xy(a1, self.r1, 0), self.xy(a1, self.r2, 0),
            self.xy(a2, self.r2, 0), self.xy(a2, self.r1, 0),
        ]
        poly.append(poly[0])

        # if more than 1-pixel gap between a1,r2 and a2,r2
        # subdivide
        def subdivide(a1, a2):
            xy0 = complex(*self.xy(a1, self.r2, 0))
            xy1 = complex(*self.xy(a2, self.r2, 0))
            d = abs(xy0 - xy1)
            if d <= (1.5 * size):
                return
            mid = (a1 + a2) / 2.0
            poly.append(self.xy(mid, self.r1, 0))
            poly.append(self.xy(mid, self.r2, 0))

            subdivide(a1, mid)
            subdivide(mid, a2)

        subdivide(a1, a2)

        return poly

    def seg_to_pixels(self, s):
        poly = self.make_seg(s)

        class Plot:
            def __init__(self):
                self.data = {}
            def plot(self, x, y):
                x, y = mul(1.0/scale, [x, y])
                if 0 <= x < size:
                    if 0 <= y < size:
                        self.data[(x, y)] = True
            def get(self):
                return self.data.keys()
        p = Plot()
        for i in range(len(poly)-1):
            p1 = int(poly[i][0]), int(poly[i][1])
            p2 = int(poly[i+1][0]), int(poly[i+1][1])
            line(p.plot, p1, p2)

        return p.get()

    def make_lut(self):
        lut = {}
        for s in range(self.segs):
            lut[s] = self.seg_to_pixels(s)
        return lut

    def draw_segs(self, image):
        cv2.circle(image, self.centre, int(self.r1*size), white)
        cv2.circle(image, self.centre, int(self.r2*size), white)

        # draw segs
        for s in range(self.segs):
            angle = self.s_to_angle(s)
            cv2.line(image, self.xy(angle, self.r1), self.xy(angle, self.r2), white, 1)

#
#

# show pixels
def show_pixels(im, pixels, getpixel, outline=None):
    for x, y in pixels:
        pt = mul(scale, (x+0.5, y+0.5))
        #print x, y
        pixel = getpixel(x, y)
        colour = tuple([ int(a) for a in pixel])
        cv2.circle(im, pt, int(scale/2), colour, -1)
        if outline:
            cv2.circle(im, pt, int(scale/2), outline, 1)

def show_all_pixels(image, getpixel):
    for x in range(size):
        for y in range(size):
            pixels = [ (x, y) ]
            show_pixels(image, pixels, getpixel)

#
#

def make_blank_image():
    image = numpy.zeros((size,size), dtype=numpy.uint8)
    image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
    image = cv2.resize(image, (size*scale, size*scale))
    return image

def av_pixels(pixels, getpixel):
    s = 0
    for x, y in pixels:
        p = getpixel(x, y)
        p = p[1] # green pixel
        s += p
    s /= float(len(pixels))
    return int(s)

def draw_graph(image, segs, avs):
    s = size * scale / 10
    y0, y1 = int(s * 4) , int(s * 6)
    w = size * scale
    for s in range(segs):
        if s >= len(avs):
            break
        x = int((s/float(segs)) * w)
        y = y0 + ((255-avs[s]) * (y1 - y0) / 256)
        print s, x
        cv2.circle(image, (x, y), int(w/segs), blue, -1)
    cv2.line(image, (0, y0), (size*scale, y0), blue, 1)
    cv2.line(image, (0, y1), (size*scale, y1), blue, 1)

#
#

def frame(path, opts):

    detect = Detect(opts)

    cv2.namedWindow("detect", 1)

    dial = cv2.imread(path)

    image = make_blank_image()

    def getpixel(x, y):
        return dial[size-x-1][size-y-1]

    # Segments
    segs = opts.segments

    lut = detect.make_lut()

    if opts.c:
        # generate C code for the pixel blocks
        generate_c(lut)
        return

    show_all_pixels(image, getpixel)
    detect.draw_segs(image)

    s = 0
    paused = False
    avs = []
    while True:
        if not paused:
            im = image.copy()

            show_pixels(im, lut[s], getpixel, red)

            av = av_pixels(lut[s], getpixel)
            avs.append(av)
            print av,

            if opts.save:
                path = "image_%06d.png" % s
                draw_graph(im, segs, avs)
                cv2.imwrite(path, im)
            cv2.imshow("detect", im)

        key = cv2.waitKey(500)
        if key == 27:
            break
        if key == ord('p'):
            paused = not paused

        s += 1
        if s >= segs:
            s = 0
            print
            if opts.save:
                break

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
    p.add_option("-R", "--r2", dest="r2", type="float", default=12.0)
    p.add_option("-x", "--x0", dest="x0", type="float", default=9.5)
    p.add_option("-y", "--y0", dest="y0", type="float", default=9.5)
    p.add_option("-s", "--segments", dest="segments", type="int", default=64)
    p.add_option("-a", "--autogain", dest="autogain", action="store_true")
    p.add_option("-F", "--filter", dest="filter", action="store_true")
    p.add_option("-D", "--detect", dest="detect", action="store_true")
    p.add_option("-S", "--save", dest="save", action="store_true")
    opts, args = p.parse_args()

    serial_dev = opts.dev

    if opts.video:
        video(opts)
    else:
        frame(opts.frame, opts)
    
# FIN
