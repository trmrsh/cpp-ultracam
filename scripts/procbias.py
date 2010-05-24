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
# !!emph{procbias} is for automatic processing of ULTRACAM bias frames. It works by grabbing frames from all
# valid runs of a directory, checking them, combining the results and then checking the final frame. The checks are
# (i) there are >= 10 frames, (ii) the average values of each half of each CCD must lie within proscribed limits, 
# (iii) the average values must not vary too much during the course of a run, (iv) there is a user-defined maximum 
# number of objects permissible in the final frameo. The code ignores the first few files of drift mode biases and 
# the final frame, but otherwise will just try to read the entire bias file. The names of the bias frames are 
# automatically generated and have the form YYYYMMDD_rNNN_[speed]_[binning]_[format] where NNN gives the run number, 
# speed is the readout speed, binning is e.g. 2x2, and format is 'FFCLR' for 'full frame clear' etc. The philosophy 
# of the script is to provide fairly stiff requirements so that it is more likely for a genuine bias to fail than for 
# a different type of frame to succeed. 
#
# Warning: this script will write many files of the form run###_#####.ucm in the current working directory, and 
# will delete them, so take care not to overwrite files of interest. Information about successful biases is written
# to 'Procbias.pass'; unsuccessful ones to 'Procbias.fail'. 
#
# To try to save time, the script tries a small initial go at each run and if successful will pass on to a much larger 
# attempt. The script uses the python ucm modules and is not terribly fast. Look at 'procbias.log' for information about
# successful frames and 'procbias.err' for unsuccessful ones. Stats from the process are stored inside the 'Procbias'
# part of the headers of the output frame.
#
# The following third-party python modules are needed for this script to run: numpy, pyfits, trm.subs and trm.ucm. You must also
# have 'SExtractor' installed. This is used to search for objects in the final bias. Various temporary files will be generated 
# starting with 'zzz'. They may be useful to diagnose problems. Final bias frames
# that are deemed to have too many objects are renamed to the standard name with 'zzz_' tacked on at the front.
#
# !!head2 Invocation
#
# procbias dir [source debug nrow ncol format run] (llevels hlevels) spread thresh onebyone high frac sthresh smarea maxobj
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
# !!arg{format}{If true, the allowed mean level ranges of each window are set from defaults hard-coded in the script. 
# If false, you get the chance to define them by hand. Note that these levels vary with readout speed. Hopefully in the 
# end, the automatic option will always be best.}
# !!arg{run}{Run number of file to process, 0 for all available. Always defaults to 0 if not specified explicitly}
# !!arg{llevels}{If you chose the manual option: lowest levels of means of each window, 6 values, left & right for each CCD.}
# !!arg{hlevels}{Highest levels of means of each window, 6 values, left & right for each CCD.}
# !!arg{spread}{Maximum range between the means of each half of the CCD, 6 values left & right for each CCD. 
# The lowest and highest mean values of windows in each CCD half is used. This value should be set fairly
# tightly to spot potential problems with drift mode biases and lights being switched on.}
# !!arg{thresh}{Threshold in RMS for computation of clipped mean}
# !!arg{onebyone}{Reject pixels for clipped mean one by one or wholesale (faster but riskier)}
# !!arg{high}{Threshold in RMS for judging excess of high pixels. The idea is that if too many pixels are significantly above
# the mean, then the frames are not biases. At least 100 such pixels must be found for this to trigger.}
# !!arg{frac}{Maximum fraction of pixels above 'high' for the frames to count as biases}
# !!arg{sthresh}{RMS threshold for SExtractor for object detection.}
# !!arg{smarea}{Minimum number of pixels that must be at sthresh*RMS above background to count as an object.}
# !!arg{edge}{We ignore objects located within edge pixels of the outermost ones because these tend to be spurious and this region
# is not normally of interest. Note that this acts in addition to the nrow / ncol parameters.}
# !!arg{maxobj}{Maximum number of objects permissible, totalled over all CCDs.}
# !!table
#
# !!end

import sys, os, os.path, copy, subprocess, re
import numpy as np
import pyfits
import trm.subs as subs
import trm.subs.input as inp
import trm.ucm as ucm
import Ultra

if os.environ.has_key('ULTRACAM'):
    ultracam = os.environ['ULTRACAM']
else:
    print 'ULTRACAM environment variable must be set to point to executables.'
    exit(1)

# Check for external commands
grab       = subs.find_exec('grab', ultracam)
combine    = subs.find_exec('combine', ultracam)
sextractor = subs.find_exec('sex')

# Upper and lower limits to the means of each half of each CCD for different readout speeds.
# The ranges can probably be narrowed as experience accumulates.
predef = {}

# 2002-05 WHT run
predef['2002-05'] = {}
predef['2002-05']['fdd'] = {'lo' : (2000.,2100.,1900.,1900.,2200.,2000.,), 'hi' : (2100., 2200., 2000., 2000.,2300.,2100.)}

# 2002-09 WHT run
predef['2002-09'] = {}
predef['2002-09']['cdd'] = {'lo' : (2000.,2000.,1800.,1800.,2100.,2100.,), 'hi' : (2300., 2300., 2100., 2100.,2400.,2400.)}

# 2010 NTT run
predef['2010-01'] = {}
predef['2010-01']['cdd'] = {'lo' : (1500.,1550.,1400.,1400.,1700.,1550.,), 'hi' : (1800., 1900., 1700., 1700.,2000.,1850.)}
predef['2010-01']['fbb'] = {'lo' : (1450.,1550.,1100.,1100.,1700.,1550.,), 'hi' : (1750., 1850., 1400., 1400.,2000.,1850.)}

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
inpt.register('run',      inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('llevels',  inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('hlevels',  inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('spread',   inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('thresh',   inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('onebyone', inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('high',     inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('frac',     inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('sthresh',  inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('smarea',   inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('edge',     inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('maxobj',   inp.Input.LOCAL, inp.Input.PROMPT)

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

inpt.set_default('run', 0)
runno    = inpt.get_value('run', 'run number to analyse, 0 for all', 0)
spread   = inpt.get_value('spread', 'maximum spread in window mean levels', 5.)
thresh   = inpt.get_value('thresh', 'RMS threshold for sigma clipping', 3.) 
onebyone = inpt.get_value('onebyone', 'slow, one-by-one rejection (else fast & risky)?', True)
high     = inpt.get_value('high', 'RMS threshold for distinguishing biases', 5.)
frac     = inpt.get_value('frac', 'maximum fraction of pixels > high*RMS above mean', 1.e-4)
sthresh  = inpt.get_value('sthresh', 'SExtractor object detection threshold', 3.) 
smarea   = inpt.get_value('smarea', 'SExtractor minimum object area (pixels)', 7) 
edge     = inpt.get_value('edge', 'number of pixels distance from edge to ignore objects (pixels)', 2.) 
maxobj   = inpt.get_value('maxobj', 'maximum number of objects', 5)

# Run SExtractor to get default parameters and uncomment a few
args = (sextractor,  '-dp')
(sout,serr) = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
sout = sout.split('\n')
dfout = open('zzz_sxt.param','w')
nhead = 0
for s in sout:
    if s.startswith('#NUMBER ') or s.startswith('#X_IMAGE ') or s.startswith('#Y_IMAGE ') or \
            s.startswith('#ISOAREA_IMAGE ') or s.startswith('#X2_IMAGE ') or s.startswith('#Y2_IMAGE ') or \
            s.startswith('#XY_IMAGE ') or s.startswith('#FLUX_BEST '):
        nhead += 1
        s = s[1:]
        dfout.write(s + '\n')
dfout.close()

args = (sextractor,  '-d')
(sout,serr) = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
sout = sout.split('\n')
dfout = open('zzz_sxt.config','w')
for s in sout:
    if s.startswith('PARAMETERS_NAME'):
        dfout.write('PARAMETERS_NAME zzz_sxt.param\n')
    elif s.startswith('CATALOG_NAME'):
        dfout.write('CATALOG_NAME zzz_sxt.cat\n')
    elif s.startswith('DETECT_MINAREA'):
        dfout.write('DETECT_MINAREA 7\n')
    elif s.startswith('DETECT_THRESH'):
        dfout.write('DETECT_THRESH ' + str(sthresh) + '\n')
    elif s.startswith('FILTER'):
        dfout.write('FILTER N\n')
    else:
        dfout.write(s + '\n')
dfout.close()

# Define checking function
def check_frames(fnames, llev, hlev):
    """
    This function checks the levels and spreads of the CCDs in the frames listed
    in fnames. It returns (ok,message,tup) where ok is True or False according to
    whether the frames are thought to be from a bias, and message is a message
    string, which gives information on the outcome. 'tup' is a tuple which is
    None unless ok = True in which case it contains 6 elements representing the
    minimum and maximum means and mean RMS values of the left and right halves of 
    each CCD (3 element structures)
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
                if nw % 2 == 0:
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
                return (False,message,None)
            elif lmx > hlev[2*nc]:
                message = 'mean of left half of CCD ' + str(nc+1) + ' of ' + fname + (' is too high = %7.1f' % lmx)
                return (False,message,None)
            elif rmn < llev[2*nc+1]:
                message = 'mean of right half of CCD ' + str(nc+1) + ' of ' + fname + (' is too low = %7.1f' % rmn)
                return (False,message,None)
            elif rmx > hlev[2*nc+1]:
                message = 'mean of right half of CCD ' + str(nc+1) + ' of ' + fname + (' is too high = %7.1f' % rmx)
                return (False,message,None)
            elif nhtot > 100 and nhtot > frac*nptot:
                message = 'there were too many high pixels (' + str(nhtot) + ' out of ' + str(nptot) + ')'
                return (False,message,None)

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
                message = 'means of left half of CCD ' + str(nc+1) + ' of ' + fname + (' vary too much = %7.2f' % (lmax[nc]-lmin[nc]))
                return (False,message,None)
            elif rmax[nc]-rmin[nc] > spread:
                message = 'means of right half of CCD ' + str(nc+1) + ' of ' + fname + (' vary too much = %7.2f' % (rmax[nc]-rmin[nc]))
                return (False,message,None)

        first = False

    lmin = np.array(lmin)
    lmax = np.array(lmax)
    rmin = np.array(rmin)
    rmax = np.array(rmax)

    message = ''
    lrm = np.array(lrmn).mean(1)
    rrm = np.array(rrmn).mean(1)
    for i in range(ufile.nccd()):
        message += "%6.1f %5.2f %5.2f %6.1f %5.2f %5.2f " % ((lmin[i]+lmax[i])/2.,lmax[i]-lmin[i],lrm[i],(rmin[i]+rmax[i])/2.,rmax[i]-rmin[i],rrm[i])
    message += "%8.2e " % (float(nhtot)/nptot,)
    return (True,message,(lmin,lmax,lrm,rmin,rmax,rrm))

# OK now to work.

# First get list of all run numbers with both .dat and .xml files is existence.
if runno == 0:
    runs = [int(x[3:-4]) for x in os.listdir(dir) if x.endswith('.xml') and os.path.isfile(os.path.join(dir, x[:-4] + '.dat'))]
    runs.sort()
else:
    runs = [runno,]

# Names of pass and fail log files.
fpass = 'Procbias.pass'
ffail = 'Procbias.fail'

# Load in runs to skip from any existant procbias log and err files
skip = []
if os.path.isfile(fpass):
    fin = open(fpass)
    for line in fin:
        skip.append(line.split()[0])
    fin.close()
if os.path.isfile(ffail):
    fin = open(ffail)
    for line in fin:
        skip.append(line.split()[0])
    fin.close()

# Open log file for output
bout = open(fpass, 'a')
berr = open(ffail, 'a')

# Wind through all runs
for run in runs:

    file  = os.path.join(dir, 'run%03d' % run)
    sfile = os.path.join('%04d-%02d-%02d' % (year, month, day), 'run%03d' % run)

    # skip any run already recorded in the probias.err / .log files
    if sfile in skip:
        continue

    robj = Ultra.Run(file + '.xml', log)
    tail = ' Target: %-20s, Comment: %s\n' % (robj.target,robj.comment)

    if robj.is_not_power_onoff():

        # Compute number of junk frames
        if robj.mode == 'DRIFT':
            njunk = int((1033./int(robj.ny[0])+1.)/2.)
        elif robj.mode.endswith('CLR'):
            njunk = 0
        else:
            njunk = 1

        # Run gettime to compute number of available frames
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
            if nf < njunk+11:
                berr.write(sfile + ('%-80s' % (' error: too few frames = ' + str(nf) + '.')) + tail)
                continue
        else:
            berr.write(sfile + ('%-80s' % (' error: found no frames.')) + tail)
            continue

        # OK, have passed a few simple tests, so set 
        # the lower and upper levels of the means of the CCDs
        # Find the most recent predefined data earlier than
        # date of run.
        if format:
            keys = predef.keys()
            keys.sort(reverse=True)
            for key in keys:
                (y,m) = key.split('-')
                y = int(y)
                m = int(m)
                if y < year:
                    break
                elif y == year and m <= month:
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

        # Try a few trial frames; ignore last frame
        first = 1
        last  = min(nf-1, njunk+5)
        args = (grab,  file, 'source=' + source, 'ndigit=5', 'first=' + str(first), \
                    'last=' + str(last), 'trim=no', 'bregion=no', 'bias=no', 'twait=1', 'tmax=1')

        print '\nGrabbing frames',first+njunk,'to',last,'from',file
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

        (ok,message, stats) = check_frames(fnames, llev, hlev)
        if ok:

            # Passed first few OK, lets grab some more
            first = last+1
            last  = min(nf-1, 300)
            if first <= last:
                args = (grab,  file, 'source=' + source, 'ndigit=5', 'first=' + str(first), \
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

            (ok,message,stats) = check_frames(fnames, llev, hlev)

            if ok:
                # having got all this way with checks, now we combine the frames
                fstr = open('zzz_procbias.lis','w')
                for fname in fnames:
                    fstr.write(fname + '\n')
                fstr.close()
                nframe = len(fnames)

                name = '%04d%02d%02d_r%03d_%s_%sx%s_%s' % (year,month,day,run,robj.speed,robj.x_bin,robj.y_bin,robj.mode)
                args = (combine,  'list=zzz_procbias.lis', 'method=c', 'sigma=3', 'careful=yes', 'adjust=b', 'output=' + name)

                print 'Combining frames ...'
                (cout,cerr) = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
                if debug:
                    print cout
                    print cerr

                # Now have a combined frame. Last thing to try is to detect objects that might have been
                # too feeble to show up in individual images. Do this using SExtractor for which we need
                # to write temporary FITS files for each window.
                result = ucm.rucm(name + '.ucm')

                # this is where we store temporary FITS files. /dev/shm for virtual memory.
                tmpfile = '/dev/shm/zzz_sxt.fits'

                # now wind through all the windows.
                nobj = np.zeros(result.nccd(),int)
                for nc in range(result.nccd()):
                    for nw in range(result.nwin(nc)):
                        if nw % 2 == 0:
                            win = result.win(nc,nw)[nrow:,ncol:]
                        else:
                            win = result.win(nc,nw)[nrow:,:-ncol]

                        # Median collapse in Y- then X-directions, subtract result. This to get rid of gradients.
                        medy = np.median(win,0)
                        win -= medy
                        medx = np.median(win,1)
                        win -= np.c_[win.shape[1]*[medx]].transpose()

                        if debug:
                            print 'Median X-profile =',medy                        
                            print 'Median Y-profile =',medx                        

                        # Write out to a FITS file
                        if os.path.isfile(tmpfile):
                            os.remove(tmpfile)
                        hdu  = pyfits.PrimaryHDU(win)
                        head = hdu.header
                        head.update('GAIN', 1.1*nframe, 'electrons/ADU')
                        hdu.writeto(tmpfile)

                        # Run SExtractor on the temporary FITS file
                        args = (sextractor,  '-c', 'zzz_sxt.config', tmpfile)
                        p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                        sout,serr = p.communicate()
                        
                        # First nhead lines are just what we choose to output
                        # Here we count objects ignoring any within edge pixels of the outermost
                        # ring of pixels.
                        (ny,nx) = win.shape
                        fin = open('zzz_sxt.cat')
                        nline = 0
                        for line in fin:
                            nline += 1
                            if nline > nhead:
                                (num,flux,area,x,y,sigx,sigy,sigxy) = line.split()
                                x = float(x)
                                y = float(y)
                                if x > 1+edge and x < nx-edge and y > 1+edge and y < ny-edge: 
                                    nobj[nc] += 1
                        fin.close()
                        
                        if debug:
                            print sout
                            print serr
                   
                # Update header of frame with results and parameters used during the procbias run.
                modf = ucm.rucm(name + '.ucm')
                lmin,lmax,lrm,rmin,rmax,rrm = stats
                nums = np.array([float(fname[7:12]) for fname in fnames])
                modf['Procbias'] = {'comment': 'procbias information', 'type' : ucm.ITYPE_DIR, 'value': None}
                modf['Procbias.nframe'] = {'comment': 'number of contributing frames', 'type' : ucm.ITYPE_INT, 'value': nframe}
                modf['Procbias.range']  = {'comment': 'range of contributing frame numbers', 'type' : ucm.ITYPE_IVECTOR, 'value': (nums.min(),nums.max())}
                modf['Procbias.nobj']   = {'comment': 'number of objects found in each CCD', 'type' : ucm.ITYPE_IVECTOR, 'value': nobj}
                modf['Procbias.lmid']   = {'comment': 'mid-ranges of mean values of left-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': (lmin+lmax)/2.}
                modf['Procbias.lrng']   = {'comment': 'ranges of mean values of left-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': lmax-lmin}
                modf['Procbias.lrms']   = {'comment': 'mean RMS of left-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': lrm}
                modf['Procbias.rmid']   = {'comment': 'mid-ranges of mean values of right-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': (rmin+rmax)/2.}
                modf['Procbias.rrng']   = {'comment': 'ranges of mean values of right-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': rmax-rmin}
                modf['Procbias.rrms']   = {'comment': 'mean RMS of right-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': rrm}
                modf['Procbias.Param']  = {'comment': 'procbias parameters', 'type' : ucm.ITYPE_DIR, 'value': None}
                modf['Procbias.Param.nrow']  = {'comment': 'number of rows to ignore at bottom', 'type' : ucm.ITYPE_INT, 'value': nrow}
                modf['Procbias.Param.ncol']  = {'comment': 'number of columns to ignore at sides', 'type' : ucm.ITYPE_INT, 'value': ncol}
                modf['Procbias.Param.llevels']  = {'comment': 'lower limits on mean levels', 'type' : ucm.ITYPE_FVECTOR, 'value': llev}
                modf['Procbias.Param.hlevels']  = {'comment': 'upper limits on mean levels', 'type' : ucm.ITYPE_FVECTOR, 'value': hlev}
                modf['Procbias.Param.spread']   = {'comment': 'maximum spread in mean levels', 'type' : ucm.ITYPE_FLOAT, 'value': spread}
                modf['Procbias.Param.thresh']   = {'comment': 'sigma clipping threshold', 'type' : ucm.ITYPE_FLOAT, 'value': thresh}
                modf['Procbias.Param.onebyone'] = {'comment': 'reject pixels one by one?', 'type' : ucm.ITYPE_BOOL, 'value': onebyone}
                modf['Procbias.Param.high']     = {'comment': 'RMS threshold for a pixel to count as high', 'type' : ucm.ITYPE_FLOAT, 'value': thresh}
                modf['Procbias.Param.frac']     = {'comment': 'maximum fraction of high pixels', 'type' : ucm.ITYPE_FLOAT, 'value': frac}
                modf['Procbias.Param.sthresh']  = {'comment': 'SExtractor object detection threshold', 'type' : ucm.ITYPE_FLOAT, 'value': sthresh}
                modf['Procbias.Param.smarea']   = {'comment': 'SExtractor minimum pixel area', 'type' : ucm.ITYPE_INT, 'value': smarea}
                modf['Procbias.Param.edge']     = {'comment': 'edge strip to ignore objects (pixels)', 'type' : ucm.ITYPE_FLOAT, 'value': edge}
                modf['Procbias.Param.maxobj']   = {'comment': 'maximum total number of objects', 'type' : ucm.ITYPE_INT, 'value': maxobj}
                modf.write(name + '.ucm')

                # Apply object number test
                if nobj.sum() > maxobj:
                    berr.write(sfile + ('%-80s' % (' error: ' + 'detected ' + str(nobj) + ' objects in the combined frame.')) + tail)
                    os.rename(name + '.ucm', 'zzz_' + name + '.ucm')
                else:
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


