#!/usr/bin/env python
#
# !!begin
# !!title    makebias
# !!author   T.R. Marsh
# !!created  30 April 2010
# !!root     hotpix
# !!index    hotpix
# !!descr    Plots locations and values of hot pixels in dark frames
# !!class    Scripts
# !!class    Observing
# !!calss    Calibration
# !!css      style.css
# !!head1    hotpix plots locations and values of hot pixels in dark frames
#
# !!emph{hotpix} allos you to plot the values of hot pixels in ULTRACAM dark frames
# indicating values of hot pixels above a user-defined threshold in counts/second.
# 
# Run the script with -h to see options and arguments. To run this script, the Python
# module trm.ucm must be installed along with numpy and matplotlib. You will need a reference
# dark
# !!end

import matplotlib.pyplot as plt
from matplotlib.transforms import offset_copy
import numpy as np
from optparse import OptionParser

import trm.ucm

usage = """usage: %prog [options] dark

Plots positions and count rates per second of hot pixels. 
matplotlib allows you to zoom into the plot.

dark -- name of a dark frame (ucm format)

"""

parser = OptionParser(usage)
parser.add_option("-n", "--nccd", dest="nccd", default=0, type="int", \
                      help="CCD number 1=red, 2=green, 3=blue, 0=all; default=0")
parser.add_option("-t", "--thresh", dest="thresh", default=20., type="float", \
                      help="threshold for displaying hot pixels; default = 20")

(options, args) = parser.parse_args()

thresh = options.thresh
nccd   = options.nccd
dark   = args[0]

ucm  = trm.ucm.rucm(dark)
texp = ucm['Exposure']['value']

fig = plt.figure()

ax = plt.subplot(111)
transOffset = offset_copy(ax.transData, fig=fig, x=0.05, y=0.10, units='inches')

if nccd == 0:
    ccds = (0,1,2)
else:
    ccds = (nccd-1,)
cols = ('r','g','b')

for nc in ccds:
    nwin = len(ucm.data[nc])
    for nw in xrange(nwin):
        (llx,lly) = ucm.off[nc][nw]
        (ny,nx)   = ucm.data[nc][nw].shape

        X = np.linspace(llx,llx+ucm.xbin*nx-1,nx)
        Y = np.linspace(lly,lly+ucm.ybin*ny-1,ny)
        X, Y = np.meshgrid(X, Y)

        image = ucm.data[nc][nw] / texp

        # identify pixels counting above thresh / second
        mask = image > thresh
        x = X[mask]
        y = Y[mask]
        z = image[mask]
        plt.plot(x, y, cols[nc] + '.')
        for xi, yi, zi in zip(x,y,z):
            plt.text(xi, yi, '%d' % (int(zi),), transform=transOffset)

plt.xlim(0,1025)
plt.ylim(0,1025)
ax.set_aspect('equal', 'box')

plt.show()
