#!/usr/bin/env python

"""
Script to search out all files of the form 'run###.dat' larger than 2GB, starting from the current 
directory and proceeding down the tree. It will split these files into files with integer numbers
o frames and no larger than 2GB and gzip the results. This is to allow data to be transferred over 
the internet. This takes a long time to run.
"""

import os, subprocess, re

fre = re.compile('run\d\d\d\.dat')

# Hunt out the data files
flist = []
for (root,d,files) in os.walk('.'):
    flist += [os.path.join(root, f) for f in files if fre.match(f)]  

MAXSIZE = 2*1024**3

for fname in flist:

    if not os.path.isfile(fname):
        print fname,'is not a regular file.'
        continue

    fxml = fname[:-3] + 'xml'
    if not os.path.isfile(fxml):
        print 'Cannot find XML equivalent of',fname
        continue

    if os.path.getsize(fname) > MAXSIZE:
        xfptr = open(fxml)
        for line in xfptr:
            frs = line.find('framesize') 
            if frs > -1:
                framesize = int(line[frs+11:frs+11+line[frs+11:].find('"')])
                break
        else:
            print 'Could not find framesize of',fname
            continue
            
        xfptr.close()
        print fname,'has framesize =',framesize

        # number of bytes / files
        nbytes = int(MAXSIZE/framesize)*framesize

        command = ['split','--bytes=' + str(nbytes),fname,fname[:-4]]
        
