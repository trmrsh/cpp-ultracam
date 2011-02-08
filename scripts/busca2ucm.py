#!/usr/bin/env python
#
#
# !!begin
# !!title    busca2ucm
# !!author   T.R. Marsh
# !!revised  08 Feb 2011
# !!root     busca2ucm
# !!index    busca2ucm
# !!descr    Converts BUSCA data to ucm format.
# !!class    IO
# !!class    Scripts
# !!css      style.css
# !!head1    busca2ucm converts Calar Alto BUSCA data to ucm format.
#
# Reads in BUSCA data FITS files and converts to ULTRACAM ucm format files. Note that BUSCA data cannot quite
# be read into ucm data generally at the moment because of the possibility of different exposure times in the
# different CCDs. I may change this in the future, but at the moment one should watch for warnings from this
# routine.
#
# !!head2 Invocation
#
# busca2ucm file1 file2 ...
#
# !!head2 Arguments
#
# !!table
# !!arg{file1 file2 ...}{names of FITS input files to read in. Could be wildcarded. A set of files such as 
# 'flat1_a.fits', 'flat1_b.fits', 'flat1_c.fits', 'flat1_d.fits', 'bias1_a.fits', 'bias1_b.fits', 
# 'bias1_c.fits', 'bias1_d.fits', will be converted to flat1.ucm and bias1.ucm. For simplicity, only '.fits' 
# is accepted as the file extension.}
# !!table
#
# See also !!ref{fits2ucm.html}{fits2ucm}
#
# !!end

from optparse import OptionParser

usage = """
usage: %prog [options] file1 file2 file3 ...

Reads in BUSCA data FITS files and converts to ULTRACAM ucm format files.Note that BUSCA data cannot quite
be read into ucm data generally at the moment because of the possibility of different exposure times in the
different CCDs. I may change this in the future, but at the moment one should watch for warnings from this
routine. 

Arguments:

file1, file2 etc -- names of FITS input files to read in. Could be wildcarded. A set of files such as 
                    'flat1_a.fits', 'flat1_b.fits', 'flat1_c.fits', 'flat1_d.fits', 'bias1_a.fits', 
                     'bias1_b.fits', 'bias1_c.fits', 'bias1_d.fits', will be converted
                     to flat1.ucm and bias1.ucm. For simplicity, only '.fits' is accepted as the file 
                     extension.
"""

parser = OptionParser(usage)

#parser.add_option("-b", dest="break", default=1., type="float",\
#                          help="scaling factor for jump distribution; default = 1")
#    parser.add_option("-e", dest="emax", default=3., type="float",\
#                          help="maximum error in sec. for residual plot; default = 3")
#    parser.add_option("-n", dest="nmcmc", default=100000, type="int",\
#                          help="number of MCMC iterations; default = 100,000")
#    parser.add_option("--ns", dest="nstore", default=100, type="int",\
#                          help="how often to store MCMC results; default = 100")
#    parser.add_option("-j", dest="jfile", \
#                          help="name of MCMC log file to define jump and initial model")
#    parser.add_option("-l", dest="lfile", default="fittimes.log", \
#                          help="name of MCMC output log; default = fittimes.log")
#    parser.add_option("-m", dest="mout",\
#                          help="output model file if wanted")

(options, fnames) = parser.parse_args()

if len(fnames) < 1:
    print 'You need to supply at least one filename; use -h for more help'
    exit(1)

roots = set()
for fname in fnames:
    if not fname.endswith('.fits'):
        print 'ERROR: File =',fname,'does not end with ".fits"'
        exit(1)
    root = fname[:-5]
    if not root.endswith('_a') and not root.endswith('_b') and not root.endswith('_c')  and not root.endswith('_d'):
        print 'ERROR: File base =',root,'does not end with any of _a, _b, _c, _d before the final .fits'
        exit(1)
    roots.add(root[:-2])

import re, pyfits
import numpy as np
import trm.subs as subs
import trm.ucm as ucm

wre = re.compile(r'^\[(\d+):\d+,(\d+):\d+\]$')
endings = ('_a','_b','_c','_d')

for root in roots:

    # Going to create a 4 CCD ucm file
    data   = 4*[[]]
    off    = 4*[[]]
    target = None
    utc    = None
    expose = None

    # Going to wind through each input image to squirrel away the windows 
    nwin    = None
    for nc,ending in enumerate(endings):
        fname = root + ending + '.fits'
        hdulist = pyfits.open(fname)
        phead = hdulist[0].header
        image = hdulist[0].data

        if nwin is None:
            nwin  = phead['winnr']
        else:
            if phead['winnr'] != nwin:
                print 'ERROR: Clashing number of windows =',phead['winnr'],'in',fname
                exit(1)

        # Lack of examples at the moment forces the next part
        if phead['sequence'].find('bin2') == -1:
            print 'WARNING: did not find "bin2" in "sequence" FITS key but will still assume xbin=ybin=2'
        xbin  = 2
        ybin  = 2
        NXTOT = 4096
        NYTOT = 4096

        data[nc] = nwin*[[]]
        off[nc]  = nwin*[[]]

        # Now set the windows
        (ny,nx) = image.shape
        if nx % 2 != 0:
            print 'ERROR: Image X dimension =',nx,'not a multiple of 2'
            print 'Program assumes that right-hand of images is overscan which will be ignored'
            exit(1)
        if ny % nwin != 0:
            print 'ERROR: Image Y dimension =',ny,'indivisible by number of windows =',nwin
            exit(1)
        ny //= nwin

        for nw in range(nwin):
            window = phead['WINXY' + str(nw)]
            m = wre.match(window)
            if not m:
                print 'ERROR: Failed to interpret "' + window + '" as a CCD window'
                exit(1)

            llx = m.group(1)
            lly = m.group(2)

            off[nc][nw]  = (xbin*(int(llx)-1)+1,ybin*(int(lly)-1)+1)
            data[nc][nw] = image[ny*nw:ny*(nw+1),0:nx/2]
        if target is None and 'object' in phead:
            target = phead['object']
        elif target != phead['object']:
            print 'WARNING: OBJECT clash in',fname
        if expose is None:
            expose = phead['exptime']
        elif expose != phead['exptime']:
            print 'WARNING: EXPTIME clash in ' + fname + ':',expose,'vs',phead['exptime'],'(new vs old)'
        if utc is None:
            mjd = int(phead['julian']-2400000.5)
            utc = subs.hms2d(phead['shopen'])
        elif utc != subs.hms2d(phead['shopen']) or mjd != int(phead['julian']-2400000.5):
            print 'WARNING: time clash in ' + fname + ':',subs.hms2d(phead['shopen']),int(phead['julian']-2400000.5),\
                'vs',utc,mjd,'(new vs old)'
        hdulist.close()
        
    # create headers
    head = subs.Odict()
    if target is not None:
        head['Object'] = {'value' : target, 'comment' : 'The target name', 'type' : ucm.ITYPE_STRING}
    if utc is not None and expose is not None:
        utc += expose/2./3600.
        if utc >= 24.:
            utc -= 24.
            mjd += 1
        head['UT_date'] = {'value' : (mjd, utc), 'comment' : 'UTC at exposure mid-point', 'type' : ucm.ITYPE_TIME}
    if expose is not None:
        head['Exposure'] = {'value' : expose, 'comment' : 'Exposure time, seconds', 'type' : ucm.ITYPE_FLOAT}

    # create and write Ucm
    frame = ucm.Ucm(head, data, off, xbin, ybin, NXTOT, NYTOT)
    frame.write(root)
    print 'Created and written',root + '.ucm','to disk.'
