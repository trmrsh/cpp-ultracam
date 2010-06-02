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
# !!emph{procbias} is for automatic processing of ULTRACAM bias frames. It
# works by grabbing frames from all valid runs of a directory, checking them,
# combining the results and then checking the final frame. The checks are (i)
# there are >= 10 frames, (ii) the average values of each half of each CCD
# must lie within proscribed limits, (iii) the average values must not vary
# too much during the course of a run, (iv) there is a user-defined maximum
# number of objects permissible in the final frame as detected by
# SExtractor. The code ignores the first few files of drift mode biases and
# the final frame, but otherwise will just try to read the entire bias
# file. The names of the bias frames are automatically generated and have the
# form YYYYMMDD_rNNN_[speed]_[binning]_[format] where NNN gives the run
# number, speed is the readout speed, binning is e.g. 2x2, and format is
# 'FFCLR' for 'full frame clear' etc. The philosophy of the script is to
# provide fairly stiff requirements so that it is more likely for a genuine
# bias to fail than for a different type of frame to succeed.
#
# Warning: this script will write many files of the form run###_#####.ucm in
# the current working directory, and will delete them, so take care not to
# overwrite files of interest. Information about successful biases is written
# to 'Gold.html' and 'Silver.hgtml'; unsuccessful ones to 'Failed.html'. Do not move these
# files; they are used to save time in future runs by skipping whichever runs 
# are listed within them. If you want to have another go at a run, it must be
# removed from these files.
#
# To try to save time, the script tries a small initial go at each run and if
# successful will pass on to a much larger attempt. The script uses the python
# ucm modules and is not terribly fast. Look at 'Gold.html' for information
# about successful frames, 'Silver.html' for partially successful ones,  
# and 'Failed.html' for thoose that did not make the grade. Stats from
# the process are stored inside the 'Procbias' part of the headers of the
# output frame. Various temporary files will be generated starting with
# 'zzz'. They may be useful to diagnose problems. Final bias frames that are
# deemed to have too many objects are renamed to the standard name with 'zzz_'
# tacked on at the front.
#
# !!emph{procbias} is a bit of an expert-user script and needs a fair few other
# bits of software installed in order to run, beyond what is required for the
# default pipeline installation. Third-party python modules numpy, pyfits,
# trm.subs and trm.ucm are required, and you must also have 'SExtractor'
# installed. This is used to search for objects in the final bias. 
#
# !!head2 Invocation
#
# procbias dir [source debug nrow ncol run maxfrm] sgold ssilver thresh high frac sthresh smarea edge regexp
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
# !!arg{run}{Run number of file to process, 0 for all available. Always defaults to 0 if not specified explicitly}
# !!arg{maxfrm}{Maximum number of frames to attempt to combine. This is to ensure that 'combine' runs successfully. 
# You may need to experiment to establish a good value for this. Once set, it should rarely need changing.}
# !!arg{sgold}{Maximum spread between the means of each half of the CCD, 6 values left & right for each CCD for a given run.
# The lowest and highest mean values of windows in each CCD half is used. This one is for the 'gold standard' biases and 
# should be set to about 4 as the majority of good biases show little variation.}
# !!arg{ssilver}{Maximum spread between the means of each half of the CCD, 6 values left & right for each CCD for a given run.
# The lowest and highest mean values of windows in each CCD half is used. This one is for the 'silver standard' biases and 
# should be set to perhaps 10 to pick up some near misses.}
# !!arg{thresh}{Threshold in RMS for computation of clipped mean}
# !!arg{high}{Threshold in RMS for judging excess of high pixels. The idea is that if too many pixels are significantly above
# the mean, then the frames are not biases. At least 100 such pixels must be found for this to trigger.}
# !!arg{frac}{Maximum fraction of pixels above 'high' for the frames to count as biases. This is affected by dark
# pixels so should not be set too low. }
# !!arg{sthresh}{RMS threshold for SExtractor for object detection.}
# !!arg{smarea}{Minimum number of pixels that must be at sthresh*RMS above background to count as an object.}
# !!arg{edge}{We ignore objects located within edge pixels of the outermost ones because these tend to be spurious and this region
# is not normally of interest. Note that this acts in addition to the nrow / ncol parameters.}
# !!arg{regexp}{Regular expression which if it is a match is used to kick frames out. The main confusing types are
# noise tests and junk frames so 'noise|junk' may be a good choice. The match is carried out case insensitively.}
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

# Target mid-range values and half variability ranges to the means of each half of each CCD for different readout speeds.
# Variation with binning probably means that the ranges should not be lowered much below 200
predef = {}

predef['2002-05'] = {}
predef['2002-05']['fdd'] = {'mrange' : (2060.,2115.,1966.,1948.,2245.,2046.,), 'range' : (200., 200., 200., 200.,200.,200.)}

predef['2002-09'] = {}
predef['2002-09']['cdd'] = {'mrange' : (2120., 2180., 1985., 2034., 2278., 2193.), 'range' : (200., 200., 200., 200.,200.,200.)}

predef['2003-10'] = {}
predef['2003-10']['cdd'] = {'mrange' : (2240.,2300.,2148.,2213.,2455.,2356.), 'range' : (200., 200., 200., 200.,200.,200.)}
predef['2003-10']['fdd'] = {'mrange' : (2240.,2300.,2148.,2213.,2455.,2356.), 'range' : (400., 400., 400., 400.,400.,400.)}
predef['2003-10']['fbb'] = {'mrange' : (2650.,2710.,2348.,2332.,2890.,2830.), 'range' : (200., 200., 200., 200.,200.,200.)}

predef['2007-06'] = {}
predef['2007-06']['cdd'] = {'mrange' : (1642.,1702.,1535.,1570.,1840.,1688.), 'range' : (200., 200., 200., 200., 200., 200.)}
predef['2007-06']['fbb'] = {'mrange' : (1642.,1702.,1535.,1570.,1840.,1688.), 'range' : (200., 200., 200., 200., 200., 200.)}

# Get inputs
inpt = inp.Input('ULTRACAM_ENV', '.ultracam', sys.argv)

allowed = ['run%03d' % x for x in range(1,1000)]
# register parameters
inpt.register('dir',      inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('source',   inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('debug',    inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('nrow',     inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('ncol',     inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('run',      inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('maxfrm',   inp.Input.LOCAL, inp.Input.HIDE)
inpt.register('sgold',    inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('ssilver',  inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('thresh',   inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('high',     inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('frac',     inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('sthresh',  inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('smarea',   inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('edge',     inp.Input.LOCAL, inp.Input.PROMPT)
inpt.register('regexp',   inp.Input.LOCAL, inp.Input.PROMPT)

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
inpt.set_default('run', 0)
runno    = inpt.get_value('run', 'run number to analyse, 0 for all', 0)
maxfrm   = inpt.get_value('maxfrm', 'maximum number of frames to attempt combining', 4000)

# these are always prompted
sgold    = inpt.get_value('sgold', 'maximum spread in window mean levels, gold class', 4.)
ssilver  = inpt.get_value('ssilver', 'maximum spread in window mean levels, silver class', 10.)
thresh   = inpt.get_value('thresh', 'RMS threshold for sigma clipping', 3.) 
high     = inpt.get_value('high', 'RMS threshold for distinguishing biases', 5.)
frac     = inpt.get_value('frac', 'maximum fraction of pixels > high*RMS above mean', 1.e-4)
sthresh  = inpt.get_value('sthresh', 'SExtractor object detection threshold', 3.) 
smarea   = inpt.get_value('smarea', 'SExtractor minimum object area (pixels)', 7) 
edge     = inpt.get_value('edge', 'number of pixels distance from edge to ignore objects (pixels)', 2.) 
regexp   = inpt.get_value('regexp', 'regular expression for picking up other frames to chuck out', 'noise|junk')

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
def check_frames(fnames, llev, hlev, spread):
    """
    This function checks the levels and spreads of the CCDs in the frames listed
    in fnames. It returns (ok,message,tup) where ok is True or False according to
    whether the frames are thought to be from a bias, and message is a message
    string, which gives information on the outcome. 'tup' is a tuple which is
    None unless ok = True in which case it contains 6 elements representing the
    minimum and maximum means and mean RMS values of the left and right halves of 
    each CCD (3 element structures). 'llev' and 'hlev' are the lower and upper
    allowed mean values of each half of each CCD (6 elements)
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
                (rmean,rrms,cmean,crms,nrej,ncyc) = subs.sigma_reject(win, thresh, False)
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
                message = 'Mean of left half of CCD ' + str(nc+1) + ' of ' + fname + ' = %7.1f is low by %7.1f' % (lmn,llev[2*nc]-lmn)
                return (False,message,None)
            elif lmx > hlev[2*nc]:
                message = 'Mean of left half of CCD ' + str(nc+1) + ' of ' + fname + ' = %7.1f is high by %7.1f' % (lmx, hlev[2*nc]-lmx)
                return (False,message,None)
            elif rmn < llev[2*nc+1]:
                message = 'Mean of right half of CCD ' + str(nc+1) + ' of ' + fname + ' = %7.1f is low by %7.1f' % (rmn,llev[2*nc]-rmn)
                return (False,message,None)
            elif rmx > hlev[2*nc+1]:
                message = 'Mean of right half of CCD ' + str(nc+1) + ' of ' + fname + ' = %7.1f is high by %7.1f' % (rmx, hlev[2*nc]-rmx)
                return (False,message,None)
            elif nhtot > 100 and nhtot > frac*nptot:
                message = 'There were too many high pixels: ' + str(nhtot) + ' out of ' + str(nptot) + ', a fraction = %8.2e' % (nhtot/float(nptot),)
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
                message = 'Means of left half of CCD ' + str(nc+1) + ' of ' + fname + (' vary too much = %7.2f' % (lmax[nc]-lmin[nc]))
                return (False,message,None)
            elif rmax[nc]-rmin[nc] > spread:
                message = 'Means of right half of CCD ' + str(nc+1) + ' of ' + fname + (' vary too much = %7.2f' % (rmax[nc]-rmin[nc]))
                return (False,message,None)

        first = False

    if nptot == 0:
        return (False,'no valid pixels',None)

    lmin = np.array(lmin)
    lmax = np.array(lmax)
    rmin = np.array(rmin)
    rmax = np.array(rmax)

    message = ''
    lrm = np.array(lrmn).mean(1)
    rrm = np.array(rrmn).mean(1)
    for i in range(ufile.nccd()):
        message += "<td>%6.1f</td><td>%5.2f</td><td>%5.2f</td><td>%6.1f</td><td>%5.2f</td><td>%5.2f</td>" \
            % ((lmin[i]+lmax[i])/2.,lmax[i]-lmin[i],lrm[i],(rmin[i]+rmax[i])/2.,rmax[i]-rmin[i],rrm[i])
    message += "<td>%8.2e</td>" % (float(nhtot)/nptot,)
    return (True,message,(lmin,lmax,lrm,rmin,rmax,rrm))

# OK now to work.

# First get list of all run numbers with both .dat and .xml files is existence.
if runno == 0:
    runs = [int(x[3:-4]) for x in os.listdir(dir) if x.endswith('.xml') and os.path.isfile(os.path.join(dir, x[:-4] + '.dat'))]
    runs.sort()
else:
    runs = [runno,]

# Names of best, second-best and failed files
fgold    = 'Gold.html'
fsilver  = 'Silver.html'
ffailed  = 'Failed.html'
fbfailed = 'FailedBiases.html'

# Load in runs to skip from any existant procbias log and err files
skip = []

gold = []
if os.path.isfile(fgold):
    fin = open(fgold)
    for line in fin:
        if line.startswith('<tr><td><a href="2'):
            gold.append(line)
            yy = int(line[17:21])
            mm = int(line[21:23])
            dd = int(line[23:25])
            nr = int(line[27:30])
            skip.append(os.path.join('%04d-%02d-%02d' % (yy, mm, dd), 'run%03d' % nr))
    fin.close()
    print 'Loaded',len(gold),'gold-standard biases which will not be recreated.'

silver = []
if os.path.isfile(fsilver):
    fin = open(fsilver)
    for line in fin:
        if line.startswith('<tr><td><a href="2'):
            silver.append(line)
            yy = int(line[17:21])
            mm = int(line[21:23])
            dd = int(line[23:25])
            nr = int(line[27:30])
            skip.append(os.path.join('%04d-%02d-%02d' % (yy, mm, dd), 'run%03d' % nr))
    fin.close()
    print 'Loaded',len(silver),'silver-standard biases which will not be recreated.'

failed = []
if os.path.isfile(ffailed):
    lsold = len(skip)
    fin = open(ffailed)
    for line in fin:
        if line.startswith('<tr><td>2'):
            failed.append(line)
            yy = int(line[8:12])
            mm = int(line[13:15])
            dd = int(line[16:18])
            nr = int(line[22:25])
            skip.append(os.path.join('%04d-%02d-%02d' % (yy, mm, dd), 'run%03d' % nr))
    fin.close()
    print 'Loaded',len(failed),'runs that failed the bias tests; these will not be re-attempted.'

try:
    print 'Working on directory =',dir
    # Wind through all runs
    rect = re.compile(regexp, re.I)
    for run in runs:

        file  = os.path.join(dir, 'run%03d' % run)
        sfile = os.path.join('%04d-%02d-%02d' % (year, month, day), 'run%03d' % run)

        # skip any run already recorded in the probias.err / .log files
        if sfile in skip:
            continue
        sfile = '<td>' + sfile + '</td>'

        robj = Ultra.Run(file + '.xml', log)
        tail = '<td nowrap>%s</td><td nowrap>%s</td></tr>\n' % (robj.target,robj.comment)
        if robj.target is not None and rect.search(robj.target):
            failed.append('<tr>' + sfile + '<td nowrap>Target name matched regular expression = ' + regexp + '</td>' + tail)
            continue

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
                    failed.append('<tr>' + sfile + '<td nowrap>Too few frames = ' + str(nf) + '</td>' + tail)
                    continue
            else:
                failed.append('<tr>' + sfile + '<td nowrap>Found no frames</td>' + tail)
                continue

            # OK, have passed a few simple tests, so set 
            # the lower and upper levels of the means of the CCDs
            # Find the most recent predefined data earlier than
            # date of run.
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
                mrange = np.array(predef[key][robj.speed]['mrange'])
                rnge   = np.array(predef[key][robj.speed]['range'])
            else:
                print 'Read speed =',robj.speed,'not recognised from predef dated: ',key
                exit(1)

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
                failed.append('<tr>' + sfile + '<td nowrap>No frames were grabbed</td>' + tail)
                continue
            else:
                print 'Grabbed',len(fnames),'frames from',file

            print 'Checking their mean values.'

            (ok,message, stats) = check_frames(fnames, mrange-rnge/2., mrange+rnge/2., ssilver)
            if ok:

                # Passed first few OK, lets grab some more
                first = last+1
                last  = min(nf-1, maxfrm+njunk)
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
                        failed.append('<tr>' + sfile + '<td nowrap>Failed to grab enough frames</td>' + tail)
                        continue
                    else:
                        print 'Grabbed another',len(extra),'frames from',file
                        fnames += extra

                print 'Checking all mean values.'

                (ok,message, stats) = check_frames(fnames, mrange-rnge/2., mrange+rnge/2., ssilver)

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
                            tmpfile = 'zzz_sxt_%d_%d.fits' % (nc+1,nw+1)

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

                    # Top-notch frames have no objects at all and a small spread.
                    if nobj.sum() == 0 and (lmax-lmin < sgold).all() and (rmax-rmin < sgold).all():
                        gold.append('<tr><td><a href="%s.ucm">%s</a></td><td>%d</td><td align="center">%sx%s</td><td align="center">%s</td>' \
                                        % (name,name,nframe,robj.x_bin,robj.y_bin,robj.speed) + message + tail)
                        modf['Procbias.class'] = {'comment': 'class of bias', 'type' : ucm.ITYPE_STRING, 'value': 'Gold'}
                    else:
                        silver.append('<tr><td><a href="%s.ucm">%s</a></td><td>%d</td><td align="center">%sx%s</td><td align="center">%s</td><td nowrap>%s</td>' \
                                          % (name,name,nframe,robj.x_bin,robj.y_bin,robj.speed,str(nobj)) + message + tail)
                        modf['Procbias.class'] = {'comment': 'class of bias', 'type' : ucm.ITYPE_STRING, 'value': 'Silver'}


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
                    modf['Procbias.Param.mrange']  = {'comment': 'target mean levels', 'type' : ucm.ITYPE_FVECTOR, 'value': mrange}
                    modf['Procbias.Param.range']   = {'comment': 'allowed ranges in mean levels', 'type' : ucm.ITYPE_FVECTOR, 'value': rnge}
                    modf['Procbias.Param.sgold']   = {'comment': 'gold-class maximum spread in mean levels', 'type' : ucm.ITYPE_FLOAT, 'value': sgold}
                    modf['Procbias.Param.ssilver'] = {'comment': 'silver-class maximum spread in mean levels', 'type' : ucm.ITYPE_FLOAT, 'value': ssilver}
                    modf['Procbias.Param.thresh']   = {'comment': 'sigma clipping threshold', 'type' : ucm.ITYPE_FLOAT, 'value': thresh}
                    modf['Procbias.Param.high']     = {'comment': 'RMS threshold for a pixel to count as high', 'type' : ucm.ITYPE_FLOAT, 'value': thresh}
                    modf['Procbias.Param.frac']     = {'comment': 'maximum fraction of high pixels', 'type' : ucm.ITYPE_FLOAT, 'value': frac}
                    modf['Procbias.Param.sthresh']  = {'comment': 'SExtractor object detection threshold', 'type' : ucm.ITYPE_FLOAT, 'value': sthresh}
                    modf['Procbias.Param.smarea']   = {'comment': 'SExtractor minimum pixel area', 'type' : ucm.ITYPE_INT, 'value': smarea}
                    modf['Procbias.Param.edge']     = {'comment': 'edge strip to ignore objects (pixels)', 'type' : ucm.ITYPE_FLOAT, 'value': edge}
                    modf.write(name + '.ucm')

                else:
                    failed.append('<tr>' + sfile + '<td nowrap>' + message + '</td>' + tail)
            else:
                failed.append('<tr>' + sfile + '<td nowrap>' + message + '</td>' + tail)

            # Delete frames unless debug in operation
            if not debug:
                for fname in fnames:
                    os.remove(fname)

except (KeyboardInterrupt, SystemExit):
    pass

gold.sort()

bout = open(fgold, 'w')
bout.write("""
<html>
<head>
<title>ULTRACAM gold biases</title>
</head>
<body>
<h1>ULTRACAM gold biases</h1>

<p>
The biases here have passed all tests applied to them. The table below lists the following stats 
to help you decide if they are likely to be OK for you:
<ol>
<li>the number of frames used to form the bias
<li>the X by Y binning factors used
<li>the readout speed hex code 
<li>for each half of each CCD, the mid-range of all contributing means, the range from mnimum to maximum mean
value and and the mean RMS (after rejection of discrepant values).
<li>the fraction of pixels identified as having a "high" value as a first guard against real objects.
</ol>
The parameters used to generate each bias are stored inside the headers (command 'uinfo' of the pipeline)
</p>

<table border="1" cellpadding="4" cellspacing="2">

<tr><th>File</th><th>Nframe</th><th align="center">Bin</th>
<th align="center">Speed</th>
<th colspan="6">CCD 1</th><th colspan="6">CCD 2</th><th colspan="6">CCD 3</th>
<th>High</th><th>Target</th><th>Comment</th></tr>

<tr><th></th><th></th><th></th><th></th>
<th colspan="3">Left</th><th colspan="3">Right</th>
<th colspan="3">Left</th><th colspan="3">Right</th>
<th colspan="3">Left</th><th colspan="3">Right</th>
<th></th><th></th><th></th></tr>

""")

# write out all table rows, one per bias
for line in gold:
    bout.write(line)

bout.write("""
</table>

<address>
Tom Marsh, Warwick
</address>

</body>
</html>
""")

bout.close()
print fgold,'written to disk'

# silver biases
silver.sort()

bout = open(fsilver, 'w')
bout.write("""
<html>
<head>
<title>ULTRACAM silver biases</title>
</head>
<body>
<h1>ULTRACAM silver biases</h1>

<p>
The biases here have passed all tests applied to them apart from the final SExtractor run which has
found what it thinks are objects. <i>Many of these may be perfectly usable!</i> For instance, sometimes
SExtractor picks up on low-level fixed pattern noise in one or more of the chips, and in at least one case
the object found is very low level that probably does not coincide in position with the targets. Nevertheless,
you should not use any of these without looking at the frame first. The table below 
lists the following stats to help you decide if they are likely to be OK for you:
<ol>
<li>the number of frames used to form the bias
<li>the X by Y binning factors used
<li>the readout speed hex code 
<li>the numbers of objects detected in each CCD using SExtractor
<li>for each half of each CCD, the mid-range of all contributing means, the range from mnimum to maximum mean
value and and the mean RMS (after rejection of discrepant values).
<li>the fraction of pixels identified as having a "high" value as a first guard against real objects.
</ol>
The parameters used to generate each bias are stored inside the headers (command 'uinfo' of the pipeline)
</p>

<table border="1" cellpadding="4" cellspacing="2">

<tr><th>File</th><th>Nframe</th><th align="center">Bin</th>
<th align="center">Speed</th><th align="center">Objects</th>
<th colspan="6">CCD 1</th><th colspan="6">CCD 2</th><th colspan="6">CCD 3</th>
<th>High</th><th>Target</th><th>Comment</th></tr>

<tr><th></th><th></th><th></th><th></th><th></th>
<th colspan="3">Left</th><th colspan="3">Right</th>
<th colspan="3">Left</th><th colspan="3">Right</th>
<th colspan="3">Left</th><th colspan="3">Right</th>
<th></th><th></th><th></th></tr>

""")

# write out all table rows, one per bias
for line in silver:
    bout.write(line)

bout.write("""
</table>

<address>
Tom Marsh, Warwick
</address>

</body>
</html>
""")

bout.close()
print fsilver,'written to disk'


# failed 
failed.sort()

bout = open(ffailed, 'w')
bout.write("""
<html>
<head>
<title>ULTRACAM not biases</title>
</head>
<body>
<h1>ULTRACAM not biases</h1>

<p>
These are the runs which have been tested as biases but failed to make the cut.
This should eventually include every single valid frame not found to match the
bias criteria and therefore most frames should be here for perfectly valid reasons.
There is also a subset list of <a href=
""")

bout.write('"%s"' % fbfailed)

bout.write("""
>apparent biases</a> (from the presence of the word 'bias' in the target name or comments)
that still did not get identified as biases. It is possible that a fair few of these are 
actually usable, but it is left to you to try to recover them if you want.

<p>
<table border="1" cellpadding="4" cellspacing="2">

<tr><th>File</th><th>Reason</th><th>Target</th><th>Comment</th></tr>

""")

# write out all table rows, one per bias
for line in failed:
    bout.write(line)

bout.write("""
</table>

<address>
Tom Marsh, Warwick
</address>

</body>
</html>
""")

bout.close()
print ffailed,'written to disk'

# now write subset of the failures out.
bout = open(fbfailed, 'w')
bout.write("""
<html>
<head>
<title>ULTRACAM not biases</title>
</head>
<body>
<h1>ULTRACAM not biases</h1>

<p>
These are runs in which the word 'bias' appears in either the target name or comment but which failed
the bias id criteria applied by 'procbias', such as too large/small mean value etc. These will include 
frames with low or high bias problems, bad pickup noise and the like, but may also include the odd 
reasonable run that failed from overly-conservative selection criteria. If you need one of these, you 
will have to go to the raw data. You may also want to review the <a href=
""")

bout.write('"%s"' % ffailed)

bout.write("""
>full list</a> of frames that did not make it as biases.

<p>
<table border="1" cellpadding="3" cellspacing="2">

<tr><th>File</th><th>Reason</th><th>Target</th><th>Comment</th></tr>

""")

# write out all table rows, one per bias
bre = re.compile('bias',re.I)
for line in failed:
    if bre.search(line):
        bout.write(line)

bout.write("""
</table>

<address>
Tom Marsh, Warwick
</address>

</body>
</html>
""")

bout.close()
print fbfailed,'written to disk'

exit(0)


