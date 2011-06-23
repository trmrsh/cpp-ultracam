#!/usr/bin/env python

"""
Script to clear out junk files from the ultracam archive.
Any run with a corresponding '.jnk' file (which can be empty, 
so just touching such a file is enough to flag it) counts as 
junk. If such a file is found with an equivalent .xml and .dat, 
the .dat and .xml will be copied to astronas while the .dat
file will be deleted.

This should be run from the top-level ultracam directory on storage.
"""

import os, sys, re, shutil, subprocess
from os.path import join, isfile, exists, isdir, getsize

cwd = os.getcwd()
if cwd != '/storage/astro2/phsaap/ultracam':
    print 'This must be run from /storage/astro2/phsaap/ultracam'
    exit(1)

# File matching regular expressions
ddir_re = re.compile('raw_data/\d\d\d\d-\d\d-\d\d$') # Search for data directories
xml_re  = re.compile('run\d\d\d\.xml$') # Search for xml files

# Walk through whole tree, selecting only data directories

total = 0
for root, dirs, files in os.walk('.'):
    if ddir_re.search(root):

        # create a list of files to be moved/copied
        runs = [x[:-4] for x in files if isfile(join(root, x)) and isfile(join(root, x[:-4] + '.jnk')) and xml_re.match(x) is not None]
        for run in runs:

            # create parent directory on astronas
            pdir = root[11:]
            if subprocess.Popen(['ssh', 'astronas', 'cd ultracam/raw_data; mkdir -p %s' % (pdir,)]).wait(): 
                print 'ERROR: failed to create containing directory',pdir,'on astronas'
                exit(1)

            data = os.path.join(root, run + '.dat')
            if os.path.exists(data):
                if subprocess.Popen(['scp', data, 'astronas:ultracam/' + data]).wait(): 
                    print 'ERROR: failed to copy',data,'to astronas'
                    exit(1)
                else:
                    total += os.path.getsize(data)
                    os.unlink(data)
                    print 'copied and removed',data

                    xml  = os.path.join(root, run + '.xml')
                    if os.path.exists(xml) and subprocess.Popen(['scp', xml, 'astronas:ultracam/' + xml]).wait(): 
                        print 'ERROR: failed to copy',xml,'to astronas'
                        exit(1)
                    else:
                        total += os.path.getsize(xml)
            else:
                data = os.path.join(root, run + '.dat.gz')
                if os.path.exists(data):
                    if subprocess.Popen(['scp', data, 'astronas:ultracam/' + data]).wait(): 
                        print 'ERROR: failed to copy',data,'to astronas'
                        exit(1)
                    else:
                        total += os.path.getsize(data)
                        os.unlink(data)
                        print 'copied and removed',data

                        xml  = os.path.join(root, run + '.xml')
                        if os.path.exists(xml) and subprocess.Popen(['scp', xml, 'astronas:ultracam/' + xml]).wait(): 
                            print 'ERROR: failed to copy',xml,'to astronas'
                            exit(1)
                        else:
                            total += os.path.getsize(xml)

print 'Transferred a total of',int(10.*float(total)/1024**2+0.5)/10.,'MB'
