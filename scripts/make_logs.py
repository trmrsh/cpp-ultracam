#!/usr/bin/env python

"""
Script to generate html web pages for ULTRACAM and ULTRASPEC. This is 
to upgrade and replace the old perl-based ones.

It should be run in the 'logs' directory which has sub-directories of 
the form '2005-11' (Nov 2005) which have a structure like so:

2005-11
  telescope   -- name of telescope
  2005-11-23  -- directory for 23 Nov
    2005-11-23.dat   -- night log file
    2005-11-23.times -- timing data
    data      -- run .dat and .xml files
  2005-11-24  -- directory for 24 Nov
    2005-11-24.dat   -- night log file
    2005-11-24.times -- timing data
    data      -- run .dat and .xml files

etc. It also expects there to be a file called TARGETS with information
of target positions and regular expressions for translating targets in
"""

import os, sys, re
from xml.dom import Node
from xml.dom.minidom import parse, parseString
import trm.subs as subs
#import traceback
import Ultra
import trm.simbad as simbad

# The main program

# Basic aim is to read as many xml and data files as possible. Errors
# are converted to warnings if possible and ideally the user should fix up the files
# until no such warnings appear. html files are made on a night-by-night
# basis.

# First read target data.
targets = Ultra.Targets('TARGETS', 'AUTO_TARGETS')

# File matching regular expressions
rdir_re = re.compile('^\d\d\d\d-\d\d$') # Search for YYYY-MM
ndir_re = re.compile('^\d\d\d\d-\d\d-\d\d$') # Search for night directories
xml_re  = re.compile('run\d\d\d\.xml$') # Search for xml files

# Targets to skip Simbad searches for; will be added to as more failures are found ensuring
# that searches for a given target are only made once.
sskip = ['Pluto','GRB','32K','Test data', 'GPS LED', 'GRB or 32K']

# Create a list directories of runs to search through
rdirs = [x for x in os.listdir(os.curdir) if os.path.isdir(x) and rdir_re.match(x) is not None]
rdirs.sort()

# to keep track of those targets IDed from Simbad
sims = []
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
    ndirs = [x for x in os.listdir(rdir) if os.path.isdir(os.path.join(rdir,x)) and ndir_re.match(x) is not None]
    ndirs.sort()

    # Write a guide
    fg = open(os.path.join(rdir, 'guide.htm'), 'w')
    fg.write("""
<html>
<head>
<link rel="stylesheet" type="text/css" href="../ultracam_test.css" />
</head>
<body>
""")

    for rd in rdirs:
        (year,month) = rd.split('-')
        fg.write('<p>\n<a href="../' + rd + '/guide.htm">' + subs.int2month(int(month)) + ' ' + year + '</a><br>\n')
        if rd == rdir:
            fg.write('<ul>\n')
            for ndir in ndirs:
                fg.write('<li> <a href="' + ndir + '/' + ndir + '.htm' + '" target="dynamic">' + ndir + '</a></li>\n')
            fg.write('</ul>\n')

    fg.write('</body>\n</html>\n')
    fg.close()

    runs  = []
    
    # now to the night-by-night files
    for ndir in ndirs:

        npath = os.path.join(rdir, ndir)
            
        # Read night log (does not matter if none exists, although a warning will be printed)
        nlog = Ultra.Log(os.path.join(npath, ndir + '.dat'))

        # Read timing data (does not matter if none exists, although a warning will be printed)
        times = Ultra.Times(os.path.join(npath, ndir + '.times'))

        # Start off html log file for the night
        htlog = os.path.join(npath, ndir + '.htm')
        if os.path.exists(htlog): continue

        print 'Generating',htlog
        fh = open(htlog, 'w')

        # Read XML files for this night
        dpath = os.path.join(npath, 'data')
        xmls = [os.path.join(dpath,x) for x in os.listdir(dpath) if xml_re.match(x) is not None]
        xmls.sort()
        first = True
        expose = 0.
        for xml in xmls:
            try:
                run = Ultra.Run(xml, nlog, times, targets, telescope, ndir, rdir, sskip, True)

                # update targets to reduce simbad lookups
                if run.simbad:
                    if run.id in targets:
                        targets[run.id]['match'].append((run.target, True))
                    else:
                        targets[run.id] = {'ra' : subs.hms2d(run.ra), 'dec' : subs.hms2d(run.dec), 'match' : [(run.target, True),]}
                        sims.append(run.id)
                elif run.id is None:
                    sskip.append(run.target)

                if first:
                    fh.write('\n' + run.html_start() + '\n')
                    first = False
                fh.write('\n' + run.html_table_row() + '\n')            
                expose += float(run.expose) if run.expose is not None and run.expose != ' ' else 0.
                runs.append(run)
            except Exception, err:
                print 'XML error: ',err,'in',xml

        # Shut down html file
        fh.write('</table>\n\n' + '<p>Total exposure time = ' + str(int(100.*expose/3600.)/100.) + ' hours\n')
        fh.write('</body>\n</html>')
        fh.close()

# Write out all 'exact' targets to disk to save future simbad lookups
if len(sims):

    # These have to be appended to the AUTO_TARGETS file. So we read them in again
    ntargs = Ultra.Targets('AUTO_TARGETS')

    if os.path.exists('AUTO_TARGETS'):
        os.rename('AUTO_TARGETS', 'AUTO_TARGETS.old')

    # add in the new ones
    for rid in sims:
        ntargs[rid] = targets[rid]

    ntargs.write('AUTO_TARGETS')

    print 'Total of',len(sims),'targets identified by Simbad.'
    print 'Written to AUTO_TARGETS to save future lookups.'

