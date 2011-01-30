#!/usr/bin/env python

"""
Script to setup directories in logs

Once new data are imported into the raw data directories (requires directories 
for each day containing log files in format 'yyyy-mm-dd.dat') run this script 
to set up corresponding directories in the log directories and to link the log
files.
"""

import os
import re

raw = 'raw_data'
log = 'logs'

# Check for run directories
mdir = re.compile(raw + '/(\d\d\d\d-\d\d)$')
rlist = []
for (root,dirs,files) in os.walk('.'):
    for d in dirs:
        fpath = os.path.join(root, d)
        m = mdir.search(fpath)
        if m:
            rlist.append(m.group(1))

# Create missing run directories
for run in rlist:   
    log_dir = os.path.join(log, run)
    if not os.path.exists(log_dir):
        os.mkdir(log_dir)
        print 'Created directory',log_dir
    elif not os.path.isdir(log_dir):
        print log_dir,'already exists but is not a directory.'
        exit(1)

mdir = re.compile(raw + '/(\d\d\d\d-\d\d)/(\d\d\d\d-\d\d-\d\d)$')

# Now check for day directories
dlist = {}
for (root,dirs,files) in os.walk('.'):
    for d in dirs:
        fpath = os.path.join(root, d)
        m = mdir.search(fpath)
        if m: 
            run  = m.group(1)
            date = m.group(2)
            if run in dlist:
                dlist[run].append(date)
            else:
                dlist[run] = [date,]

# Create day directories, copy data logs across
# if need be
for key,dirs in dlist.iteritems():
    for d in dirs:
        log_dir  = os.path.join(log, d)
        if not os.path.exists(log_dir):
            os.mkdir(log_dir)
            print 'Created directory',log_dir
            link_targ = os.path.join(log, key, d)
            link      = os.path.join('..', d)
            print 'linking',link_targ,'--->',link
            os.symlink(link, link_targ)
        elif not os.path.isdir(log_dir):
            print log_dir,'already exists but is not a directory.'
            continue

        (year,month,day) = d.split('-')
        link_targ = os.path.join(log, d, d + '.dat')
        if not os.path.exists(link_targ):
            fl = os.path.join(raw, d, d + '.dat')
            if os.path.exists(fl):
                link = os.path.join('../..', raw, d, d + '.dat')
                print 'linking',link_targ,'--->',link
                os.symlink(link, link_targ)
            else:
                print 'Failed to find',fl,'to link to',link_targ
                
