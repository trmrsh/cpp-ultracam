#!/usr/bin/env python

import os

# Kick off with a few basic checks
cwd = os.getcwd()
if cwd != '/storage/astro2/phsaap/ultracam':
    print 'ERROR: this must be run from /storage/astro2/phsaap/ultracam'
    exit(1)

if 'ULTRACAM' not in os.environ:
    print 'ERROR: environment variable "ULTRACAM" is not defined.'
    exit(1)

grab    = os.path.join(os.environ['ULTRACAM'], 'grab')
combine = os.path.join(os.environ['ULTRACAM'], 'combine')
if not os.path.exists(grab) or not os.path.exists(combine):
    print 'ERROR: either or both of "grab" and "combine" could not be found.'
    exit(1)

# OK now handle the arguments
from optparse import OptionParser

usage = """%prog [options] flist

Script to process ULTRACAM biases and darks. The aim is to once-and-for-all median 
average any runs sent to this script, copy the data file to astronas and then delete 
the original, in order to save space. The script must be run from the top-level 
directory (checked). Each run is processed one-by-one to avoid large potential 
increases in required disk space if the 'redo' option is set (see options).

Arguments: 

flist   -- a file containing lines of the following sort

            2005-05-10 2 3 5 8 10 11

            i.e. a run date in YYYY-MM-DD format followed by a series of run numbers.
            Lines starting with '#' or which are blank will be ignored. There are 
            quite a few checks that must be satisfied; most are obvious (e.g. files 
            must exist). The only one that isn't is that there must be at least 3 
            frames (for a median combine to make sense) and since sometimes the 
            first or last frames have to be skipped for being of a different exposure 
            time, this may mean that there must be at least 5 frames in the file.

Problems come in three varieties:

 ERROR   -- a show stopper
 WARNING -- bad enough to skip a run but not stop everything
 NOTE    -- just for information
"""

parser = OptionParser(usage)
parser.add_option("-r", "--redo", dest="redo", default=False, action="store_true",\
                  help="recover data files from astronas if they are not found")
parser.add_option("-c", "--clobber", dest="clobber", default=False, action="store_true",\
                  help="clobber output ucm files")
parser.add_option("-k", "--keep", dest="keep", default=False, action="store_true",\
                  help="keep the .dat files")

(options, args) = parser.parse_args()
if len(args) != 1:
    print 'ERROR: a single argument is required; use -h for help.'
    exit(1)

redo    = options.redo
clobber = options.clobber
keep    = options.keep

import sys, re, subprocess
import Ultra
from trm import ucm

# Read the biases in
fin = open(args[0])

biases = []
dates  = []
dre = re.compile('^\d\d\d\d-\d\d-\d\d$')
for line in fin:
    if not line.startswith('#') and line.strip() != '':
        elems = line.split()
        if len(elems) < 2:
            print 'ERROR: too few entries in line =',line
            exit(1)
        date = elems[0]
        if not dre.match(date):
            print 'ERROR: first element of each line must be a date in the form YYYY-MM-DD'
            print 'Invalid line =',line
            exit(1)
        if date in dates:
            print 'ERROR: date =',date,'is repeated more than once'
            print 'Second entry in line =',line
            exit(1)
        dates.append(date)

        runs = elems[1:]
        for run in runs:
            bias = os.path.join(date , 'run%03d' % (int(run),))
            if bias in biases:
                print 'ERROR: one or more runs repeated more than once.'
                print 'Line =',line
                exit(1)
            biases.append(bias)
fin.close()

if not len(biases):
    print 'No bias names loaded from',sys.argv[1]
    exit(1)

print 'Loaded',len(biases),'names from',sys.argv[1],'\n'
        
fcre = re.compile('^(\d\d\d\d-\d\d-\d\d)/(run\d\d\d)$')


# OK lets get on with it
total = 0
for bias in biases:

    if not fcre.match(bias):
        print 'WARNING: file name =',bias,'does not have the correct YYYY-MM-DD/run### format; run skipped.'
        continue

    xml  = os.path.join('raw_data', bias + '.xml')
    if not os.path.exists(xml):
        print 'WARNING: xml file =',xml,'does not exist; run skipped.'
        continue

    jnk  = os.path.join('raw_data', bias + '.jnk')
    if not os.path.exists(jnk):
        print 'WARNING: file =',jnk,'indicates that run is flagged as junk; run skipped.'
        continue

    ufile = os.path.join('derived_data', bias + '.ucm')
    if not clobber and os.path.exists(ufile):
        print 'WARNING: ucm file =',ufile,'already exists; run skipped.'
        continue

    # Search for .dat then .dat.gz; if redo=True, also look on astronas
    # gunzip if .dat.gz
    data = os.path.join('raw_data', bias + '.dat')
    retrieved = False
    if not os.path.exists(data):
        datagz = os.path.join('raw_data', bias + '.dat.gz')
        if not os.path.exists(datagz):
            if redo:
                if not subprocess.Popen(['ssh', 'astronas', 'cd ultracam; ls ' + data]).wait():

                    print '.... found',data,'on astronas; now retrieving'
                    if subprocess.Popen(['scp', 'astronas:ultracam/' + data, data]).wait():
                        print 'WARNING: dould not recover data file =',data,'from astronas; run skipped.'
                    retrieved = True
                elif not subprocess.Popen(['ssh', 'astronas', 'cd ultracam; ls ' + datagz]).wait():

                    print '.... found',datagz,'on astronas; now retrieving'
                    if subprocess.Popen(['scp', 'astronas:ultracam/' + datagz, datagz]).wait():
                        print 'WARNING: dould not recover data file =',datagz,'from astronas; run skipped.'
                    print '.... gunzipping',datagz

                    if subprocess.Popen(['gunzip', datagz]).wait(): 
                        print 'WARNING: failed to gunzip',datagz,', skipping run.'
                        continue
                    retrieved = True
                else:
                    print 'WARNING: dould not recover data file =',data,'from astronas; run skipped.'
                    continue
            else:
                print 'WARNING: data file =',data,'does not exist; run skipped.'
                continue
        else:
            print '.... did not find',data,'but did find',datagz,'... applying gunzip.'
            if subprocess.Popen(['gunzip', datagz]).wait(): 
                print 'WARNING: failed to gunzip',datagz,' skipping run'
                continue
            

    # Read in basic info on the run
    run = Ultra.Run(xml, noid=True)
    nbytes_per_frame = run.size()
    nbytes_total = os.path.getsize(data)

    if nbytes_per_frame == 0:
        print 'WARNING: file =',data,'has zero bytes per frame; run skipped.'
        continue

    if nbytes_total % nbytes_per_frame != 0:
        print 'WARNING: file =',data,'does not contain a whole number of frames; run skipped.'
        continue

    nframes = nbytes_total // nbytes_per_frame

    if int(run.num_exp) != -1 and int(run.num_exp) != nframes:
        print 'NOTE: XML file =',xml,'indicates that there are',run.num_exp,'exposures but',nbytes_total // nbytes_per_frame,\
            'were found in the data file =',data

    # Determine range of frame to include. Last is skipped if it was stopped by hand. First is skipped
    # if not a clear mode because it has a different exposure time from the rest.
    last = nframes if int(run.num_exp) > 0 and int(run.num_exp) == nframes else nframes - 1

    if run.mode == 'DRIFT':
        first = int((1033./int(run.ny[0])/int(run.y_bin)+1.)/2.) + 1
    elif run.was_cleared():
        first = 1
    else:
        first = 2

    # limit max number to 1000
    last = min(last, first+999)
    if last-first+1 < 3:
        print 'WARNING: frame range =',first,'to',last,'is invalid -- need at least 3 frames; run skipped.'
        continue

    # all initial checks passed, lets start processing.
    rpath  = os.path.join(cwd, 'raw_data', bias)

    print '\nProcessing run =',rpath

    print '.... grabbing frames',first,'to',last

    # grab the files, saving output
    args = (grab, 'source=l', 'file=' + rpath, 'data=' + data, 'ndigit=5', 'first='+str(first), 'last='+str(last), 'trim=no', 'twait=1', 'tmax=1', 'skip=no', \
                'bregion=no', 'bias=no', 'nodefs')
    output = subprocess.Popen(args, stdout=subprocess.PIPE, cwd='/tmp', stderr=subprocess.PIPE).communicate()[0].split('\n')

    # Extract list of files written to disk
    flist = [line[8:line.find(',')] for line in output if line.startswith('Written')]
    if len(flist) != last-first+1:
        print 'ERROR: wrote',len(flist),'files but expected',last-first+1
        print '\n'.join(output)
        exit(1)

    # write to a disk file
    ldisk = os.path.join(cwd, 'derived_data', bias + '.lis')

    foptr = open(ldisk, 'w')
    foptr.write('\n'.join(flist))
    foptr.close()

    # Finally remove output ucm if clobber set and it exists
    if os.path.exists(ufile) and clobber:
        os.unlink(ufile)

    # combine the files
    print '.... combining the frames'
    ofile = os.path.join(cwd, 'derived_data', bias)

    args = (combine, 'list=' + ldisk, 'method=c', 'sigma=2.8', 'careful=yes', 'adjust=b', 'output=' + ofile, 'nodefs')
    output = subprocess.Popen(args, stdout=subprocess.PIPE, cwd='/tmp', stderr=subprocess.PIPE).communicate()[0].split('\n')
    print '.... combined file written to',ofile+'.ucm'

    if len(output) < 3 or output[-3] != 'Finished.':
        print 'ERROR: "combine" failed with output:'
        print '\n'.join(output)
        exit(1)

    # compute some stats to add to file
    print '.... computing some stats to store in file -- this can take a while.'
    lmin,lmax,lmm,lrm,lgrad,rmin,rmax,rmm,rrm,rgrad = Ultra.flist_stats([os.path.join('/tmp',fname) for fname in flist], 2, 2, 3.)
    modf = ucm.rucm(ofile + '.ucm')
    modf['Procbias']        = {'comment': 'procbias information', 'type' : ucm.ITYPE_DIR, 'value': None}
    modf['Procbias.nframe'] = {'comment': 'number of contributing frames', 'type' : ucm.ITYPE_INT, 'value': len(flist)}
    modf['Procbias.first']  = {'comment': 'first contributing frame', 'type' : ucm.ITYPE_INT, 'value': first}
    modf['Procbias.last']   = {'comment': 'last contributing frame', 'type' : ucm.ITYPE_INT, 'value': last}
    modf['Procbias.lmid']   = {'comment': 'mid-ranges of mean values of left-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': (lmin+lmax)/2.}
    modf['Procbias.lmean']  = {'comment': 'means of mean values of left-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': lmm}
    modf['Procbias.lrng']   = {'comment': 'ranges of mean values of left-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': lmax-lmin}
    modf['Procbias.lrms']   = {'comment': 'means of RMSs of left-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': lrm}
    modf['Procbias.lgrad']  = {'comment': 'gradients of means of left-hand sides of CCDs (counts/frame)', 'type' : ucm.ITYPE_FVECTOR, 'value': lgrad}
    modf['Procbias.rmid']   = {'comment': 'mid-ranges of mean values of right-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': (rmin+rmax)/2.}
    modf['Procbias.rmean']  = {'comment': 'means of mean values of right-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': rmm}
    modf['Procbias.rrng']   = {'comment': 'ranges of mean values of right-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': rmax-rmin}
    modf['Procbias.rrms']   = {'comment': 'means of RMSs of right-hand sides of CCDs', 'type' : ucm.ITYPE_FVECTOR, 'value': rrm}
    modf['Procbias.rgrad']  = {'comment': 'gradients of means of right-hand sides of CCDs (counts/frame)', 'type' : ucm.ITYPE_FVECTOR, 'value': rgrad}
    modf.write(ofile + '.ucm')

    # delete the intermediate ucm files
    map(lambda fn: os.unlink(os.path.join('/tmp', fn + '.ucm')), flist)
    print '.... removed intermediate ucm files'

    # set protection on file generated  
    os.chmod(ofile + '.ucm', 0444)
    print '.... set permissions on',ofile+'.ucm'

    # copy data file to astronas, ensuring parent directory is in place. 
    # skip this stage if the file wasd retrieved from astronas in the first
    # place (redo option)
    if not retrieved:
        print '.... copying data and xml files to astronas'

        m    = fcre.match(bias)
        pdir = m.group(1)

        if subprocess.Popen(['ssh', 'astronas', 'cd ultracam/raw_data; mkdir -p %s' % (pdir,)]).wait(): 
            print 'ERROR: failed to create containing directory',pdir,'on astronas'
            exit(1)

        if subprocess.Popen(['scp', data, 'astronas:ultracam/' + data]).wait(): 
            print 'ERROR: failed to copy',data,'to astronas'
            exit(1)
    else:
        print '.... copying xml file to astronas'

    xml  = os.path.join('raw_data', bias + '.xml')
    if subprocess.Popen(['scp', xml, 'astronas:ultracam/' + xml]).wait(): 
        print 'ERROR: failed to copy',xml,'to astronas'
        exit(1)
        
    # delete data file
    if not keep: 
        os.unlink(data)
        if not retrieved:
            total += nbytes_total
        print '.... deleted',data

if total > 0:
    print 'Raw data files containing',int(total/(1024.*1024.)+0.5),'MB were removed.'
