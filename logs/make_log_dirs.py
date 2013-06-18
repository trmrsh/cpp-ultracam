#!/usr/bin/env python

"""
Script to setup directories in logs. Run from the top-level ultracam directory

Once new data are imported into the raw data directories (requires directories 
for each day containing log files in format 'yyyy-mm-dd.dat') run this script 
to set up corresponding directories in the log directories and to link the log
files.
"""

import os, re

cwd = os.getcwd()
if cwd != '/storage/astro1/phsaap/ultracam':
    print 'This must be run from /storage/astro1/phsaap/ultracam'
    exit(1)

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

    link_targ = os.path.join(log, run, 'telescope')
    if not os.path.exists(link_targ):
        link = os.path.join('../..', raw, run, 'telescope')
        print 'linking',link_targ,'--->',link
        os.symlink(link, link_targ)        


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
# if need be, link back to data directories
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

        link_targ = os.path.join(log, d, 'data')
        if not os.path.exists(link_targ):
            link      = os.path.join('../..', raw, d)
            print 'linking',link_targ,'--->',link
            os.symlink(link, link_targ)        

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
                
