#!/usr/bin/env python

# !!begin
# !!title    procbias
# !!author   T.R. Marsh
# !!created  01 May 2010
# !!root     procbias
# !!index    procbias
# !!descr    Processes biases
# !!class    Scripts
# !!class    Observing
# !!css      style.css
# !!head1    procbias processes biases runs
#
# !!emph{procbias} grabs a set of frames from a bias run, checks them for consistency and then combines
# the results. Consistency checks are made of the the range of mean values of each half of each CCD and
# on the range of means encountered. The emphasis is on trying to be careful and !!emph{procbias} should
# be considered to be a superior version of !!ref{makebias}. It is very specifically designed for ULTRACAM
# data.
#
# !!head2 Invocation
#
# procbias run [source debug offset] first last [format] (llevels hlevels) spread info bias
#
# !!head2 Arguments
#
# !!table
#
# !!arg{run}{List of ucm files to be considered for a bias}
# !!arg{source}{data source, 's' for server, 'l' for local}
# !!arg{debug}{print extra information to diagnose problems}
# !!arg{offset}{Offset in hours to subtract from times to get them to be at the start of the night. This parameter is needed so that the
# automatically generated filenames for the bias produced start with the same date for any given night. Its value depends upon
# observatory longitude (as the times are in UT) and when the dividing time between one night and the next is defined. Probably 09.00am
# local time would be a good approx to the latter in which case one would set offset = 9 + (degrees west)/15.}
# !!arg{first}{first run to consider. Should usually be 1, but the first few frames of drift mode
# runs are junk and should be avoided.}
# !!arg{last}{last run to consider. Set to a large number to get the lot.}
# !!arg{format}{If true, the allowed mean level ranges of each window are set by the code itself. If false, you get the chance
# to define them by hand. Note that these levels vary with readout speed. The automatic option has a set of
# recognised formats for which values have been set.}
# !!arg{llevels}{Lowest levels of means of each window, 6 values, left & right for each CCD. These are designed to spot
# bias problems (too low, too high). Unfortunately the correct values depend upon the binning.}
# !!arg{hlevels}{Highest levels of means of each window, 6 values, left & right for each CCD.}}
# !!arg{spread}{Maximum range between the means of each half of the CCD, 6 values left & right for each CCD. 
# The lowest and highest mean values of windows in each CCD half is used. This value should be set fairly
# tightly to spot potential problems with drift mode biases and lights being switched on.}
# !!arg{log}{Name of file to log a line of information on this bias (appended). This will allow the build up of a database}
# !!arg{bias}{Name of output bias. This should be a short descriptive part that will be added to a standarized descriptor that will
# be built from the date and other properties.}
#
# !!table
# !!end

import sys, os, os.path, copy, subprocess
import numpy as np
import trm.subs as subs
import trm.subs.input as inp
import trm.ucm as ucm
import trm.sla as sla

if os.environ.has_key('ULTRACAM'):
    ultracam = os.environ['ULTRACAM']
else:
    print 'ULTRACAM environment variable must be set to point to executables.'
    exit(1)

# Predefined formats for cdd and fbb readout speeds. Ranges can probably be narrowed as experience accumulates.
predef = {}
predef['cdd'] = {'lo' : (1500.,1550.,1400.,1400.,1700.,1550.,), 'hi' : (1800., 1900., 1700., 1700.,2000.,1850.)}
predef['fbb'] = {'lo' : (1450.,1550.,1100.,1100.,1700.,1550.,), 'hi' : (1750., 1850., 1400., 1400.,2000.,1850.)}

# Get inputs
inpt = inp.Input('ULTRACAM_ENV', '.ultracam', sys.argv)

allowed = ['run%03d' % x for x in range(1,1000)]

# register parameters
inpt.register('run',      inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('source',   inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('debug',    inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('offset',   inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('first',    inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('last',     inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('format',   inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('llevels',  inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('hlevels',  inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('spread',   inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('log',      inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('bias',     inp.Input.LOCAL, inp.Input.PROMPT)

run     = inpt.get_value('run', 'the ULTRACAM run to process', 'run003', lvals=allowed)
source  = inpt.get_value('source', 'data source, s(erver) or l(ocal)?','s')
debug   = inpt.get_value('debug', 'print extra information to diagnose problems', False)
offset  = inpt.get_value('offset', 'offset in hours to subtract to get to start of night', 13.)
first   = inpt.get_value('first',  'first frame to consider', 1, 1)
last    = inpt.get_value('last',   'last frame to consider', 1000, first)
format  = inpt.get_value('format', 'automatic dectection of format?', True)
if not format:
    llevels = inpt.get_value('llevels', 'lower limits to window mean levels', (1500, 1550, 1400, 1400, 1700, 1550))
    hlevels = inpt.get_value('hlevels', 'upper limits to window mean levels', (1800, 1900, 1700, 1700, 2000, 1850))
spread = inpt.get_value('spread', 'maximum spread in window mean levels', 5.)
info   = inpt.get_value('log', 'name of file to log information on this bias', subs.Fname('bias','.log',subs.Fname.NEW))
bias   = inpt.get_value('bias', 'name of output bias frame', 'bias')

# OK run 'grab'
command = os.path.join(ultracam, 'grab')

args = (command,  run, 'source=' + source, 'ndigit=5', 'first=' + str(first), \
            'last=' + str(last), 'trim=no', 'bregion=no', 'bias=no', 'twait=1', 'tmax=1')

print '\nGrabbing frames ...'
(gout,gerr) = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

if debug:
    print gout
    print gerr

gout = gout.split('\n')

# Find what files were written
fnames = [line[8:line.find(',')]+'.ucm' for line in gout if line.startswith('Written')]
if len(fnames) == 0:
    print 'No frames were grabbed. "grab" returned:' + gerr
    exit(1)
else:
    print '\nGrabbed',len(fnames),'frames.\n'

# Accumulate the means of each window
means = []
for fname in fnames:
    ufile = ucm.rucm(fname)
    mc = []
    for nc in range(ufile.nccd()):
        mw = []
        for nw in range(ufile.nwin(nc)):
            mw.append(ufile.win(nc,nw).mean())
        mc.append(mw)
    means.append(mc)

# Process the means, regarding all windows in a given half as multiple
# samples of the same entity so that in the end we will arrive at 12 numbers,
# representing the minima and maxima of each half of the three CCDs.
means = np.array(means)

(nframe,nccd,nwin) = means.shape

lmean = means[:,:,0::2]
rmean = means[:,:,1::2]
lmin  = np.min(np.min(lmean,2),0)
rmin  = np.min(np.min(rmean,2),0)
lmax  = np.max(np.max(lmean,2),0)
rmax  = np.max(np.max(rmean,2),0)
mins  = np.c_[lmin,rmin].reshape(-1)
maxs  = np.c_[lmax,rmax].reshape(-1)

for i in range(nccd):
    print 'CCD',i+1,' left range =',lmin[i],'to',lmax[i],', right range =',rmin[i],'to',rmax[i]

# Now set the llevels and hlevels if not user-defined:
if format:
    speed   = hex(int(ufile['Instrument.Gain_Speed']['value']))[2:]
    if speed in predef:
        llevels = predef[speed]['lo']
        hlevels = predef[speed]['hi']
    else:
        print 'Read speed =',speed,'not recognised.'
        exit(1)

llevels = np.array(llevels)
hlevels = np.array(hlevels)

comb = True
if np.any(mins < llevels) or np.any(maxs > hlevels):
    print '\nSome windows had mean values that were out of range.\n'
    print 'Lowest and highest levels were defined as:'
    print ' [' + ' '.join(["%6.1f" % x for x in llevels]) + ']'
    print ' [' + ' '.join(["%6.1f" % x for x in hlevels]) + ']'
    print 'Corresponding data mininima and maxima were:'
    print ' [' + ' '.join(["%6.1f" % x for x in mins]) + ']'
    print ' [' + ' '.join(["%6.1f" % x for x in maxs]) + ']'
    print 'Tests, which should all be false were as follows:'
    print mins < llevels
    print maxs > hlevels
    comb = False
else:
    print 'All mean values were within range'

if np.any(maxs-mins > spread):
    print 'Some CCD halves had a larger spread of mean values than specified (=',spread,')'
    print 'Data mininima and maxima were:'
    print ' [' + ' '.join(["%6.1f" % x for x in mins]) + ']'
    print ' [' + ' '.join(["%6.1f" % x for x in maxs]) + ']'
    comb = False
else:
    print 'Max-Min mean of all windows was within the specified tolerance of',spread

# Gather info to log results and to generate name for the output 
# bias file. Offset is applied to always get the 'start of night'

(mjd,hour) = ufile['UT_date']['value']
if hour - offset < 0: mjd -= 1
(year,month,day,hour) = sla.djcl(mjd)
xbin = ufile.xbin
ybin = ufile.ybin

if comb:

    # write files to a disk file for 'combine'
    fstr = open('zzz_procbias.lis','w')
    for fname in fnames:
        fstr.write(fname + '\n')
    fstr.close()
        
    # OK combine the frames to make a bias
    command = os.path.join(ultracam, 'combine')
    
    rnum = run[3:]
    name = '%04d%02d%02d_r%s_%s_%dx%d_%s' % (year,month,day,rnum,speed,xbin,ybin,bias)
    args = (command,  'list=zzz_procbias.lis', 'method=c', 'sigma=3', 'careful=yes', 'adjust=b', \
                'output=' + name)

    print 'Combining frames ...'
    (cout,cerr) = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
    if debug:
        print cout
        print cerr

    cout = cout.split('\n')
    print 'All tests passed; combined bias written to ' + name

else:

    print 'One or more tests failed; no bias made.'


fout = open(info,'a')
fout.write("%d %06.3f %s %d %d %s " % (mjd,hour,run,xbin,ybin,speed)) 
for i in range(nccd):
    fout.write("%6.1f %6.1f %6.1f %6.1f " % (lmin[i],lmax[i],rmin[i],rmax[i]))
if comb:
    fout.write(name + '\n')
else:
    fout.write('No bias; values out of spec.\n')
    
fout.close()

print 'Wrote out a line of information to',info

exit(0)
