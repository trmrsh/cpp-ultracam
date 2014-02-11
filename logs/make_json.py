#!/usr/bin/env python

"""
Script to generate a JSON database for use with dynamic web pages to search
for ULTRASPEC?CAM runs. This should be run once new data are ingested into the
archive and as many targets have been identified and make_logs run for the 
final time. Only targets with position IDs are tracked.

It should be run in the 'logs' directory which has sub-directories of
the form '2005-11' (Nov 2005).
"""
import os, sys, re, json
from xml.dom import Node
from xml.dom.minidom import parse, parseString
import numpy as np
from trm import subs, simbad
import Ultra

# The main program

# Basic aim is to read as many xml and data files as possible. Errors are
# converted to warnings if possible and ideally the user should fix up the
# files until no such warnings appear.

# First read target data.
targets = Ultra.Targets('TARGETS', 'AUTO_TARGETS')

# File matching regular expressions
rdir_re = re.compile('^\d\d\d\d-\d\d$') # Search for YYYY-MM
ndir_re = re.compile('^\d\d\d\d-\d\d-\d\d$') # Search for night directories
xml_re  = re.compile('run\d\d\d\.xml$') # Search for xml files

# Targets to skip Simbad searches for; will be added to as more failures are
# found ensuring that searches for a given target are only made once.
fp    = open('SKIP_TARGETS')
sskip = fp.readlines()
sskip = [name.strip() for name in sskip if not name.startswith('#')]
fp.close()

# Create a list directories of runs to search through
rdirs = [x for x in os.listdir(os.curdir) if os.path.isdir(x) and \
             rdir_re.match(x) is not None]
rdirs.sort()

# Data to store for each run
entries = []

for rdir in rdirs:

    # Try to find the telescope.
    try:
        f = open(os.path.join(rdir, 'telescope'))
        telescope = f.readline().rstrip()
        f.close()
        print 'Run directory =',rdir,', telescope =',telescope
    except Exception, err:
        telescope = None
        print 'Run directory =',rdir,',',err

    # Now the night-by-night directories
    ndirs = [x for x in os.listdir(rdir) \
                 if os.path.isdir(os.path.join(rdir,x)) and \
                 ndir_re.match(x) is not None]
    ndirs.sort()

    # now to the night-by-night files
    for ndir in ndirs:

        npath = os.path.join(rdir, ndir)

        # Read night log (does not matter if none exists, although a warning
        # will be printed)
        nlog = Ultra.Log(os.path.join(npath, ndir + '.dat'))

        # Read timing data (does not matter if none exists, although a warning
        # will be printed)
        times = Ultra.Times(os.path.join(npath, ndir + '.times'))

        # Read XML files for this night
        dpath = os.path.join(npath, 'data')
        xmls = [os.path.join(dpath,x) for x in os.listdir(dpath) \
                    if xml_re.match(x) is not None]
        xmls.sort()
        first = True
        expose = 0.
        for xml in xmls:
            try:
                run = Ultra.Run(xml, nlog, times, targets, telescope, ndir,
                                rdir, sskip, True)

                # update targets to reduce simbad lookups
                if run.simbad:
                    if run.id in targets:
                        targets[run.id]['match'].append((run.target, True))
                    else:
                        targets[run.id] = {'ra' : subs.hms2d(run.ra), \
                                               'dec' : subs.hms2d(run.dec),
                                           'match' : [(run.target, True),]}
                elif run.id is None:
                    sskip.append(run.target)

                if run.ra is not None and run.dec is not None:
                    rah, decd, csys = subs.str2radec(run.ra + ' ' + run.dec)
                    entries.append(
                        {'ra' : rah, 'dec' : decd, 'run' : run.run,
                         'night' : run.night,
                         'num' : run.number, 'target' : run.target.strip(),
                         'id' : run.id.strip(),
                         'expose' : 0. if run.expose is None else \
                             float(run.expose)/60.,
                         'comment' : run.comment.strip()})

            except Exception, err:
                print 'XML error: ',err,'in',xml

print '\nFound',len(entries),'runs.'

with open('ultra.json','w') as fout:
    json.dump(entries, fout)

print 'Written to ultra.json'


