#!/usr/bin/env python

# Script to re-organise files after initial import in VSD/PK format
# with underscores and the like. It should be run from the raw_data
# directory where all the night-by-night and run directories are.
# Just give the name of the run directory of the form YYYY-MM 
# containing the initial import of data from ULTRACAM. It will:
#
# 1) Find all directories of form yyyy_mm_dd
# 2) Move all files into a sub-directory 'data' of each night
# 3) Copy the hand log of form yyyy_mm_dd_log.dat to yyyy-mm-dd.dat
# 4) Replace underscores in directory with '-' signs.


import sys, os, re, shutil
import re

if len(sys.argv) != 2:
    print 'usage: directory'
    exit(1)

cwd = os.getcwd()
if cwd != '/storage/astro1/phsaap/ultracam/raw_data':
    print 'This must be run from /storage/astro1/phsaap/ultracam/raw_data'
    exit(1)

tdir = sys.argv[1]
if not os.path.isdir(tdir):
    print tdir,'is not a directory; aborting',sys.argv[0]
    exit(1)

vikdir = re.compile('\d\d\d\d_\d\d_\d\d$')
dirs = [d for d in os.listdir(tdir) if vikdir.match(d) \
            and os.path.isdir(os.path.join(tdir, d))]

lfile = re.compile('\d\d\d\d_\d\d_\d\d_log.dat$')
for d in dirs:

    old_dir   = os.path.join(tdir, d)
    new_dir   = d.replace('_','-')
    if os.path.exists(new_dir):
        print new_dir,'already exists!'
        exit(1)

    flist = os.listdir(old_dir)

    for fname in flist:
        old_log = os.path.join(old_dir, fname)
        if lfile.match(fname):
            new_log = os.path.join(old_dir, fname[:-8].replace('_','-') + '.dat')
            print '\ncopy',old_log,'-->',new_log
            shutil.copy(old_log, new_log)

    print 'move',old_dir,'-->',new_dir
    shutil.move(old_dir, new_dir)
    src = os.path.join('..', new_dir)
    dst = os.path.join(tdir, new_dir)
    print 'linking',src,'--->',dst
    os.symlink(src, dst)
