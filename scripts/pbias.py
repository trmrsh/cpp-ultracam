#!/usr/bin/env python

descr = \
"""
Plots ULTRACAM biases. This is to allow high-contrast searches for problems in bias frames so
it removes the medians from each window separately so that the pixels should be split 
50/50 above and below the mid-plot level in each window.
"""

import argparse, os.path
import numpy as np
from trm import ucm
from ppgplot import *

class UcmFile(argparse.FileType):
    def __call__(self, string):
        base, ext = os.path.splitext(string)
        if ext != '.ucm':
            return super(UcmFile, self).__call__(string + '.ucm')
        else:            
            return super(TxtFile, self).__call__(string)            

# arguments
parser = argparse.ArgumentParser(description=descr)

# positional
parser.add_argument('bias', type=UcmFile('rb'), help='bias frame. It should be a standard ucm frame.')

# optional
parser.add_argument('-r', dest='range', type=float, default=20.,
                   help='range in counts about mean level to define upper and lower intensity levels.')

parser.add_argument('-d', dest='device', default='/xs', help='plot device.')

parser.add_argument('-n', dest='nccd', type=int, default=0, help='CCD number to plot; 0 for all')

parser.add_argument('-x1', dest='x1', type=float, default=0., help='left plot limit')

parser.add_argument('-x2', dest='x2', type=float, default=0., help='right plot limit')

parser.add_argument('-y1', dest='y1', type=float, default=0., help='lower plot limit')

parser.add_argument('-y2', dest='y2', type=float, default=0., help='upper plot limit')

# parse them
args = parser.parse_args()


# read the bias frame
bias = ucm.rucm(args.bias)

pgopen(args.device)
pgsch(1.5)
pgslw(3)

x1 = args.x1
y1 = args.y1
x2 = args.x2 if args.x2 != 0. else float(bias.nxtot)
y2 = args.y2 if args.y2 != 0. else float(bias.nytot)

nccd = len(bias.data)
if args.nccd == 0:
    pgsubp(nccd,1)
    for nc in range(nccd):
        pgpanl(nc+1,1)
        pgsci(1)
        pgwnad(x1,x2,y1,y2)

        # Remove median levels from each window
        for nw in range(len(bias.data[nc])):
            bias.data[nc][nw] -= np.median(bias.data[nc][nw])

        # plot
        bias.pggray(nc, -args.range/2.,args.range/2.)
        pgsci(4)
        pgbox('bcnst',0,0,'bcnst',0,0)
        pgsci(2)
        pglab('X','Y','CCD ' + str(nc+1))
else:
    if args.nccd < 0 or args.nccd > nccd:
        print 'CCD number must lie from 0 to',nccd
        exit(1)

    nc = args.nccd - 1
    pgsci(1)
    pgwnad(x1,x2,y1,y2)
        
    # Remove median levels from each window
    for nw in range(len(bias.data[nc])):
        bias.data[nc][nw] -= np.median(bias.data[nc][nw])
        
    # plot
    bias.pggray(nc, -args.range/2.,args.range/2.)
    pgsci(4)
    pgbox('bcnst',0,0,'bcnst',0,0)
    pgsci(2)
    pglab('X','Y','CCD ' + str(nc+1))

pgclos()
