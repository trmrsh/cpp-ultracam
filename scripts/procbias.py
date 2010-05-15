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
# !!emph{procbias} is for semi-automatic processing of ULTRACAM bias frames. It works by grabbing frames from all
# valid runs of a directory, checking them for consistency and then combining the results. Consistency means that the average values
# of each half of each CCD must lie within proscribed limits and not vary by too much during the course of 
# a run. The bias frames are dumped in whichever directory this is run in. The code knows to ignore the first
# few files of drift mode biases but otherwise will just try ro read the entire bias file. The names of the bias
# frames are automatically generated and have the form YYYYMMDD_rMMM_[speed]_[binning]_[format] where MMM gives
# the run number, speed is the readout speed, binning in e.g. 2x2, and format is 'ff' for 'full frame' etc.
#
# Warning: this script will write many files of the form run###_#####.ucm in the current working directory, and 
# will delete them, so take care not to overwrite files of interest. Information about successful biases is written
# to 'procbias.log'; unsuccessful one to 'procbias.err'. 
#
# To try to save time, the script tries a small initial go at each run and if successful will pass on to a much larger 
# attempt.
#
# !!head2 Invocation
#
# procbias dir [source debug nrow ncol format] (llevels hlevels) spread thresh onebyone high frac
#
# !!head2 Arguments
#
# !!table
#
# !!arg{dir}{Path to directory containing bias runs. 
# The directory must end with a date of the form YYYY[_-]MM[_-]DD or. This is used to generate automatic names for the resulting
# biases and is assumed to be the date of the start of the night. Should also contain a ASCII log file from which comments will be grabbed. 
# It assumed that this is of the form YYYY-MM-DD.dat}
# !!arg{source}{data source, 's' for server, 'l' for local}
# !!arg{debug}{print extra information to diagnose problems}
# !!arg{nrow}{Number of rows to trim off bottom}
# !!arg{ncol}{Number of columns to trim off outer edges of windows}
# !!arg{format}{If true, the allowed mean level ranges of each window are set by the code itself. If false, you get the chance
# to define them by hand. Note that these levels vary with readout speed. The automatic option has a set of
# recognised formats for which values have been set.}
# !!arg{llevels}{Lowest levels of means of each window, 6 values, left & right for each CCD. These are designed to spot
# bias problems (too low, too high). Unfortunately the correct values depend upon the binning.}
# !!arg{hlevels}{Highest levels of means of each window, 6 values, left & right for each CCD.}}
# !!arg{spread}{Maximum range between the means of each half of the CCD, 6 values left & right for each CCD. 
# The lowest and highest mean values of windows in each CCD half is used. This value should be set fairly
# tightly to spot potential problems with drift mode biases and lights being switched on.}
# !!arg{thresh}{Threshold in RMS for computation of clipped mean}
# !!arg{onebyone}{Reject pixels for clipped mean one by one or wholesale (faster but riskier)}
# !!arg{high}{Threshold in RMS for judging excess of high pixels. The idea is that if too many pixels are significantly above
# the mean, then the frames are not biases. At least 100 such pixels must be found for this to trigger.}
# !!arg{frac}{Maximum fraction of pixels above 'high' for the frames to count as biases}
#
# !!table
# !!end

import sys, os, os.path, copy, subprocess, re
import numpy as np
import trm.subs as subs
import trm.subs.input as inp
import trm.ucm as ucm
import Ultra

if os.environ.has_key('ULTRACAM'):
    ultracam = os.environ['ULTRACAM']
else:
    print 'ULTRACAM environment variable must be set to point to executables.'
    exit(1)

# Predefined formats for cdd and fbb readout speeds. Ranges can probably be narrowed as experience accumulates.
predef = {}
predef[2002] = {}
predef[2002]['cdd'] = {'lo' : (2200.,2250.,2100.,2150.,2400.,2300.,), 'hi' : (2300., 2350., 2200., 2250.,2500.,2400.)}
predef[2002]['fbb'] = {'lo' : (2500.,2550.,2200.,2200.,2750.,2700.,), 'hi' : (2800., 2850., 2500., 2500.,3050.,3000.)}
predef[2010] = {}
predef[2010]['cdd'] = {'lo' : (1500.,1550.,1400.,1400.,1700.,1550.,), 'hi' : (1800., 1900., 1700., 1700.,2000.,1850.)}
predef[2010]['fbb'] = {'lo' : (1450.,1550.,1100.,1100.,1700.,1550.,), 'hi' : (1750., 1850., 1400., 1400.,2000.,1850.)}

# Get inputs
inpt = inp.Input('ULTRACAM_ENV', '.ultracam', sys.argv)

allowed = ['run%03d' % x for x in range(1,1000)]

# register parameters
inpt.register('dir',      inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('source',   inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('debug',    inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('nrow',     inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('ncol',     inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('format',   inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('llevels',  inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('hlevels',  inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('spread',   inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('thresh',   inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('onebyone', inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('high',     inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('frac',     inp.Input.LOCAL, inp.Input.PROMPT)

dir = inpt.get_value('dir', 'directory containing runs', '2010_04_28')
if not os.path.isdir(dir):
    print dir,'is not a directory.'
    exit(1)

dre = re.compile('(\d\d\d\d)-(\d\d)-(\d\d)/?$')
m = dre.search(dir)
if m:
    year  = int(m.group(1))
    month = int(m.group(2))
    day   = int(m.group(3))
else:
    print "procbias only recognises directories ending with the date in YYYY-MM-DD form"
    exit(1)

# Generate the name of the ASCII comments file, check and then read it.
ascii = os.path.join(dir, '%04d-%02d-%02d.dat' % (year,month,day))
if not os.path.isfile(ascii):
    print 'Could not find ASCII log file =',ascii
    exit(1)
log = Ultra.Log(ascii)

# these parameters are normally hidden
source  = inpt.get_value('source', 'data source, s(erver) or l(ocal)?','s')
debug   = inpt.get_value('debug', 'print extra information to diagnose problems', False)
nrow    = inpt.get_value('nrow', 'number of rows to trim from bottom of windows', 1)
ncol    = inpt.get_value('ncol', 'number of columns to trim from outer edges of windows', 1)
format  = inpt.get_value('format', 'automatic dectection of format?', True)
if not format:
    llevels = inpt.get_value('llevels', 'lower limits to window mean levels', (1500, 1550, 1400, 1400, 1700, 1550))
    hlevels = inpt.get_value('hlevels', 'upper limits to window mean levels', (1800, 1900, 1700, 1700, 2000, 1850))

spread   = inpt.get_value('spread', 'maximum spread in window mean levels', 5.)
thresh   = inpt.get_value('thresh', 'RMS threshold for sigma clipping', 3.) 
onebyone = inpt.get_value('onebyone', 'slow, one-by-one rejection (else fast & risky)?', True)
high     = inpt.get_value('high', 'RMS threshold for distinguishing biases', 5.)
frac     = inpt.get_value('frac', 'maximum fraction of pixels > high*RMS above mean', 1.e-4)

# Define checking function
def check_frames(fnames, llev, hlev):
    """
    This function checks the levels and spreads of the CCDs in the frames listed
    in fnames. It returns (ok,message) where ok is True or False according to
    whether the frames are thought to be from a bias, and message is a message
    string, which gives
    """

    # Work out minimum and maximum means of each half of each CCD. Try to 
    # halt as soon as possible when values go out of range
    lmin = []
    lmax = []
    rmin = []
    rmax = []
    lrmn = []
    rrmn = []
    first = True
    nptot = 0
    nhtot = 0
    for fname in fnames:
        ufile = ucm.rucm(fname)
        mc = []
        for nc in range(ufile.nccd()):
            mw = []
            rw = []
            for nw in range(ufile.nwin(nc)):
                if nw % 2:
                    win = ufile.win(nc,nw)[nrow:,ncol:]
                else:
                    win = ufile.win(nc,nw)[nrow:,:-ncol]

                # Collapse in Y- then X-directions, subtract result. This to get rid of gradients.
                medy = np.median(win,0)
                win -= medy
                medx = np.median(win,1)
                win -= np.c_[win.shape[1]*[medx]].transpose()

                # Sigma-clipped mean
                (rmean,rrms,cmean,crms,nrej,ncyc) = subs.sigma_reject(win, thresh, onebyone)
                mw.append(cmean+medx.mean()+medy.mean())
                rw.append(crms)
                nptot += np.size(win)
                nhtot += len(win[(win - cmean) > high*crms])

            lm   = np.array(mw[0::2])
            rm   = np.array(mw[1::2])
            lr   = np.array(rw[0::2]).mean()
            rr   = np.array(rw[1::2]).mean()
            lmn  = lm.min()
            lmx  = lm.max()
            rmn  = rm.min()
            rmx  = rm.max()
            
            if lmn < llev[2*nc]:
                message = 'mean of left half of CCD ' + str(nc+1) + ' of ' + fname + (' is too low = %7.1f' % lmn)
                return (False,message)
            elif lmx > hlev[2*nc]:
                message = 'mean of left half of CCD ' + str(nc+1) + ' of ' + fname + (' is too high = %7.1f' % lmx)
                return (False,message)
            elif rmn < llev[2*nc+1]:
                message = 'mean of right half of CCD ' + str(nc+1) + ' of ' + fname + (' is too low = %7.1f' % rmn)
                return (False,message)
            elif rmx > hlev[2*nc+1]:
                message = 'mean of right half of CCD ' + str(nc+1) + ' of ' + fname + (' is too high = %7.1f' % rmx)
                return (False,message)
            elif nhtot > 100 and nhtot > frac*nptot:
                message = 'there were too many high pixels (' + str(nhtot) + ' out of ' + str(nptot) + ')'
                return (False,message)

            if first:
                lmin.append(lmn)
                lmax.append(lmx)
                rmin.append(rmn)
                rmax.append(rmx)
                lrmn.append([])
                rrmn.append([])
            else:
                lmin[nc] = min(lmn, lmin[nc])
                lmax[nc] = max(lmx, lmax[nc])
                rmin[nc] = min(rmn, rmin[nc])
                rmax[nc] = max(rmx, rmax[nc])
            lrmn[nc].append(lr)
            rrmn[nc].append(rr)

            if lmax[nc]-lmin[nc] > spread:
                message = 'means of left half of CCD ' + str(nc+1) + ' of ' + fname + (' vary too much = %7.1f' % (lmax[nc]-lmin[nc]))
                return (False,message)
            elif rmax[nc]-rmin[nc] > spread:
                message = 'means of right half of CCD ' + str(nc+1) + ' of ' + fname + (' vary too much = %7.1f' % (rmax[nc]-rmin[nc]))
                return (False,message)

        first = False

    message = ''
    for i in range(ufile.nccd()):
        lrm = np.array(lrmn[i]).mean()
        rrm = np.array(rrmn[i]).mean()
        message += "%6.1f %6.1f %5.2f %6.1f %6.1f %5.2f " % (lmin[i],lmax[i],lrm,rmin[i],rmax[i],rrm)
    message += "%8.2e " % (float(nhtot)/nptot,)
    return (True,message)

# OK now to work.

# First get list of all run numbers with both .dat and .xml files is existence.
runs = [int(x[3:-4]) for x in os.listdir(dir) if x.endswith('.xml') and os.path.isfile(os.path.join(dir, x[:-4] + '.dat'))]
runs.sort()

# Open log file for output
bout = open('procbias.log','a')
berr = open('procbias.err','a')

# Wind through all runs
for run in runs:

    file  = os.path.join(dir, 'run%03d' % run)
    sfile = os.path.join('%04d-%02d-%02d' % (year, month, day), 'run%03d' % run)

    robj = Ultra.Run(file + '.xml', log)
    tail = ' Target: %-20s, Comment: %s\n' % (robj.target,robj.comment)

    if robj.is_not_power_onoff():

        # Compute number of junk frames
        if robj.mode == 'DRIFT':
            njunk = int((1033./int(robj.ny[0])+1.)/2.)
        else:
            njunk = 1

        # Run gettime then apply a few tests
        command = os.path.join(ultracam, 'gettime')
        args = (command, file)
        (gout,gerr) = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
        if debug:
            print gout
            print gerr

        nf = re.compile('Number of frames = (\d+)')
        m = nf.search(gout)
        if m:
            nf = int(m.group(1))
            if nf < njunk+10:
                berr.write(sfile + ('%-80s' % (' error: too few frames = ' + str(nf) + '.')) + tail)
                continue
        else:
            berr.write(sfile + ('%-80s' % (' error: found no frames.')) + tail)
            continue

        ed = re.compile('Exposure delay = ([\.\d]+)')
        m  = ed.search(gout)
        if m:
            ed = float(m.group(1))
            if ed > 0.5:
                berr.write(sfile + ('%-80s' % (' error: exposure delay = ' + str(ed) + ' is too large.')) + tail)

        else:
            berr.write(sfile + ('%-80s' % (' error: could not find exposure delay.')) + tail)
            continue

        # OK, have passed a few simple tests, so set 
        # the lower and upper levels of the means of the CCDs
        if format:
            keys = predef.keys()
            keys.sort()
            for key in keys:
                if key < year:
                    break
            if robj.speed in predef[key]:
                llev = np.array(predef[key][robj.speed]['lo'])
                hlev = np.array(predef[key][robj.speed]['hi'])
            else:
                print 'Read speed =',robj.speed,'not recognised.'
                exit(1)
        else:
            llev = np.array(llevels)
            hlev = np.array(hlevels)

        # Try a few trial frames
        command = os.path.join(ultracam, 'grab')

        first = 1
        last  = min(nf, njunk+10)
        args = (command,  file, 'source=' + source, 'ndigit=5', 'first=' + str(first), \
                    'last=' + str(last), 'trim=no', 'bregion=no', 'bias=no', 'twait=1', 'tmax=1')

        print '\nGrabbing frames',first,'to',last,'from',file
        (gout,gerr) = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

        if debug:
            print gout
            print gerr

        gout = gout.split('\n')

        # Find what files were written
        fnames = [line[8:line.find(',')]+'.ucm' for line in gout if line.startswith('Written')]
        if len(fnames) == 0:
            berr.write(sfile + ('%-80s' % (' error: no frames were grabbed.')) + tail)
            continue
        else:
            print 'Grabbed',len(fnames),'frames from',file

        print 'Checking their mean values.'

        (ok,message) = check_frames(fnames, llev, hlev)
        if ok:

            # Passed first few OK, lets grab some more
            first = last+1
            last  = min(nf, 300)
            if first <= last:
                args = (command,  file, 'source=' + source, 'ndigit=5', 'first=' + str(first), \
                            'last=' + str(last), 'trim=no', 'bregion=no', 'bias=no', 'twait=1', 'tmax=1')

                print 'First few frames were OK, so now will grab frames',first,'to',last,'from',file
                (gout,gerr) = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

                if debug:
                    print gout
                    print gerr

                gout = gout.split('\n')

                # Find what files were written
                extra = [line[8:line.find(',')]+'.ucm' for line in gout if line.startswith('Written')]
                if len(extra) == 0:
                    berr.write(sfile + ('%-80s' % (' error: failed to grab any additional frames.')) + tail)
                    continue
                else:
                    print 'Grabbed another',len(extra),'frames from',file
                    fnames += extra

            print 'Checking all mean values.'

            (ok,message) = check_frames(fnames, llev, hlev)

            if ok:
                # having got all this way with checks, now we combine the frames
                fstr = open('zzz_procbias.lis','w')
                for fname in fnames:
                    fstr.write(fname + '\n')
                fstr.close()
        
                command = os.path.join(ultracam, 'combine')
            
                name = '%04d%02d%02d_r%03d_%s_%sx%s_%s' % (year,month,day,run,robj.speed,robj.x_bin,robj.y_bin,robj.mode)
                args = (command,  'list=zzz_procbias.lis', 'method=c', 'sigma=3', 'careful=yes', 'adjust=b', 'output=' + name)

                print 'Combining frames ...'
                (cout,cerr) = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
                if debug:
                    print cout
                    print cerr
                print 'All tests passed; combined bias written to ' + name

                bout.write("%s %sx%s %s " % (sfile,robj.x_bin,robj.y_bin,robj.speed)) 
                bout.write(message)
                bout.write(('%-30s' % name) + tail)
            else:
                berr.write(sfile + ('%-80s' % (' error: ' + message + '.')) + tail)
        else:
            berr.write(sfile + ('%-80s' % (' error: ' + message + '.')) + tail)

        # Delete frames unless debug in operation
        if not debug:
            for fname in fnames:
                os.remove(fname)
   
# Pack up and leave 
bout.close()
berr.close()
exit(0)


