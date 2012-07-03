#!/usr/bin/env python

# !!begin
# !!title    ederived
# !!author   T.R. Marsh
# !!created  10 July 2007
# !!revised  16 Feb 2011
# !!root     ederived
# !!index    ederived
# !!descr    exports derived data directory using hard links
# !!class    Scripts
# !!css      style.css
# !!head1    ederived makes all the derived data directories available
#
# !!emph{ederived} creates hard links to the files in the derived data directories
#
# !!head2 Invocation
#
# ederived
#
#
# !!end

import sys, os, re

fwd = '/storage/astro2/www/phsaap/data'
wd = os.getcwd()    
if wd != fwd:
    print 'You must run this script from ' + fwd
    exit(1)

derived_dir = '/storage/astro2/phsaap/ultracam/derived_data'

for path, dirs, files in os.walk(derived_dir):
    start = path.find('derived_data')
    if start == -1:
        print 'Unexpected directory name = ',rdir
        exit(1)
    ndir = path[start:]
    if not os.path.exists(ndir):
        os.makedirs(ndir)
    for fname in files:
        source = os.path.join(path, fname)
        link   = os.path.join(ndir, fname)
        print source,'-->',link

        if os.path.exists(source):
            if os.path.exists(link):
                print 'Link =',link,'already exists and will not be over-written'
            else:
                os.link(source, link)


