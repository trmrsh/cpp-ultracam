#!/usr/bin/env python
#
# !!begin
# !!title    grms
# !!author   T.R. Marsh
# !!created  16 Oct 2012
# !!root     grms
# !!index    grms
# !!descr    generates RMS frame
# !!class    Scripts
# !!class    Observing
# !!calss    Calibration
# !!css      style.css
# !!head1    gmrs generate RMS frame
#
# !!emph{grms} computes the RMS of a frame, useful for identifying peppering in flats.
#
# Type "grms -h" for info on arguments.
#
# !!end
descr = \
"""
Generates RMS vs position frames. This is help identify peppered frames. It should be applied to flat fields
thought to be near-peppered. The peppering region shows up as a high region in the middle.
"""

import argparse, os.path
import numpy as np
from scipy import signal
from trm import ucm
from ppgplot import *

class UcmFile(argparse.FileType):
    def __call__(self, string):
        base, ext = os.path.splitext(string)
        if ext != '.ucm':
            return super(UcmFile, self).__call__(string + '.ucm')
        else:            
            return super(UcmFile, self).__call__(string)            

def gauss_kern(blurr):
    """ Returns a normalized 2D gauss kernel array for convolutions """
    width = int(3*blurr)+1
    x, y  = np.mgrid[-width:width+1, -width:width+1]
    g     = np.exp(-(x/blurr)**2/2.-(y/blurr)**2/2.)
    return g / g.sum()
 
# arguments
parser = argparse.ArgumentParser(description=descr)

# positional
parser.add_argument('idata', type=UcmFile('rb'), help='input data, a standard ucm frame.')

parser.add_argument('odata', type=UcmFile('wb'), help='output data, a standard ucm frame.')

# optional
parser.add_argument('-d', dest='dblurr', type=float, default=3.,\
                   help='RMS to blurr data with in order to measure deviations.')

parser.add_argument('-v', dest='vblurr', type=float, default=3.,\
                    help='RMS to blurr variances with.')

# parse them
args = parser.parse_args()

# read the frame
uc   = ucm.rucm(args.idata)
data = uc.data

gd = gauss_kern(args.dblurr)
gv = gauss_kern(args.vblurr)

for nccd,ccd in enumerate(data):
    for nwin,win in enumerate(ccd):
        print 'Processing window',nwin+1,'of CCD',nccd+1
        devsq  = win - signal.convolve2d( win, gd, mode='same', boundary='symm')
        devsq *= devsq
        data[nccd][nwin] = np.sqrt(signal.convolve2d( devsq, gv, mode='same', boundary='symm'))

uc.write(args.odata)
