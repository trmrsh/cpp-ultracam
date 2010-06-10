#!/usr/bin/env python

# !!begin
# !!title    checkbias
# !!author   T.R. Marsh
# !!created  01 May 2010
# !!root     checkbias
# !!index    checkbias
# !!descr    Checks biases for being OK
# !!class    Scripts
# !!class    Observing
# !!css      style.css
# !!head1    checkbias checks biases for being OK
#
# !!emph{checkbias} reads a list of ucm files that are to be used for making biases and checks them for consistency
# using a set of user-defined lower and upper levels and variances, window by window. It is designed for ucm files
# produced by ULTRACAM and may fail with other instruments.
#
# !!head2 Invocation
#
# checkbias flist llevels hlevels spreads
#
# !!head2 Arguments
#
# !!table
#
# !!arg{flist}{List of ucm files to be considered for a bias}
# !!arg{llevels}{Lowest levels of means of each window, 6 values, left & right for each CCD. These are designed to spot
# bias problems (too low, too high).}
# !!arg{hlevels}{Highest levels of means of each window, 6 values, left & right for each CCD.}}
# !!arg{spreads}{Maximum range of means of each window, 6 values left & right for each CCD. These are designed to be sensitive
# to lights being switched on during the bias.}
#
# !!table
# !!end

import sys, os.path, copy
import numpy as np
import trm.subs as subs
import trm.subs.input as inp
import trm.ucm as ucm

inpt = inp.Input('ULTRACAM_ENV', '.ultracam', sys.argv)

# register parameters
inpt.register('flist',    inp.Input.GLOBAL, inp.Input.PROMPT)
inpt.register('llevels',  inp.Input.GLOBAL, inp.Input.PROMPT)
inpt.register('hlevels',  inp.Input.GLOBAL, inp.Input.PROMPT)
inpt.register('spread',   inp.Input.GLOBAL, inp.Input.PROMPT)

flist   = inpt.get_value('flist',   'list of ucm files', subs.Fname('bias', '.lis'))

llevels = inpt.get_value('llevels', 'lower limits to window mean levels', (1.,2.,3.,4.,5.,6.))
if len(llevels) != 6:
    print 'You must specify 6 values for llevels: red-left, red-right, green-left etc'
    exit(1)

hlevels = inpt.get_value('hlevels', 'upper limits to window mean levels', (10.,20.,30.,40.,50.,60.))
if len(hlevels) != 6:
    print 'You must specify 6 values for hlevels: red-left, red-right, green-left etc'
    exit(1)

spreads = inpt.get_value('spread', 'maximum spread in window mean levels', 5.)

# Read in file list make sure it ends up as as ucm files
fstr   = open(flist)
fnames = [x.strip() if x.strip().endswith('.ucm') else x.strip() + '.ucm' for x in fstr.readlines()]
fstr.close()

for fname in fnames:
    if not os.path.isfile(fname):
        print '\n',fname,'is not a file; script aborted.'
        exit(1)

# OK now have a list of valid file names, compute and store means
# while checking format compatibility.
    
first = True
means = []
for fname in fnames:
    ufile = ucm.rucm(fname)
    if first:
        fufile = copy.deepcopy(ufile)
        nwin = ufile.nwin(0)
        for nc in range(1,ufile.nccd()):
            if ufile.nwin(nc) != nwin:
                print 'Script only works for same number of windows for each CCD'
                exit(1)
        ffname = fname
        first  = False

    if ufile != fufile:
        print '\nFormat of',fname,'clashes with that of the first file read',ffname
        exit(1)

    mc = []
    for nc in range(ufile.nccd()):
        mw = []
        for nw in range(ufile.nwin(nc)):
            mw.append(ufile.win(nc,nw).mean())
        mc.append(mw)
    means.append(mc)

means = np.array(means)
(nframe,nccd,nwin) = means.shape

lmean = means[:,:,0::2]
rmean = means[:,:,1::2]
lmin  = np.min(np.min(lmean,2),0)
rmin  = np.min(np.min(rmean,2),0)
lmax  = np.max(np.max(lmean,2),0)
rmax  = np.max(np.max(rmean,2),0)

mins = np.c_[lmin,rmin].reshape(-1)
maxs = np.c_[lmax,rmax].reshape(-1)

llevels = np.array(llevels)
hlevels = np.array(hlevels)

print 'Minimum mean values = ',mins
print 'Maximum mean values = ',maxs

ok =0
if np.any(mins < llevels) or np.any(maxs > hlevels):
    print "Some mean values were out of range"
    ok = 1
else:
    print 'All mean values were within range'


