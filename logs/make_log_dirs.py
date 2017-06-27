#!/usr/bin/env python

from __future__ import print_function

"""
Script to setup directories in logs. Run from the top-level ultracam or
ultraspec directory (will be checked).

Once new data are imported into the raw data directories (requires directories
for each day containing log files in format 'yyyy-mm-dd.dat') run this script
to set up corresponding directories in the log directories and to link the log
files.
"""

import os, re

root  = '/storage/astro1/phsaap'

ucam  = os.path.join(root, 'ultracam')
uspc  = os.path.join(root, 'ultraspec')

cwd = os.getcwd()
if cwd != ucam and cwd != uspc:
    print('This must be run from ',ucam,'or',uspc)
    exit(1)

raw = 'raw_data'
log = 'logs'

# regular expressions to match run and night directories
mrdir = re.compile('^\d\d\d\d-\d\d$')
mndir = re.compile('^\d\d\d\d-\d\d-\d\d$')

# Create list of run directories in the raw data directory
rlist = [rdir for rdir in os.listdir(raw)
         if mrdir.search(rdir) or rdir == 'Others']

# Create equivalents in the log directory (if they don't exist)
dlist = {}
for run in rlist:

    # log_dir is run directory name in log directories
    log_dir = os.path.join(log, run)
    if not os.path.exists(log_dir):
        os.mkdir(log_dir)
        print('Created directory',log_dir)
    elif not os.path.isdir(log_dir):
        print(log_dir,'already exists but is not a directory.')
        exit(1)

    # create link to telescope file
    link_targ = os.path.join(log, run, 'telescope')
    if not os.path.exists(link_targ):
        link = os.path.join('../..', raw, run, 'telescope')
        print('linking',link_targ,'--->',link)
        os.symlink(link, link_targ)

    # Compile list of night-by-night directories
    raw_dir = os.path.join(raw, run)
    ndirs = [ndir for ndir in os.listdir(raw_dir)
             if mndir.search(ndir)]

    key = run
    dirs = ndirs
    # make sure equivalents exist in log directory
    for ndir in ndirs:
        log_dir  = os.path.join(log, ndir)
        if not os.path.exists(log_dir):
            os.mkdir(log_dir)
            print('Created directory',log_dir)
            link_targ = os.path.join(log, run, ndir)
            link      = os.path.join('..', ndir)
            print('linking',link_targ,'--->',link)
            os.symlink(link, link_targ)
        elif not os.path.isdir(log_dir):
            print(log_dir,'already exists but is not a directory.')
            continue

        link_targ = os.path.join(log, ndir, 'data')
        if not os.path.exists(link_targ):
            link      = os.path.join('../..', raw, ndir)
            print('linking',link_targ,'--->',link)
            os.symlink(link, link_targ)

        year,month,day = ndir.split('-')
        link_targ = os.path.join(log, ndir, ndir + '.dat')
        if not os.path.exists(link_targ):
            fl = os.path.join(raw, ndir, ndir + '.dat')
            if os.path.exists(fl):
                link = os.path.join('../..', raw, ndir, ndir + '.dat')
                print('linking',link_targ,'--->',link)
                os.symlink(link, link_targ)
            else:
                print('Failed to find',fl,'to link to',link_targ)
