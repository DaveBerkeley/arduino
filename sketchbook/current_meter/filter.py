#!/usr/bin/python

import time
import datetime
import math
import sys
from subprocess import Popen, PIPE

#
#

def normalise(data):
    d = [ float(x) for x in data ]
    mean = sum(d) / float(len(d))
    return [ (x-mean) for x in d ]

def normal(data):
    d = normalise(data)
    mx = max(d)
    return [ (x/mx) for x in data ]

fridge = {
    "name" : "fridge",
    "data" : 
        #'166.7', '169.4', '167.6', '169.1', '167.1', '169.0', '166.7', '166.2', '168.5', '167.0', '168.5', '166.0', '168.2', '166.6', '167.2', '167.1', '167.7', '166.1', '167.5', '166.5', '166.5', '167.6', '167.0', '168.4', '166.7', '167.3', '166.3', '168.1', '166.2', '166.8', '166.5', '167.2', '166.2', '166.0', '168.8', '167.2', '169.5', '168.7', '169.2', '167.0', 
        normalise(['167.8', '167.1', '167.5', '166.8', '167.7', '167.2', '166.9', '168.3', '166.6', '168.2', '166.8', '167.9', '166.6', '168.1', '1877.0', '1881.1', '1890.2', '1902.8', '1800.4', '1732.0', '1629.6', '1480.4', '1429.9', '1435.0', '1440.4', '1435.4', '1451.1', '1428.7', '1450.3', '1441.4', '1449.3', '1437.3', '1438.8', '1454.6', '1439.7', '1446.5', '1432.6', '1447.4', '1433.3', '1439.1', '1421.4', '1432.3', '1416.1', '1415.0', '1393.7', '1384.1', '1380.1', '1342.9', '1320.8', '1263.6', '1203.2', '1057.6', '817.2', '557.2', '434.8', '387.9', '347.6', '326.6', '318.0', '304.5', '295.2', '297.6', '288.1', '282.8', '280.6', '279.2', '274.2', '272.3', '268.5', '270.1', '267.6', '264.2', '264.7'])
        #'262.8', '264.9', '260.9', '260.6', '259.4', '261.0', '256.9', '257.0', '254.5', '257.4', '254.2', '255.3', '253.3', '254.7', '256.3', '253.0', '255.0', '252.6', '254.6', '251.5', '253.4', '251.5', '253.5', '251.1', '254.1', '251.5', '251.7', '250.0', '250.9', '252.8', '250.6', '253.0', '250.9', '253.7', '249.9', '252.1', '250.9', '252.0', '250.7', '251.6', '250.0', '251.8', '249.2', '250.6', '248.3', '248.5', '252.0', '249.3', '251.2', '249.1', '251.4', '249.1', '251.0', '248.4', '251.3', '250.1', '250.6', '249.1', '251.8', '248.8', '251.1', '247.5', '248.7', '251.6', '250.2', '252.3', '248.6', '251.6', '249.1', '250.3', '249.1', '250.7', '248.2', '249.9', '249.5', '250.6', '248.0', '248.8', '248.1', '249.6', '246.1', '247.8', '249.4', '247.8', '250.3', '247.7', '249.5',
        ,
}

def pad(points, length):
    if len(points) >= length:
        return points
    pad = [ 0.0, ] * (length - len(points))
    return points + pad

def closeness(a, offset, b):
    total = 0.0
    for i in range(len(b)):
        d = a[i+offset] * b[i]
        total += d
    return total

def match(points, f):
    print "match"
    n = normalise(points)
    result = []
    for i in range(len(points)-len(f)):
        p = closeness(points, i, f)
        result.append(p)
    d_max = max(result)
    p_max = max(points)
    scale = p_max / d_max
    return [ (scale*x) for x in result ]

#
#

def gnuplot(title, *data):
    # create the data file
    path = "/tmp/dave.txt"
    f = open(path, "w")
    for info in zip(*data):
        for item in info:
            print >> f, item,
        print >> f
    f.close()

    cmd = [
        "set terminal png",
        "set key off",
        "set title '%s'" % title,
    ]

    plots = []
    for i in range(len(data)):
        plots.append("'%s' using %d w l" % (path, i+1))
    plot = "plot " + ",".join(plots)

    cmd.append(plot)
    cmd = "\n".join(cmd)

    p = Popen("gnuplot", stdin=PIPE, stdout=PIPE, close_fds=True)
    p.stdin.write(cmd)
    p.stdin.close()

    png_path = "/tmp/graph.png"
    png = p.stdout.read()
    f = open(png_path, "wb")
    f.write(png)
    f.close()

    import os
    os.system("eog %s" % png_path)

#
#

def reader():
    f = open(path)
    while True:
        line = f.readline()
        if not line:
            time.sleep(0.1)
            continue

        parts = line.split()
        if len(parts) != 51:
            continue
        tt = parts[0]
        dt = datetime.datetime.strptime(tt, "%H%M%S")

        global find_t
        if find_t:
            if find_t != tt:
                continue
            find_t = None

        pp = [ float(x) for x in parts[1:] ]
        for i, power in enumerate(pp):
            dt.replace(microsecond = 20000 * i)
            yield dt, power

#
#

class EdgeDetector:

    def __init__(self):
        self.prev = [ None, ] * 3
        self.av = [ None, ] * 4

    def add(self, point):
        old = self.prev[0]
        self.prev = self.prev[1:] + [ point ]
        if old is None:
            return 0.0
        diff = point - self.prev[0]
        old = self.av[0]
        self.av = self.av[1:] + [ diff ]

        if old == None:
            return 0.0

        return sum(self.av) / float(len(self.av))

    def length(self):
        return len(self.prev) + len(self.av)

#
#

def hist(width, data):
    bins = [ 0, ] * width
    d_min = min(data)
    d_max = max(data)

    for p in data:
        idx = int((width-1) * (p - d_min) / (d_max - d_min))
        bins[idx] += 1

    scale = abs(d_max / max(bins))
    return [ (100+(scale*x)) for x in bins ]

#
#

class Store:

    def __init__(self, size):
        self.data = [ None, ] * size

    def add(self, point):
        old = self.data[0]
        self.data = self.data[1:] + [ point, ]
        return old

    def array(self):
        return self.data

#
#

class Filter:

    def __init__(self, taps):
        self.taps = taps

    def filt(self, data):
        result = []
        for i in range(len(data) - len(self.taps)):
            r = 0.0
            for j in range(len(self.taps)):
                r += data[i+j] * self.taps[j]
            result.append(r)
        return result

#
#

path = "/usr/local/data/power/2012/07/07.log.aux"

find_t = None
if len(sys.argv) > 1:
    find_t = sys.argv[1]

width = 200

edge = EdgeDetector()

store = Store(width)
d_store = Store(width)
found = []

for dt, power in reader():
    #print dt, power

    old = store.add(power)

    p = edge.add(power)

    d_store.add(p)

    if old is None:
        continue

    if not found:
        if abs(p) > 10:
            print dt, power, p
            next_t = dt + datetime.timedelta(seconds=3)
            found.append((p, next_t))

    if found and (dt == found[0][1]):

        f_width = 20
        d = store.array()
        mean1 = sum(d[:f_width]) / f_width
        mean2 = sum(d[-f_width:]) / f_width
        step = mean2 - mean1

        # find max peak in d_store
        d_max = max([ abs(x) for x in d_store.array()])

        print dt, "Step", step, d_max, abs(d_max / step)

        if abs(step) > 5:
            wave = [ 1, 5, 1, -1, -5, -1 ]
            self_corr = Filter(wave)
            f3 = self_corr.filt(store.array())

            lpf = [ 1.0/20, ] * 20
            xx = Filter(lpf)
            f3 = xx.filt(f3)

            more = Filter(lpf)
            f4 = more.filt(store.array())
            
            a = store.array()
            dc = [ (x-a[0]) for x in a ]

            info = ( 
                dc,
                #f3,
                #f4,
            )
            gnuplot(dt.strftime("%H:%M:%S"), *info)

        found = found[1:]

# FIN
