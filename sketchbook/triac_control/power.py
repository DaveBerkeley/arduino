#!/usr/bin/python

import sys
import math

def angle_to_count(a):
    # a in radians, count in 0..10000 us for 0..pi/2
    return a * 10000.0 / (math.pi)

power = []

for i in range(1800):
    angle = math.radians(i / 10.0)
    sin = math.sin(angle)
    integral = (0.5 * angle) - (0.25 * math.sin(2 * angle))
    #print angle, sin, sin * sin, integral
    power.append(integral)

maxp = power[-1]

steps = 100
step_p = maxp / float(steps)

idx = 0

print "static int lut[] = {"

for i in range(steps):

    nextp = i * step_p

    while power[idx] < nextp:
        #print >> sys.stderr, "next", i
        idx += 1

    angle = math.radians(idx / 10.0)
    count = angle_to_count(angle)
    #print idx, i, power[idx], count
    print "  %d," % int(count),
    if not ((i+1) % 10):
        print 

print "};"

#print maxp, step_p, math.pi, math.pi / 2

# FIN
