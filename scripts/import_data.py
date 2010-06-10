#!/usr/bin/env python

# Script to re-organise files after initial import in VSD/PK format
# with underscores and the like. Just give the name of the directory
# containing the night directories. It will:
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

tdir = sys.argv[1]
if not os.path.isdir(tdir):
    print tdir,'is not a directory; aborting',sys.argv[0]
    exit(1)


vikdir = re.compile('\d\d\d\d_\d\d_\d\d$')
dirs = [d for d in os.listdir(tdir) if vikdir.match(d) \
            and os.path.isdir(os.path.join(tdir, d))]

lfile = re.compile('\d\d\d\d_\d\d_\d\d_log.dat$')
for d in dirs:
    dir   = os.path.join(tdir, d)
    flist = os.listdir(dir)
    data  = os.path.join(dir, 'data')
    if not os.path.exists(data):
        pass
        os.makedirs(data)
    elif os.path.exists(data) and not os.path.isdir(data):
        print 'Found',data,'but it is not a directory.'
        print 'Aborting',sys.argv[0]
        exit(1)

    for fname in flist:
        old_name = os.path.join(dir, fname)
        if fname != 'data':
            if lfile.match(fname):
                new_log = os.path.join(data, fname[:-8].replace('_','-') + '.dat')
                print 'copy',old_name,'-->',new_log
                shutil.copy(old_name, new_log)
            new_name = os.path.join(data, fname)
            print 'move',old_name,'-->',new_name
            shutil.move(old_name, new_name)

    new_dir = dir.replace('_','-')
    print 'move',dir,'-->',new_dir
    shutil.move(dir, new_dir)
