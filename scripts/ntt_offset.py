#!/usr/bin/env python

"""
Script to help offsetting when acquiring onto the slit with ULTRASPEC
on the NTT. You tell it the current rotator on the sky, your slit, 
your current X position and how far you want to move in Y, and it comes
back with how you should move the telescope.

NB This needs offsets in X for each slit which may change from run to run.

Written by TRM
"""

import math as m

print '\nGRB: for mindless aquisition of objects onto the EFOSC slits\n'

pa      = float(raw_input('Enter "Rotator on sky" value: '))
slit    = float(raw_input('Enter slit width: '))
slits   = (0.5,0.7,1.0,1.5,2.0,5.0)
xslit   = (556.,565.,556.,557.,556.,545.)
xoff    = None
for s,x in zip(slits,xslit):
    if s == slit:
        xoff = x

if xoff is None:
    print 'Did not recognize slit. Currently only the following values'
    print 'are accepted = ',slits
    exit(1)

scale = 0.1055

sp = m.sin(m.pi*pa/180.)
cp = m.cos(m.pi*pa/180.)

while True:

    xpos  = float(raw_input('Enter the current X position of your target: '))
    ymove = scale*float(raw_input('Enter the change in Y that you desire: '))
    xmove = scale*(xoff-xpos)

    east  = -xmove*sp - ymove*cp
    north = -xmove*cp + ymove*sp
    
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



                  
