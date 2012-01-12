#!/usr/bin/env python

"""
Script to compute extinction from a series of ULTRACAM log files, computing
the coefficient on the way up and down separately. It plots thedata binned 
for clarity versus the best-fit linear extinction laws. It needs a stack of
third-party modules: scipy, numpy, matplotlib, pyfits, trm.subs, trm.sla.

To use it you need to edit (copy it over) the following:

 1) the contributing run numbers, 
 2) the star position, 
 3) the observatory,
 4) the target aperture to use,
 5) the time bin width,
 6) the rejection threshold for removing bad data.

There should be more good data than bad for the auto-rejection to work effectively.
"""

import numpy as np
import matplotlib.pyplot as plt
import pyfits
from scipy.optimize import leastsq, fmin
from trm import subs, sla

# Names of runs (names assumed to be of form 'run123.fits' -- you provide the numbers)
runs = (11, 12, 13, 15, 19)

# Star position
ra,dec,syst = subs.str2radec('06 30 32.79 +29 40 20.3')

# Observatory
tel,obs,longit,latit,height = subs.observatory('WHT')

# Target aperture to use
nap = 2

# Bin width in minutes
deltat = 1.

# Rejection threshold, RMS
thresh = 2.5

def linfit(p, x):
    return p[0]+p[1]*x

def chisq(p, x, y):
    return ((y-linfit(p,x))**2).sum()

airs = []
mags = []
has  = []
for run in runs:
    hdul  = pyfits.open('run' + ('%03d' % (run,)) + '.fits')
    for i in range(1,4):
        table = hdul[i].data
        ok    = table.field('Sigma_' + str(nap)) > 0.
        mjd   = table.field('MJD')[ok]
        flx   = table.field('Counts_' + str(nap))[ok]

        # Bin up fluxes and times
        nbin     = int(np.ceil((mjd.max() - mjd.min())/(deltat/1440.)))
        bins     = np.linspace(mjd.min(), mjd.max(), nbin)
        nbins, b = np.histogram(mjd, bins)
        bmjd, b  = np.histogram(mjd, bins, weights=mjd)
        bflx, b  = np.histogram(mjd, bins, weights=flx)
        bmjd    /= nbins
        bflx    /= nbins

        # Convert to airmass and magnitudes
        air,alt,az,ha,pa,delz = sla.amass(bmjd, longit, latit, height, ra, dec) 
        mag = 25-2.5*np.log10(bflx)

        if len(airs) < 3:
            airs.append([air[ha < 0], air[ha >= 0]])
            mags.append([mag[ha < 0], mag[ha >= 0]])
        else:
            airs[i-1][0] = np.concatenate((airs[i-1][0],air[ha < 0]))
            airs[i-1][1] = np.concatenate((airs[i-1][1],air[ha >= 0]))
            mags[i-1][0] = np.concatenate((mags[i-1][0],mag[ha < 0]))
            mags[i-1][1] = np.concatenate((mags[i-1][1],mag[ha >= 0]))
 
    hdul.close()
    
# Fit and plot results

for air,mag,c in zip(airs,mags,('r','g','b')):
    x  = np.array([9.3, 0.1])
    for i in (0,1):
        ok = True
        while ok:
            x     = fmin(chisq, x, (air[i], mag[i]), disp=False)
            fit   = linfit(x, air[i])
            diff  = mag[i]-fit
            rms   = diff.std()
            adiff = np.abs(diff)
            imax  = np.argmax(adiff)
            if adiff[imax] > thresh*rms:
                keep = np.arange(len(air[i])) != imax
                air[i] = air[i][keep]
                mag[i] = mag[i][keep]
            else:
                ok = False

        plt.plot(air[i], fit, c)
        if i == 0:
            print 'Solution for colour = ',c,', HA < 0:',x
            plt.plot(air[i], mag[i], c + '.')  
        else:
            print 'Solution for colour = ',c,', HA > 0:',x
            plt.plot(air[i], mag[i], c + 'o')        

plt.show()


