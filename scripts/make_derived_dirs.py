#!/usr/bin/env python

"""
Script to setup directories in derived_data. Run from the top-level 
ultracam directory

Once new data have been imported into the raw data directories, run this script
to set up corresponding directories in the derived_data directories together
with 'data' sub-directory links to the corresponding raw data directories.
"""

import os, re

cwd = os.getcwd()
if cwd != '/storage/astro2/phsaap/ultracam':
    print 'This must be run from /storage/astro2/phsaap/ultracam'
    exit(1)

raw     = 'raw_data'
derived = 'derived_data'

# Check for day directories
mdir = re.compile(raw + '/(\d\d\d\d-\d\d-\d\d)$')
dlist = []
for (root,dirs,files) in os.walk('.'):
    for d in dirs:
        fpath = os.path.join(root, d)
        m = mdir.search(fpath)
        if m:
            dlist.append(m.group(1))

# Create missing day directories in derived
for day in dlist:   
    derived_dir = os.path.join(derived, day)
    if not os.path.exists(derived_dir):
        os.mkdir(derived_dir)
        print 'Created directory',derived_dir
    elif not os.path.isdir(derived_dir):
        print derived_dir,'already exists but is not a directory.'
        exit(1)

    link_targ = os.path.join(derived_dir, 'data')
    if not os.path.exists(link_targ):
        link = os.path.join('../..', raw, day)
        print 'linking',link_targ,'--->',link
        os.symlink(link, link_targ)        

