#!/usr/bin/env python

"""
Reads in an ultracam log file and generates an equivalent
FITS file with the data stored in binary tables. These are
about half the size of the ASCII log files. For large log files
this can be quite slow. The routine tries not to use too much
memory by accumulating in numpy arrays. This uses pyfits which 
cannot write gzipped fits files, but can read them, so you should
gzip the files at the end which compresses quite a bit further still
(~ factor 5). Once generated the FITS files are much faster to read
than their ASCII equivalents.

Invocation:

ulog2fits.py ulog clobber

ulog     -- name of ASCII log file, '.log' will be appended.
clobber  -- y or n to clobber files on output
"""

import sys

if len(sys.argv) != 3:
    print 'usage: ' + sys.argv[0] + ' ulog clobber'
    exit(1)

ulog  = sys.argv[1]
if sys.argv[2] == 'y' or sys.argv[2] == 'Y':
    clobber = True
elif sys.argv[2] == 'n' or sys.argv[2] == 'N':
    clobber = False
else:
    print '"clobber" must be "y" or "n"'
    exit(1)

if ulog.rfind('.log') == -1 or ulog.rfind('.log') != len(ulog) - 4:
    ulog += '.log'

ufits = ulog[:-4] + '.fits'

import os.path

if not os.path.exists(ulog):
    print 'Cannot find input file = ' + ulog
    exit(1)

if not clobber and os.path.exists(ufits):
    print 'Output file = ' + ufits + ' already exists'
    exit(1)

# OK, now for the real work.

import numpy as npy
import pyfits

# initialise dictionaries keyed on CCD number

# these will be lists for flexible but somewhat inefficient storage
tcols   = {}
tmjd    = {}
ttflag  = {}
tnsat   = {}
texpose = {}
tfwhm   = {}
tbeta   = {}
tx      = {}
ty      = {}
txm     = {}
tym     = {}
texm    = {}
teym    = {}
tcounts = {}
tsigma  = {}
tsky    = {}
tnsky   = {}
tnrej   = {}
tworst  = {}
teflag  = {}

# these will be numpy arrays for more efficient use of memory
cols    = {}
mjd     = {}
tflag   = {}
nsat    = {}
expose  = {}
fwhm    = {}
beta    = {}
x       = {}
y       = {}
xm      = {}
ym      = {}
exm     = {}
eym     = {}
counts  = {}
sigma   = {}
sky     = {}
nsky    = {}
nrej    = {}
worst   = {}
eflag   = {}

nchunk  = {}

found_all_ccds = False

header = pyfits.Header()
header.update('LOGFILE', ulog) 
header.add_comment('File created from ULTRACAM log file. One binary table per CCD.')
header.add_comment('This was created using ult2fits which is based on pyfits.')
header.add_comment('There follows the comments from the log file, verbatim.')

nline  = 0
nbytes = os.path.getsize(ulog)
tlines = 0
fin    = open(ulog)
NMAX   = 10000
NREP   = 2000
nhead  = 0
nunit  = 0
nlhead = 0
print

format = None

for line in fin:
    nline += 1
    if nline % NREP == 0 and tlines:
        sys.stdout.write('\r' + ('...%4.1f' % (100.*nline/tlines)) + '%   ')
        sys.stdout.flush()

    if line[0:1] == '#':
        header.add_comment(line[1:].rstrip())
        nhead  += len(line)
        nlhead += 1
        if line.startswith('# name/number mjd flag nsat'):
            # old-style log files with number of satellites
            format = 1
            print 'This is a pre-March 2010 reduce log file.'
        if line.startswith('# name/number mjd flag expose'):
            # post March 2010 log files
            format = 2
            print 'This is a post-March 2010 reduce log file.'
            
    elif not line.isspace():

        if format is None:
            print 'Could not identify the format of the log file.'
            exit(1)

        svar = line.split()

# we accumulate apertures numbers for each new CCD encounter, but
# if we re-find a CCD, we check that aperture numbers match. Also
# check that extra CCDs are not found after all were thought to have
# been found
                    
        if (format == 1 and (len(svar) - 8 ) % 14 > 0) or (format == 2 and (len(svar) - 7 ) % 14 > 0):
            print 'ulog2fits: incorrect number of entries in line ' + str(nline) + ' of ' + ulog
            fin.close()
            exit(1)

        if format == 1:
            nc  = int(svar[5])
        elif format == 2:
            nc  = int(svar[4])

        if nc in cols:
            found_all_ccds = True
            tlines = nlhead + len(cols)*(nbytes - nhead)/nunit
        else:

            if found_all_ccds:
                print 'ulog2fits: new CCD was found even though all were thought to be found in line ' + str(nline) + ' of ' + ulog

            nunit += len(line)

            # Define the columns
            cols[nc] = []
            cols[nc].append(pyfits.Column(name='MJD', unit='day', format='D'))
            cols[nc].append(pyfits.Column(name='Flag', format='L'))
            if format == 1: cols[nc].append(pyfits.Column(name='Nsat', format='I'))
            cols[nc].append(pyfits.Column(name='Expose', unit='sec', format='E'))
            cols[nc].append(pyfits.Column(name='FWHM', unit='pix', format='E'))
            cols[nc].append(pyfits.Column(name='beta', format='E'))
            if format == 1:
                nap  = (len(svar) - 8 ) / 14
            elif format == 2:
                nap  = (len(svar) - 7 ) / 14

            for ap in range(nap):
                snap = str(ap+1)
                cols[nc].append(pyfits.Column(name='X ' + snap, unit='pix', format='E'))
                cols[nc].append(pyfits.Column(name='Y ' + snap, unit='pix', format='E'))
                cols[nc].append(pyfits.Column(name='XM ' + snap, unit='pix', format='E'))
                cols[nc].append(pyfits.Column(name='YM ' + snap, unit='pix', format='E'))
                cols[nc].append(pyfits.Column(name='EXM ' + snap, unit='pix', format='E'))
                cols[nc].append(pyfits.Column(name='EYM ' + snap, unit='pix', format='E'))
                cols[nc].append(pyfits.Column(name='Counts ' + snap, unit='DN', format='E'))
                cols[nc].append(pyfits.Column(name='Sigma ' + snap, unit='DN', format='E'))
                cols[nc].append(pyfits.Column(name='Sky ' + snap, unit='DN/pix', format='E'))
                cols[nc].append(pyfits.Column(name='Nsky ' + snap, format='J'))
                cols[nc].append(pyfits.Column(name='Nrej ' + snap, format='J'))
                cols[nc].append(pyfits.Column(name='Worst ' + snap, format='J'))
                cols[nc].append(pyfits.Column(name='Eflag ' + snap, format='J'))

            # Define the lists
            tmjd[nc]    = []
            ttflag[nc]  = []
            if format == 1: tnsat[nc]   = []
            texpose[nc] = []
            tfwhm[nc]   = []
            tbeta[nc]   = []
            tx[nc]      = []
            ty[nc]      = []
            txm[nc]     = []
            tym[nc]     = []
            texm[nc]    = []
            teym[nc]    = []
            tcounts[nc] = []
            tsigma[nc]  = []
            tsky[nc]    = []
            tnsky[nc]   = []
            tnrej[nc]   = []
            tworst[nc]  = []
            teflag[nc]  = []
            nchunk[nc]  = 0

        # squirrel the data away

        if format == 1:
            tmjd[nc].append(float(svar[1]))
            ttflag[nc].append(bool(int(svar[2])))
            tnsat[nc].append(int(svar[3]))
            texpose[nc].append(float(svar[4]))
            tfwhm[nc].append(float(svar[6]))
            tbeta[nc].append(float(svar[7]))
            tx[nc].append(npy.cast['float32'](svar[9::14]))
            ty[nc].append(npy.cast['float32'](svar[10::14]))
            txm[nc].append(npy.cast['float32'](svar[11::14]))
            tym[nc].append(npy.cast['float32'](svar[12::14]))
            texm[nc].append(npy.cast['float32'](svar[13::14]))
            teym[nc].append(npy.cast['float32'](svar[14::14]))
            tcounts[nc].append(npy.cast['float32'](svar[15::14]))
            tsigma[nc].append(npy.cast['float32'](svar[16::14]))
            tsky[nc].append(npy.cast['float32'](svar[17::14]))
            tnsky[nc].append(npy.cast['int32'](svar[18::14]))
            tnrej[nc].append(npy.cast['int32'](svar[19::14]))
            tworst[nc].append(npy.cast['int32'](svar[20::14]))
            teflag[nc].append(npy.cast['int32'](svar[21::14]))
        elif format == 2:
            tmjd[nc].append(float(svar[1]))
            ttflag[nc].append(bool(int(svar[2])))
            texpose[nc].append(float(svar[3]))
            tfwhm[nc].append(float(svar[5]))
            tbeta[nc].append(float(svar[6]))
            tx[nc].append(npy.cast['float32'](svar[8::14]))
            ty[nc].append(npy.cast['float32'](svar[9::14]))
            txm[nc].append(npy.cast['float32'](svar[10::14]))
            tym[nc].append(npy.cast['float32'](svar[11::14]))
            texm[nc].append(npy.cast['float32'](svar[12::14]))
            teym[nc].append(npy.cast['float32'](svar[13::14]))
            tcounts[nc].append(npy.cast['float32'](svar[14::14]))
            tsigma[nc].append(npy.cast['float32'](svar[15::14]))
            tsky[nc].append(npy.cast['float32'](svar[16::14]))
            tnsky[nc].append(npy.cast['int32'](svar[17::14]))
            tnrej[nc].append(npy.cast['int32'](svar[18::14]))
            tworst[nc].append(npy.cast['int32'](svar[19::14]))
            teflag[nc].append(npy.cast['int32'](svar[20::14]))

        # in an endevour to reduce memory usage, convert data to numpy arrays
        # every so often
        if len(tmjd[nc]) == NMAX:
            if nchunk[nc] == 0:
                mjd[nc]    = npy.array(tmjd[nc])
                tflag[nc]  = npy.array(ttflag[nc])
                if format == 1: nsat[nc]   = npy.array(tnsat[nc], dtype='int16')
                expose[nc] = npy.array(texpose[nc], dtype='float32')
                fwhm[nc]   = npy.array(tfwhm[nc], dtype='float32')
                beta[nc]   = npy.array(tbeta[nc], dtype='float32')
                x[nc]      = npy.array(tx[nc])
                y[nc]      = npy.array(ty[nc])
                xm[nc]     = npy.array(txm[nc])
                ym[nc]     = npy.array(tym[nc])
                exm[nc]    = npy.array(texm[nc])
                eym[nc]    = npy.array(teym[nc])
                counts[nc] = npy.array(tcounts[nc])
                sigma[nc]  = npy.array(tsigma[nc])
                sky[nc]    = npy.array(tsky[nc])
                nsky[nc]   = npy.array(tnsky[nc])
                nrej[nc]   = npy.array(tnrej[nc])
                worst[nc]  = npy.array(tworst[nc])
                eflag[nc]  = npy.array(teflag[nc])
                nchunk[nc] += 1
            else:
                mjd[nc]    = npy.append(mjd[nc], tmjd[nc])
                tflag[nc]  = npy.append(tflag[nc], ttflag[nc])
                if format == 1: nsat[nc]   = npy.append(nsat[nc], npy.array(tnsat[nc], dtype='int16'))
                expose[nc] = npy.append(expose[nc], npy.array(texpose[nc], dtype='float32'))
                fwhm[nc]   = npy.append(fwhm[nc], npy.array(tfwhm[nc], dtype='float32'))
                beta[nc]   = npy.append(beta[nc], npy.array(tbeta[nc], dtype='float32'))
                x[nc]      = npy.append(x[nc], tx[nc], 0)
                y[nc]      = npy.append(y[nc], ty[nc], 0)
                xm[nc]     = npy.append(xm[nc], txm[nc], 0)
                ym[nc]     = npy.append(ym[nc], tym[nc], 0)
                exm[nc]    = npy.append(exm[nc], texm[nc], 0)
                eym[nc]    = npy.append(eym[nc], teym[nc], 0)
                counts[nc] = npy.append(counts[nc], tcounts[nc], 0)
                sigma[nc]  = npy.append(sigma[nc], tsigma[nc], 0)
                sky[nc]    = npy.append(sky[nc], tsky[nc], 0)
                nsky[nc]   = npy.append(nsky[nc], tnsky[nc], 0)
                nrej[nc]   = npy.append(nrej[nc], tnrej[nc], 0)
                worst[nc]  = npy.append(worst[nc], tworst[nc], 0)
                eflag[nc]  = npy.append(eflag[nc], teflag[nc], 0)
                    
            # zero the temporary arrays
            tmjd[nc]    = []
            ttflag[nc]  = []
            if format == 1: tnsat[nc]   = []
            texpose[nc] = []
            tfwhm[nc]   = []
            tbeta[nc]   = []
            tx[nc]      = []
            ty[nc]      = []
            txm[nc]     = []
            tym[nc]     = []
            texm[nc]    = []
            teym[nc]    = []
            tcounts[nc] = []
            tsigma[nc]  = []
            tsky[nc]    = []
            tnsky[nc]   = []
            tnrej[nc]   = []
            tworst[nc]  = []
            teflag[nc]  = []

fin.close()

# All data from log file is now read. Some may have already been
# put into numpy arrays, some may be left in the temporary buffers
# these need to be flushed out

# Create primary HDU, no data but lots of header
hdus = [pyfits.PrimaryHDU(header=header),]
            
nccd = tmjd.keys()
nccd.sort()
for nc in nccd:
    if len(tmjd[nc]) > 0:
        if nchunk[nc] == 0:
            mjd[nc]    = npy.array(tmjd[nc])
            tflag[nc]  = npy.array(ttflag[nc])
            if format == 1: nsat[nc]   = npy.array(tnsat[nc], dtype='int16')
            expose[nc] = npy.array(texpose[nc], dtype='float32')
            fwhm[nc]   = npy.array(tfwhm[nc], dtype='float32')
            beta[nc]   = npy.array(tbeta[nc], dtype='float32')
            x[nc]      = npy.array(tx[nc])
            y[nc]      = npy.array(ty[nc])
            xm[nc]     = npy.array(txm[nc])
            ym[nc]     = npy.array(tym[nc])
            exm[nc]    = npy.array(texm[nc])
            eym[nc]    = npy.array(teym[nc])
            counts[nc] = npy.array(tcounts[nc])
            sigma[nc]  = npy.array(tsigma[nc])
            sky[nc]    = npy.array(tsky[nc])
            nsky[nc]   = npy.array(tnsky[nc])
            nrej[nc]   = npy.array(tnrej[nc])
            worst[nc]  = npy.array(tworst[nc])
            eflag[nc]  = npy.array(teflag[nc])
        else:
            mjd[nc]    = npy.append(mjd[nc], tmjd[nc])
            tflag[nc]  = npy.append(tflag[nc], ttflag[nc])
            if format == 1: nsat[nc]   = npy.append(nsat[nc], npy.array(tnsat[nc], dtype='int16'))
            expose[nc] = npy.append(expose[nc], npy.array(texpose[nc], dtype='float32'))
            fwhm[nc]   = npy.append(fwhm[nc], npy.array(tfwhm[nc], dtype='float32'))
            beta[nc]   = npy.append(beta[nc], npy.array(tbeta[nc], dtype='float32'))
            x[nc]      = npy.append(x[nc], tx[nc], 0)
            y[nc]      = npy.append(y[nc], ty[nc], 0)
            xm[nc]     = npy.append(xm[nc], txm[nc], 0)
            ym[nc]     = npy.append(ym[nc], tym[nc], 0)
            exm[nc]    = npy.append(exm[nc], texm[nc], 0)
            eym[nc]    = npy.append(eym[nc], teym[nc], 0)
            counts[nc] = npy.append(counts[nc], tcounts[nc], 0)
            sigma[nc]  = npy.append(sigma[nc], tsigma[nc], 0)
            sky[nc]    = npy.append(sky[nc], tsky[nc], 0)
            nsky[nc]   = npy.append(nsky[nc], tnsky[nc], 0)
            nrej[nc]   = npy.append(nrej[nc], tnrej[nc], 0)
            worst[nc]  = npy.append(worst[nc], tworst[nc], 0)
            eflag[nc]  = npy.append(eflag[nc], teflag[nc], 0)

    # Create tables

    theader = pyfits.Header()
    theader.update('NCCD', nc)
    theader.update('NAPERTUR', x[nc].shape[1])
    tbhdu = pyfits.new_table(cols[nc], theader, len(x[nc]))
    tbhdu.header.update('NCCD', nc)
    tbhdu.header.update('EXTNAME', 'CCD ' + str(nc))
    tbhdu.data.field('MJD')[:]    = mjd[nc]
    tbhdu.data.field('Flag')[:]   = tflag[nc]
    if format == 1: tbhdu.data.field('Nsat')[:]   = nsat[nc]
    tbhdu.data.field('Expose')[:] = expose[nc]
    tbhdu.data.field('FWHM')[:]   = fwhm[nc]
    tbhdu.data.field('beta')[:]   = beta[nc]
    for ap in range(x[nc].shape[1]):
        snap = str(ap+1)
        tbhdu.data.field('X ' + snap)[:]      = x[nc][:,ap]
        tbhdu.data.field('Y ' + snap)[:]      = y[nc][:,ap]
        tbhdu.data.field('XM ' + snap)[:]     = xm[nc][:,ap]
        tbhdu.data.field('YM ' + snap)[:]     = ym[nc][:,ap]
        tbhdu.data.field('Counts ' + snap)[:] = counts[nc][:,ap]
        tbhdu.data.field('Sigma ' + snap)[:]  = sigma[nc][:,ap]
        tbhdu.data.field('Sky ' + snap)[:]    = sky[nc][:,ap]
        tbhdu.data.field('Nsky ' + snap)[:]   = nsky[nc][:,ap]
        tbhdu.data.field('Nrej ' + snap)[:]   = nrej[nc][:,ap]
        tbhdu.data.field('Worst ' + snap)[:]  = worst[nc][:,ap]
        tbhdu.data.field('Eflag ' + snap)[:]  = eflag[nc][:,ap]
    hdus.append(tbhdu)

    # save memory
    del mjd[nc]
    del tflag[nc]
    if format == 1: del nsat[nc]
    del expose[nc]
    del fwhm[nc]
    del beta[nc]
    del x[nc]
    del y[nc]
    del xm[nc]
    del ym[nc]
    del counts[nc]
    del sigma[nc]
    del sky[nc]
    del nsky[nc]
    del nrej[nc]
    del worst[nc]
    del eflag[nc]

# finally write out the fits file!
hdulist = pyfits.HDUList(hdus)
hdulist.writeto(ufits, clobber=clobber)
print '\nFinished.'





