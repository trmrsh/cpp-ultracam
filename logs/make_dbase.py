#!/usr/bin/env python

"""
Script to generate a database (as a FITS file) for use with dynamic web pages
to search for ULTRACAM runs. This should only be run once any new data are ingested
into the archive and as many targets have been identified and make_logs run. Only
targets with position IDs are tracked.

It should be run in the 'logs' directory which has sub-directories of 
the form '2005-11' (Nov 2005) which have a structure like so. 

"""

import os, sys, re, pyfits
from xml.dom import Node
from xml.dom.minidom import parse, parseString
import numpy as np
from trm import subs, simbad
import Ultra

#import traceback


# The main program

# Basic aim is to read as many xml and data files as possible. Errors
# are converted to warnings if possible and ideally the user should fix up the files
# until no such warnings appear.

# First read target data.
targets = Ultra.Targets('TARGETS', 'AUTO_TARGETS')

# File matching regular expressions
rdir_re = re.compile('^\d\d\d\d-\d\d$') # Search for YYYY-MM
ndir_re = re.compile('^\d\d\d\d-\d\d-\d\d$') # Search for night directories
xml_re  = re.compile('run\d\d\d\.xml$') # Search for xml files

# Targets to skip Simbad searches for; will be added to as more failures are found ensuring
# that searches for a given target are only made once.
fp    = open('SKIP_TARGETS')
sskip = fp.readlines()
sskip = [name.strip() for name in sskip if not name.startswith('#')]
fp.close()

# Create a list directories of runs to search through
rdirs = [x for x in os.listdir(os.curdir) if os.path.isdir(x) and rdir_re.match(x) is not None]
rdirs.sort()

# Data to store for each run
sfield  = []
ras     = []
decs    = []
runs    = []
nights  = []
numbers = []
targs   = []
ids     = []
exps    = []
coms    = []

for rdir in rdirs:

    # Try to find the telescope.
    try:
        f = open(os.path.join(rdir, 'telescope'))
        telescope = f.readline().rstrip()
        f.close()
        print 'Run directory =',rdir,', telescope =',telescope
    except Exception, err:
        telescope = None
        print 'Run directory =',rdir,',',err

    # Now the night-by-night directories
    ndirs = [x for x in os.listdir(rdir) if os.path.isdir(os.path.join(rdir,x)) and ndir_re.match(x) is not None]
    ndirs.sort()

    # now to the night-by-night files
    for ndir in ndirs:

        npath = os.path.join(rdir, ndir)
            
        # Read night log (does not matter if none exists, although a warning will be printed)
        nlog = Ultra.Log(os.path.join(npath, ndir + '.dat'))

        # Read timing data (does not matter if none exists, although a warning will be printed)
        times = Ultra.Times(os.path.join(npath, ndir + '.times'))

        # Read XML files for this night
        dpath = os.path.join(npath, 'data')
        xmls = [os.path.join(dpath,x) for x in os.listdir(dpath) if xml_re.match(x) is not None]
        xmls.sort()
        first = True
        expose = 0.
        for xml in xmls:
            try:
                run = Ultra.Run(xml, nlog, times, targets, telescope, ndir, rdir, sskip, True)

                # update targets to reduce simbad lookups
                if run.simbad:
                    if run.id in targets:
                        targets[run.id]['match'].append((run.target, True))
                    else:
                        targets[run.id] = {'ra' : subs.hms2d(run.ra), 'dec' : subs.hms2d(run.dec), 'match' : [(run.target, True),]}
                elif run.id is None:
                    sskip.append(run.target)

                if run.ra is not None and run.dec is not None:
                    sfield.append(run.ra + run.run + '%03d' % (run.number,))
                    rah, decd, csys = subs.str2radec(run.ra + ' ' + run.dec)
                    ras.append(rah)
                    decs.append(decd)
                    runs.append(run.run)
                    nights.append(run.night)
                    numbers.append(run.number)
                    targs.append(str(run.target.strip()))
                    if len(run.target.strip()) > 50:
                        print run.night,run.number,run.target
                    ids.append(run.id.strip())
                    exps.append(0. if run.expose is None else float(run.expose))
                    coms.append(run.comment.strip())
 
            except Exception, err:
                print 'XML error: ',err,'in',xml
#    if len(ids) >= 100: break

print '\nFound',len(ras),'runs. Will now generate database file.'

# Convert lists to numpy arrays
sfield  = np.array(sfield)
ras     = np.array(ras)
decs    = np.array(decs)
runs    = np.array(runs)
nights  = np.array(nights)
numbers = np.array(numbers)
targs   = np.array(targs)
ids     = np.array(ids)
exps    = np.array(exps) / 60.
coms    = np.array(coms)

# Sort on RA
isort   = np.argsort(sfield)

ras     = ras[isort]
decs    = decs[isort]
runs    = runs[isort]
nights  = nights[isort]
numbers = numbers[isort]
targs   = targs[isort]
ids     = ids[isort]
exps    = exps[isort]
coms    = coms[isort]

# Construct FITS file
cra     = pyfits.Column(name='RA', format='D', unit='hours', array=ras)
cdec    = pyfits.Column(name='Dec', format='D', unit='deg.', array=decs)
crun    = pyfits.Column(name='Run', format='7A', array=runs)
cnight  = pyfits.Column(name='Night', format='10A', array=nights)
cnumber = pyfits.Column(name='Num', format='I', array=numbers)
ctarg   = pyfits.Column(name='Target', format=str(targs.dtype.itemsize) + 'A', array=targs)
cids    = pyfits.Column(name='ID', format=str(ids.dtype.itemsize) + 'A', array=ids)
cexps   = pyfits.Column(name='Exposure', format='E', unit='mins', array=exps)
ccoms   = pyfits.Column(name='Comment', format=str(coms.dtype.itemsize) + 'A', array=coms)

cols    = pyfits.ColDefs([cra,cdec,crun,cnight,cnumber,ctarg,cids,cexps,ccoms])

tbhdu = pyfits.new_table(cols)
hdu   = pyfits.PrimaryHDU()
hdul  = pyfits.HDUList([hdu, tbhdu])
hdul.writeto('ultracam_dbase.fits')
