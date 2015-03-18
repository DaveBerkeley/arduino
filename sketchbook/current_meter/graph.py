
import time

import matplotlib
matplotlib.use('TkAgg')

from matplotlib import pyplot
import pylab 

import serial

s = serial.Serial("/dev/ttyUSB0", 9600, timeout=1)

def get_data_():
    ii = []
    vv = []

    while True:
        line = s.readline()
        print `line`
        if not line:
            continue
        if line.startswith("***"):
            break

        try:
            parts = line.split(" ")
            v, i = [ int(x,  10) for x in parts ]
        except:
            continue
        ii.append(i)
        vv.append(v)

    return ii, vv

def get_data():
    while True:
        ii, vv = get_data_()
        if len(ii) >= 128:
            return ii, vv

#
#

fout = open("/tmp/mains.csv", "w")

ax = pylab.subplot(111)
canvas = ax.figure.canvas

i, v = get_data()
i, v = get_data()
v_line, = pyplot.plot(v, animated=True, lw=2)
i_line, = pyplot.plot(i, animated=True, lw=2)

def run():
    background = canvas.copy_from_bbox(ax.bbox)

    while True:
        print time.ctime()
        i, v = get_data()
        print >> fout, str(v)[1:-1]
        fout.flush()
        canvas.restore_region(background)
        v_line.set_ydata(v)
        ax.draw_artist(v_line)
        i_line.set_ydata(i)
        ax.draw_artist(i_line)
        canvas.blit(ax.bbox)
    
pylab.subplots_adjust(left=0.3, bottom=0.3) # check for flipy bugs
pylab.grid() # to ensure proper background restore
manager = pylab.get_current_fig_manager()
manager.window.after(100, run)

pylab.show()

# FIN
