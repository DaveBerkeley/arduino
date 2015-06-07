#!/usr/bin/python -u

import time
import datetime
import optparse
import os

import serial

#
#

class Filter:

    def __init__(self, sectors):
        self.sectors = sectors
        self.data = None
        self.last = None
        self.count = 0
        self.rot = False
    def add(self, data):
        # filter out small reverse travel
        if not self.last is None:
            diff = data - self.last
            if diff in [ -1, -2, self.sectors-1, self.sectors-2 ]:
                #print "ignore it"
                return
        if data == self.data:
            self.count += 1
        else:
            self.count = 0
        self.data = data
    def get(self):
        if self.count >= 3:
            if not self.last is None:
                diff = self.data - self.last
                if diff in [ -(self.sectors-1), -(self.sectors-2) ]:
                    self.rot = True

            self.last = self.data
            return self.data
        return None
    def rotation(self):
        # detect rotations
        r = self.rot
        self.rot = False
        return r

#
#

if __name__ == "__main__":

    cfeet_2_cmeters = 0.0283168466
    parkinson_cowen = 0.017
    rot = parkinson_cowen * cfeet_2_cmeters

    p = optparse.OptionParser()
    p.add_option("-d", "--dev", dest="dev", default="/dev/nano")
    #p.add_option("-b", "--base", dest="base", default="/usr/local/data/gas")
    p.add_option("-b", "--base", dest="base", default="/tmp/gas")
    p.add_option("-s", "--sectors", dest="sectors", default=64)
    p.add_option("-r", "--rotation", dest="rotation", default=rot)
    opts, args = p.parse_args()

    s = None
    last_sector = None
    last_time = datetime.datetime.now()
    f = None
    filt = Filter(opts.sectors)
    rotations = 0

    def parse(line):
        line = line.strip()
        return int(line)

    while True:
        if s is None:
            s = serial.Serial(opts.dev, 9600)

        try:
            line = s.readline()
        except:
            s = None
            time.sleep(5)
            continue

        if not line:
            time.sleep(1)
            continue

        try:
            sector = parse(line)
        except ValueError:
            continue

        filt.add(sector)
        filtered = filt.get()
        #print sector, filtered
        if filtered is None:
            continue

        if filt.rotation():
            rotations += 1

        now = datetime.datetime.now()
        ymd = now.strftime("%Y/%m/%d.log")
        hm = now.strftime("%H%M%S")
        path = os.path.join(opts.base, ymd)

        dirname, x = os.path.split(path)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
            f = None
            last_sector = None

        if f is None:
            f = file(path, "a")

        if filtered != last_sector:
            last_sector = filtered
            this_rot = (filtered / float(opts.sectors)) * opts.rotation
            rot = this_rot + (rotations * opts.rotation)
            #print "log", hm, filtered, rotations, rot
            print >> f, hm, filtered, rotations, "%.5f" % rot
            f.flush()

# FIN
