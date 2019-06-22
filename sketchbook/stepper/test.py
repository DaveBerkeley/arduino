#!/usr/bin/python

import time
import threading
import serial

def log(*args):
    print time.strftime("%y/%m/%d %H:%M:%S :"), 
    for arg in args:
        print arg,
    print

def command(text):
    global s
    log("cmd", text)
    s.write(text + "\r\n")

dead = False;

def listen(s):
    while not dead:
        t = s.read(1024)
        if not t:
            continue
        t = t.strip()
        log(t)

#
#

dev = "/dev/arduino"

s = serial.Serial(dev, 9600, timeout=0.1)

thread = threading.Thread(target=listen, args=(s,))
thread.start()

end_stop = 3800

# flush the stepper's command buffer
time.sleep(1);
command("")
time.sleep(1);
command("S%d" % end_stop);
time.sleep(1);

try:
    while True:
        for i in range(0, end_stop, 100):
            command("G%d" % i);
            time.sleep(0.5);
        command("G0")
        time.sleep(5)
except KeyboardInterrupt:
    pass

dead = True
thread.join()

# FIN
