#!/usr/bin/env python

usage = \
"""
Script to generate html web pages ULTRASPEC or CAM

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
of target positions and regular expressions for translating targets in.
"""

import os, sys, re, argparse
import trm.subs as subs
#import traceback
import trm.simbad as simbad
import Ultra

cwd = os.getcwd()
if cwd.find('ultracam') > -1:
    instrument = 'ULTRACAM'
elif cwd.find('ultraspec') > -1:
    instrument = 'ULTRASPEC'
else:
    print 'Could not recognise instrument from ' + cwd
    print 'please fix.'
    exit(1)

# Start off with fixed strings for html output
GUIDE_HEAD = \
"""
<h3>{instrument} guide</h3>

<p>
<a class="search" href="#">Run and target search</a>

<p>
<a id="short" href="#">Brief</a>, <a id="long" href="#">Full</a>.

<p>
Observing runs:<br>

"""

LOG_FOOT = \
"""
<p>
<table>
<tr><th class="left">Auto ID</th><td class="left">automated look-up of positions
through a combination of string matching and SIMBAD lookups, this shows
the identified object name</td></tr>
<tr><th class="left">Dwell</th><td class="left">total time on target</td></tr>
<tr><th class="left">Cycle</th><td class="left">time from one exposure to the next</td></tr>
<tr><th class="left">Spd</th><td class="left">readout speed. ULTRACAM: 'cdd' =  slow, 'fbb' =
fast. ULTRASPEC: 'S', 'F' for slow and fast. Bias frames must match the data in
this parameter.</td></tr>
<tr><th class="left">Opt</th><td class="left">ULTRASPEC: output, 'N' normal, 'A' avalanche.</td></tr>
<tr><th class="left">Gn</th><td class="left">ULTRASPEC: avalanche gain setting.</td></tr>
</table>
"""

# Main program starts here

if __name__ == '__main__':

    # arguments
    parser = argparse.ArgumentParser(description='Compiles web pages for ULTRASPEC/CAM logs')

    parser.add_argument('-r', dest='rdir', default=None,
                        help='name of run directory (all will be searched otherwise)')

    # optional
    parser.add_argument('-a', dest='all', action="store_true", default=False,
                        help='all pages are recreated. Default is only missing pages')
    # parse them
    args = parser.parse_args()

    # Basic aim is to read as many xml and data files as possible. Errors are
    # converted to warnings if possible and ideally the user should fix up the
    # files until no such warnings appear. html files are made on a
    # night-by-night basis.

    # First read target data.
    targets = Ultra.Targets('TARGETS', 'AUTO_TARGETS')

    # File matching regular expressions
    rdir_re = re.compile('^\d\d\d\d-\d\d$') # Search for YYYY-MM
    ndir_re = re.compile('^\d\d\d\d-\d\d-\d\d$') # Search for night directories
    xml_re  = re.compile('run\d\d\d\.xml$') # Search for xml files

    # Targets to skip Simbad searches for; will be added to as more failures are found ensuring
    # that searches for a given target are only made once.
    with open('SKIP_TARGETS') as fp:
        sskip = fp.readlines()
        sskip = [name.strip() for name in sskip if not name.startswith('#')]

    # Create a list directories of runs to search through
    if args.rdir:
        if not os.path.isdir(args.rdir):
            print args.rdir,'is not a directory or does not exist'
            exit(1)
        if not rdir_re.match(args.rdir):
            print args.rdir,'dies not have the required YYYY-MM format'
            exit(1)
        rdirs = [args.rdir,]
    else:
        rdirs = [x for x in os.listdir(os.curdir) if os.path.isdir(x) and rdir_re.match(x) is not None]
    rdirs.sort()

    # Write the guide
    with open('guide.html', 'w') as fg:
        fg.write(GUIDE_HEAD.format(instrument=instrument))

        for rdir in rdirs:
            fg.write('\n<p>\n')
            year,month = rdir.split('-')
            mname = subs.int2month(int(month))
            id = mname + '_' + year
            fg.write('<a class="run" id="' + id + '" href="#">' + mname + ' ' + year + '</a><br>\n')
            fg.write('<div class="details" id="' + id + '_details">\n')
            fg.write('<ul>\n')

            # Now the night-by-night directories
            ndirs = [x for x in os.listdir(rdir) if os.path.isdir(os.path.join(rdir,x)) and \
                         ndir_re.match(x) is not None]
            ndirs.sort()

            for ndir in ndirs:
                fg.write('<li> <a class="night" id="' + ndir + '" href="#">' + ndir + '</a></li>\n')
            fg.write('</ul>\n</div>\n')

    fg.close()
    print 'Written guide to guide.html'

    # Now the logs.
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
        ndirs = [x for x in os.listdir(rdir) if os.path.isdir(os.path.join(rdir,x)) and \
                     ndir_re.match(x) is not None]
        ndirs.sort()

        runs  = []

        # now to the night-by-night files
        for inight,ndir in enumerate(ndirs):

            npath = os.path.join(rdir, ndir)

            # Read night log (does not matter if none exists, although a warning will be printed)
            nlog = Ultra.Log(os.path.join(npath, ndir + '.dat'))

            # Read timing data (does not matter if none exists, although a warning will be printed)
            times = Ultra.Times(os.path.join(npath, ndir + '.times'))

            # Start off html log for the night
            htlog = os.path.join(npath, ndir + '.html')
            if os.path.exists(htlog) and not args.all:
                continue

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

                    if first:
                        previous = ndirs[inight-1] if inight > 0 else None
                        next     = ndirs[inight+1] if inight < len(ndirs)-1 else None
                        fh.write('\n' + run.html_start(previous, next) + '\n')
                        first = False

                    fh.write('\n' + run.html_table_row() + '\n')

                    expose += float(run.expose) if run.expose is not None and run.expose != ' ' else 0.
                    runs.append(run)

                except Exception, err:
                    print 'XML error: ',err,'in',xml

            # Shut down html file
            fh.write('</table>\n\n' + '<p>Total exposure time = ' + str(int(100.*expose/3600.)/100.) + ' hours.<br>\n')
            fh.write(LOG_FOOT)
            fh.close()

    # Write out newly added / modified targets to disk to save future simbad lookups
    if len(Ultra.sims):

        # These have to be appended to the AUTO_TARGETS file. So we read them in again
        ntargs = Ultra.Targets('AUTO_TARGETS')

        if os.path.exists('AUTO_TARGETS'):
            os.rename('AUTO_TARGETS', 'AUTO_TARGETS.old')

        # add in the new ones
        for rid, names in Ultra.sims.iteritems():
            entry = targets[rid]
            ntargs[rid] = {'ra' : entry['ra'], 'dec' : entry['dec'], 'names' : names}

        ntargs.write('AUTO_TARGETS')

        print 'Total of',len(Ultra.sims),'targets identified by Simbad.'
        print 'Written to AUTO_TARGETS to save on future lookups.'

    else:
        print 'There were no targets identified using Simbad'
        print 'No new AUTO_TARGETS file was written.'

    # write list of failures
    if len(Ultra.failures):
        with open('FAILED_TARGETS','w') as fobj:
            fobj.write('# The following have no IDs:\n')
            for name, value in Ultra.failures.iteritems():
                fobj.write('%-32s %s %s run%03d' % (name.replace(' ','~'),value[0],value[1],value[2]) + '\n')

        print 'Wrote',len(Ultra.failures),'names to FAILED_TARGETS'
    else:
        print 'There were no targets that could not be identified.'
        print 'No new FAILED_TARGETS file was written.'

