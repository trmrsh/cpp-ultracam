#!/usr/bin/env python

"""
Script to help offsetting when observing with ULTRACAM
on the NTT. You tell it the current sky PA, how far you want 
to move in X and Y and it comes back with how far you should move #
the telescope

Written by TRM
"""

import math as m

print '\nFor setting up ULTRACAM/NTT observations\n'

pa     = float(raw_input('Enter "Rotator on sky" value: '))

scale  = 0.35
paoff  = 0.

cp = m.cos(m.radians(pa+paoff))
sp = m.sin(m.radians(pa+paoff))

print

while True:

    xmove  = float(raw_input('Enter movement in X (pixels): '))
    ymove  = float(raw_input('Enter movement in Y (pixels): '))

    east  = scale*(+xmove*cp + ymove*sp)
    north = scale*(-xmove*sp + ymove*cp)
    
    if east < 0:
        east *= -1
        e_or_w = 'East'
    else:
        e_or_w = 'West'

    if north < 0:
        north *= -1
        n_or_s = 'North'
    else:
        n_or_s= 'South'

    east  = '%5.1f' % (east,)
    north = '%5.1f' % (north,)
    print '\n!!! Ask the TO to move the telescope ' + east + '" to the ' + e_or_w + ' and ' + north + ' to the ' + n_or_s + ' !!!\n'



                  
