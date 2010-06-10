#!/usr/bin/env python

"""
Script to generate timing files for ULTRACAM and ULTRASPEC. 

It expects directories of the form '2005-11' (Nov 2005) which have
a structure like so:

2005-11
  2005-11-23
    data             -- directory with run###.xml and run###.dat files    
  2005-11-24  -- directory for 24 Nov
    data      -- directory with run###.xml and run###.dat files    

etc. It then winds through all of the data and generates
files such as 2005-11-23.times which will be placed in the 2005-11-23
directory. It will only do this if there is no such file already.

"""

import os
import sys
import re
import trm.subs as subs
import subprocess as subproc

if __name__ == "__main__":

    gettime = os.path.join(os.environ['TRM_SOFTWARE'], 'bin', 'ultracam', 'gettime')

    # The main program

    # Create a list directories of runs to search through
    rdirs = [x for x in os.listdir(os.curdir) if os.path.isdir(x) and x.startswith('20')]
    for rdir in rdirs:

        # Now the night-by-night directories
        ndirs = [x for x in os.listdir(rdir) if os.path.isdir(os.path.join(rdir,x)) and x.startswith('20')]

        for ndir in ndirs:

            npath = os.path.join(rdir, ndir)

            times = os.path.join(npath, ndir + '.times')


            if not os.path.exists(times):
                data  = os.path.join(npath, 'data')
                if not os.path.isdir(data):
                    print 'File = ' + times + ' does not exist, but ' + data + ' does not either.'
                else:                    
                    ft = open(times,'w')
                    dfiles = [os.path.join(data, x[:-4]) for x in os.listdir(data) if x.startswith('run') and x.endswith('.xml')]

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
                            


