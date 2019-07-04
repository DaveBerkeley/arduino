#!/usr/bin/python

import time
import threading
import serial
import random

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

class Motor:

    def __init__(self):
        self.rsp = "XXX0"

    def listen(self, s):
        text = ""
        while not dead:
            t = s.read()
            if not t:
                continue
            text += t
            if not t in "\r\n":
                continue
            text, rsp = "", text.strip()
            if not rsp:
                continue
            log(rsp)
            self.rsp = rsp

    def ready(self):
        return self.rsp[0] == "R"

    def posn(self):
        return int(self.rsp[3:])

    def wait_for(self, p):
        while p != self.posn():
            time.sleep(0.01)

#
#

dev = "/dev/arduino"

s = serial.Serial(dev, 9600, timeout=0.1)

motor = Motor()

thread = threading.Thread(target=motor.listen, args=(s,))
thread.start()

end_stop = 4096

# flush the stepper's command buffer
time.sleep(1);
command("")
time.sleep(1);
command("S%d" % end_stop);
time.sleep(1);

try:

    while False:
        j = random.randint(0, end_stop-1)
        command("G%d" % j);
        motor.wait_for(j)
        time.sleep(0.5)

        while True:
            command("P0") # power off
            time.sleep(0.01)
            command("P1") # power on
            time.sleep(0.01)

    while True:
        for z in range(0, end_stop, 100):

            for i in range(4):
                j = random.randint(0, end_stop-1)
                command("G%d" % j);
                motor.wait_for(j)
                time.sleep(0.5)

            command("G0")
            motor.wait_for(0)
            time.sleep(1)

except KeyboardInterrupt:
    pass

dead = True
thread.join()

# FIN
