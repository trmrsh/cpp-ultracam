#!/usr/bin/env python

# !!begin
# !!title    fmatch
# !!author   T.R. Marsh
# !!created  23 June 2007
# !!revised  23 June 2007
# !!root     fmatch
# !!index    fmatch
# !!descr    finds matching ULTRACAM files by searching through xml files
# !!class    Scripts
# !!css      style.css
# !!head1    fmatch finds matching ULTRACAM files by searching through xml files
#
# !!emph{fmatch} searches a list of directories for ULTRACAM runs matching
# the format of a specific run.
# This is potentially helpful during reduction.
#
# !!head2 Invocation
#
# fmatch list run
#
# !!head2 Arguments
#
# !!table
#
# !!arg{list}{A file containing the names of the directories to search. Typically these would be the directories of the
# whole observing run.}
# !!arg{run}{the run number defining the foramt to be matched, as in run043}
#
# !!table
#
# Related commands: !!ref{missbias.html}{missbias}, !!ref{unique.html}{unique}
#
# !!end

import sys
import os
import re
import ultracam

if len(sys.argv) != 3:
    print 'usage: <directory search list> run'
    exit(1)

# name the arguments
dlist = sys.argv[1]
run   = sys.argv[2] if sys.argv[2].endswith('.xml') else sys.argv[2] + '.xml'

# read the list of directories to search
f = open(dlist)
dirs = f.readlines()
f.close()

# remove comments and blank lines and strip off trailing newlines
dirs = [elem.strip() for elem in dirs if not elem.startswith('#') and len(elem.strip()) > 0]

# read the format of the specified run
template = ultracam.Ultracam(run)

# now read the directories looking for all files of the form 'run[0-9][0-9][0-9].xml'
# read each of these files and compare their formats with the template just read in
# print those that match

rtest = re.compile('run[0-9][0-9][0-9]\.xml')

for d in dirs:
    runs = [rn for rn in os.listdir(d) if rtest.match(rn)]
    for rn in runs:
        fpath = os.path.join(d,rn)
        form  = ultracam.Ultracam(fpath)
        if ultracam.match(form, template):
            print form['run'] + ' '+ form.to_string()
            



            
