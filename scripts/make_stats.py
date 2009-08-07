#!/usr/bin/env python

"""
Script to generate statistics files. Runs ULTRACAM command diags on all files. 
Should be run in the top-level directory which contains all the various runs.
"""

import os
from os.path import join, isfile
import sys
import re
import trm.subs as subs
import subprocess

# command to carry out
command = [join(os.environ['TRM_SOFTWARE'], 'bin', 'ultracam', 'diags'), \
               'source=l','file=run000','first=1','last=0','trim=yes','ncol=0','nrow=2','twait=0', \
               'tmax=0','skip=yes']

# File matching regular expressions
ddir_re = re.compile('\d\d\d\d-\d\d/\d\d\d\d-\d\d-\d\d/data$') # Search for YYYY-MM/YYYY-MM-DD/data
xml_re  = re.compile('run\d\d\d\.xml$') # Search for xml files

# Walk through whole tree, selecting only data directories
cwd = os.getcwd()

for root, dirs, files in os.walk('.'):
    if ddir_re.search(root):

        runs = [x[:-4] for x in files if xml_re.match(x) is not None and isfile(join(root, x)) and isfile(join(root, x[:-4] + '.dat')) and \
                    not isfile(join(root, x[:-4] + '.dgs'))]
        os.chdir(root)
        print 'Switched to directory =',root
        for run in runs:
            print 'Processing ',join(root, run)
            fout = open(run + '.dgs', 'w')
            command[2] = 'file=' + run
            subprocess.Popen(command, stdout=fout).wait()
            fout.close()

        # return to top-level directory
        os.chdir(cwd)
