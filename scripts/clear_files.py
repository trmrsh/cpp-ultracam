#!/usr/bin/env python

"""
Script to clear out files from the ultracam archive.
Any run with a corresponding '.jnk' file (which can be empty, 
so just touching such a file is enough to flag it) counts as 
junk. If such a file is found with an equivalent .xml and .dat, 
the .dat will be moved to a different location; the xml will be 
copied along with it. Similarly any file with a corresponding
'.mov' file will be moved (with any luck the .mov file will contain 
a reason for doing so).

Run on scafell. The files are copied to the same directory tree 
under /tmp on scafell but you should take a copy as these will be 
wiped should there ever be a re-install. The following commands
will do this:

cd /tmp
rsync -av ultracam astronas: 
"""

import os
from os.path import join, isfile, exists, isdir, getsize
import sys
import re
import shutil

# File matching regular expressions
ddir_re = re.compile('\d\d\d\d-\d\d/\d\d\d\d-\d\d-\d\d/data$') # Search for YYYY-MM/YYYY-MM-DD/data
xml_re  = re.compile('run\d\d\d\.xml$') # Search for xml files

save_dir = '/tmp/ultracam'

# Walk through whole tree, selecting only data directories

total = 0
for root, dirs, files in os.walk('.'):
    if ddir_re.search(root):

        # create a list of files to be moved/copied
        runs = [x[:-4] for x in files if isfile(join(root, x)) and \
                    (isfile(join(root, x[:-4] + '.jnk')) or isfile(join(root, x[:-4] + '.mov'))) \
                    and xml_re.match(x) is not None]

        for run in runs:

            olddat = join(root, run + '.dat')
            oldxml = join(root, run + '.xml')

            if olddat or oldxml:

                # make a directory if need be
                new_dir = join(save_dir, root[-15:-5])
                if exists(new_dir) and not isdir(new_dir):
                    print 'FATAL ERROR: an entity called',new_dir,'exists but is not a directory.'
                    print 'Please fix this.'
                    print 'Transferred a total of',int(10.*float(total)/1024**2+0.5)/10.,'MB'
                    exit(1)
                elif not exists(new_dir):
                    os.makedirs(new_dir)
                    pass

                # move the data if possible
                newdat = join(new_dir, run + '.dat')
                if not exists(newdat):
                    shutil.move(olddat, newdat)
                    total += getsize(newdat)
                    print 'Moved ' + olddat + ' to ' + newdat
                else:
                    print 'File = ',newdat,'already exists and will not be over-written.'
                    print 'File = ',olddat,'will be left untouched.'

                # copy the xml if possible
                newxml = join(new_dir, run + '.xml')
                if not exists(newxml):
                    shutil.copy(oldxml, newxml)
                    print 'Copied ' + oldxml + ' to ' + newxml
                else:
                    print 'File = ',newxml,'already exists and will not be over-written.'

print 'Transferred a total of',int(10.*float(total)/1024**2+0.5)/10.,'MB'
