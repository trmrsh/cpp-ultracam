#!/usr/bin/env python

"""
Script to generate timing files for ULTRACAM and ULTRASPEC. Run this in the 
'logs' directory

It expects directories of the form '2005-11' (Nov 2005) which have
a structure like so:

2005-11
  2005-11-23 -- directory with data subdirectory containing run###.xml and run###.dat files.
                File called 2005-11-23.times will be created here if not already present.

"""

import os, sys, re
import trm.subs as subs
import subprocess as subproc

if __name__ == "__main__":

    gettime = os.path.join(os.environ['TRM_SOFTWARE'], 'bin', 'ultracam', 'gettime')

    # The main program

    # Create a list directories of runs to search through
    dre1 = re.compile(r'^\d\d\d\d-\d\d$')
    dre2 = re.compile(r'^\d\d\d\d-\d\d-\d\d$')

    rdirs = [x for x in os.listdir(os.curdir) if os.path.isdir(x) and dre1.match(x)]

    for rdir in rdirs:

        # Now the night-by-night directories, must be in both raw and log directories
        ndirs = [x for x in os.listdir(rdir) if dre2.match(x) and os.path.isdir(os.path.join(rdir,x))]

        for ndir in ndirs:

            dpath = os.path.join(rdir, ndir, 'data')
            times = os.path.join(rdir, ndir, ndir + '.times')

            # only try to construct times file if it does not exist
            if not os.path.exists(times):
                print 'File =',times,'does not exist; will create.'
                ft = open(times,'w')
                dfiles = [os.path.join(dpath, x[:-4]) for x in os.listdir(dpath) if x.startswith('run') and x.endswith('.xml')]

                for dfile in dfiles:
                    args = (gettime, dfile)
                    output  = subproc.Popen(args, stdout=subproc.PIPE, stderr=subproc.PIPE).communicate()[0].split('\n')
                    run     = 'UNDEF'
                    date    = 'UNDEF'
                    utstart = 'UNDEF'
                    utend   = 'UNDEF'
                    ngood   = 'UNDEF'
                    nframe  = 'UNDEF'
                    expose  = 'UNDEF'
                    sample  = 'UNDEF'
                    for line in output:
                        arr = line.split()
                        if line.startswith('Run'):
                            run = arr[2][-6:]
                        elif line.startswith('Number of frames'):
                            nframe = arr[4]
                        elif line.startswith('Number of good frames'):
                            ngood = arr[5]
                        elif line.startswith('OK run length'):
                            expose = arr[4]
                            sample = arr[9]
                        elif line.startswith('UT at start'):
                            date    = arr[4][:-1]
                            utstart = arr[5]
                        elif line.startswith('UT at end'):
                            utend   = arr[5]
                    ft.write(run + ' ' + date + ' ' + utstart + ' ' + utend + ' ' + ngood + ' ' + expose + ' ' + sample + '\n')
                ft.close()
                print 'Created file =',times
                            


