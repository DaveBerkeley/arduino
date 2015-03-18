
import time
import sys

import serial

target_cycle = 0
if len(sys.argv) > 1:
    target_cycle = int(sys.argv[1])

s = serial.Serial("/dev/ttyUSB0", 9600, timeout=1)

for i in range(10):
    time.sleep(0.1)
    s.write("W\n") # Set Wave mode

def get_data_():
    ii = []
    vv = []
    p_total = 0.0

    while True:
        line = s.readline()
        #print `line`
        if not line:
            continue
        if line.startswith("***"):
            break

        try:
            parts = line.split(" ")
            v = int(parts[0], 10) * (378 / 512.0)
            i = float(parts[1]) * (128.2 / 512.0)
        except:
            continue
        ii.append(i * 20)
        vv.append(v)

        p_total += i * v

    if vv:
        print p_total / len(vv)
    sys.stdout.flush()
    return ii, vv

def get_data():
    while True:
        ii, vv = get_data_()
        if len(ii) >= 128:
            return ii, vv

#
#

fout = open("/tmp/mains.csv", "w")

i, v = get_data()
i, v = get_data()

print i, v

from matplotlib import pyplot

pyplot.ion()
axes = pyplot.axes()

v_line, = pyplot.plot(v)
i_line, = pyplot.plot(i)

while True:
    i, v = get_data()

    v_line.set_ydata(v)
    i_line.set_ydata(i)
    pyplot.draw()


# FIN
