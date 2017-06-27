#!/usr/bin/env python

from __future__ import print_function

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

One other directory called 'Other' will be recognised as a run directory.
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
    print('Could not recognise instrument from ' + cwd)
    print('please fix.')
    exit(1)

# Start off with fixed strings for html output
GUIDE_HEAD = \
"""
<h3>{instrument} guide</h3>

"""

LOG_TITLE_HEAD = \
"""
<!DOCTYPE html>
<html>
<head>
<title>ULTRASPEC logs</title>

</head>

<body>
"""

LOG_HEAD = \
"""
<!DOCTYPE html>
<html>
<head>
<title>ULTRASPEC logs</title>

</head>

<body>
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

</body>
</html>
"""

# Main program starts here

if __name__ == '__main__':

    # arguments
    parser = argparse.ArgumentParser(
        description='Compiles web pages for ULTRASPEC/CAM logs')

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
    rdir_re = re.compile('^\d\d\d\d-\d\d$')      # Search for YYYY-MM
    ndir_re = re.compile('^\d\d\d\d-\d\d-\d\d$') # Search for night directories
    xml_re  = re.compile('run\d\d\d\.xml$')      # Search for xml files

    # Targets to skip Simbad searches for; will be added to as more failures
    # are found ensuring that searches for a given target are only made once.
    with open('SKIP_TARGETS') as fp:
        sskip = fp.readlines()
        sskip = [name.strip() for name in sskip if not name.startswith('#')]

    # Read in failed targets. This is a list of object name, run directory, night date and run
    # for each target that failed. The names are added to the list of targets to be skipped
    # but the details are preserved here so they can be written out into the FAILED_TARGETS
    # list at the end. If they are identified they can be edited out of the FAILED_TARGETS later
    failed = {}
    if os.path.isfile('FAILED_TARGETS'):
        nline = 0
        with open('FAILED_TARGETS') as fp:
            for line in fp:
                nline += 1
                if not line.startswith('#'):
                    try:
                        target,rdir,ndir,srun = line.split()
                        run = int(srun[3:])
                        failed[target] = (rdir, ndir, run)
                        sskip.append(target.replace('~',' '))
                    except Exception as err:
                        print('Error reading FAILED_TARGETS')
                        print('Line number',nline)
                        print('Line =',line)
                        exit(1)

        print('Loaded',len(failed),'targets from FAILED_TARGETS; will skip SIMBAD lookups for these.')
        print('If you want them to be re-tried, edit them out of FAILED_TARGETS')
    else:
        print('Did not find any FAILED_TARGETS list')

    # Create a list directories of runs to search through
    if args.rdir:
        if not os.path.isdir(args.rdir):
            print(args.rdir,'is not a directory or does not exist')
            exit(1)
        if not rdir_re.match(args.rdir):
            print(args.rdir,'dies not have the required YYYY-MM format')
            exit(1)
        rdirs = [args.rdir,]
    else:
        rdirs = [x for x in os.listdir(os.curdir) if os.path.isdir(x) and \
                 rdir_re.match(x) is not None]
        if os.path.isdir('Others'):
            rdirs.append('Others')

    rdirs.sort()

    # Write the guide
    with open('guide.html', 'w') as fg:
        fg.write(GUIDE_HEAD.format(instrument=instrument))

        fg.write('<p>\n<a href="ultra_search.html">Run search</a>\n');

        fg.write('<p>\nObserving runs:<br>\n');

        for rdir in rdirs:
            fg.write('\n<p>\n')
            if rdir == 'Others':
                run = 'Others'
            else:
                year,month = rdir.split('-')
                mname = subs.int2month(int(month))
                run = mname + ' ' + year

            fg.write('<a class="run" href="#" onclick="showHideNights(\'' +
                     run + '\')">' + run + '</a><br>\n')
            fg.write('<div class="details" id="guide_details_' + run + '">\n')
            fg.write('<ul>\n')

            # Now the night-by-night directories
            ndirs = [x for x in os.listdir(rdir) \
                     if os.path.isdir(os.path.join(rdir,x)) and \
                     ndir_re.match(x) is not None]
            ndirs.sort()

            for ndir in ndirs:
                fg.write('<li> <a class="night" id="guide_' + ndir + \
                         '" href="#">' + ndir + '</a></li>\n')
            fg.write('</ul>\n</div>\n')

    fg.close()
    print('Written guide to guide.html')

    # Now the logs.
    for rdir in rdirs:

        # Try to find the telescope.
        try:
            f = open(os.path.join(rdir, 'telescope'))
            telescope = f.readline().rstrip()
            f.close()
            print('Run directory =',rdir,', telescope =',telescope)
        except Exception as err:
            telescope = None
            print('Run directory =',rdir,',',err)

        # Now the night-by-night directories
        ndirs = [x for x in os.listdir(rdir) if os.path.isdir(os.path.join(rdir,x)) and \
                     ndir_re.match(x) is not None]
        ndirs.sort()

        runs  = []

        # now to the night-by-night files
        for inight,ndir in enumerate(ndirs):

            npath = os.path.join(rdir, ndir)

            # Read night log (does not matter if none exists, although a
            # warning will be printed)
            nlog = Ultra.Log(os.path.join(npath, ndir + '.dat'))

            # Read timing data (does not matter if none exists, although a
            # warning will be printed)
            times = Ultra.Times(os.path.join(npath, ndir + '.times'))

            # write heading for html log
            htlog = os.path.join(npath, ndir + '_title.html')
            if os.path.exists(htlog) and not args.all:
                continue

            print('Generating',htlog)
            with open(htlog, 'w') as fh:
                fh.write(LOG_TITLE_HEAD)

                # build up start with small table indicating the telescope and
                # instrument
                st = \
"""
<h1> Night of {0}</h1>

<p>
<table>
<tr><td class="left">Telescope: </td><td class="left">{1}</td></tr>
<tr><td class="left">Instrument:</td><td class="left">{2}</td></tr>
<tr><td class="left">Run ID:    </td><td class="left">{3}</td></tr>
</table>
""".format(ndir, telescope, instrument, rdir)

                st += \
"""
<p>
<a href="#" onclick="showBrief()">brief</a>,
<a href="#" onclick="showFull()">full</a>,
"""

                # now pointers to the previous and next nights. The words are always
                # there to make for convenient clicking through runs, but they won't
                # be high-lighted if there is no 'previous' or 'next'.
                if inight > 0:
                    st += '\n<a class="night" id="main_{0}" href="#">previous night</a>, '\
                        .format(ndirs[inight-1])
                else:
                    st  += '\nprevious night, '

                if inight < len(ndirs)-1:
                    st += '\n<a class="night" id="main_{0}" href="#">next night</a>.'\
                        .format(ndirs[inight+1])
                else:
                    st += 'next night.'
                st += \
"""

</body>
</html>
"""
                fh.write(st)

            # now the log with the run-by-run table
            htlog = os.path.join(npath, ndir + '_log.html')
            if os.path.exists(htlog) and not args.all:
                continue

            print('Generating',htlog)
            with open(htlog, 'w') as fh:
                fh.write(LOG_HEAD)
                fh.write('\n' + Ultra.Run.html_head(instrument) + '\n')

                # Read XML files for this night
                dpath = os.path.join(npath, 'data')
                xmls = [os.path.join(dpath,x) for x in os.listdir(dpath) \
                        if xml_re.match(x) is not None]
                xmls.sort()

                expose = 0.
                for nrun, xml in enumerate(xmls):
                    try:
                        run = Ultra.Run(xml, nlog, times, targets, telescope,
                                        ndir, rdir, sskip, True)
                        fh.write('\n' + run.html_table_row() + '\n')
                        expose += float(run.expose) if run.expose is not None and \
                                  run.expose != ' ' else 0.
                        runs.append(run)

                        # write extra header line
                        if (nrun + 1) % 20 == 0 and nrun < len(xmls)-1:
                            fh.write('\n' + Ultra.Run.html_head(instrument,False)
                                     + '\n')

                    except Exception as err:
                        print('XML error: ',err,'in',xml)

                # Shut down html file
                fh.write('</table>\n\n' + '<p>Total exposure time = ' + \
                         str(int(100.*expose/3600.)/100.) + ' hours.<br>\n')
                fh.write(LOG_FOOT)

    # Write out newly added / modified targets to disk to save future simbad
    # lookups
    if len(Ultra.sims):

        # These have to be appended to the AUTO_TARGETS file. So we read them
        # in again
        ntargs = Ultra.Targets('AUTO_TARGETS')

        if os.path.exists('AUTO_TARGETS'):
            os.rename('AUTO_TARGETS', 'AUTO_TARGETS.old')

        # add in the new ones
        for rid, names in Ultra.sims.items():
            entry = targets[rid]
            ntargs[rid] = {'ra' : entry['ra'], 'dec' : entry['dec'], 'names' : names}

        ntargs.write('AUTO_TARGETS')

        print('Total of',len(Ultra.sims),'targets identified by Simbad.')
        print('Written to AUTO_TARGETS to save on future lookups.')

    else:
        print('There were no targets identified using Simbad')
        print('No new AUTO_TARGETS file was written.')

    # write list of failures to FAILED_TARGET, including
    # those loaded from the same file at the start.
    if len(Ultra.failures) or len(failed):
        with open('FAILED_TARGETS','w') as fobj:
            fobj.write('# The following have no IDs:\n')
            for name, value in Ultra.failures.items():
                fobj.write('{0:<32s} {1:s} {2:s} run{3:03d}\n'.format(
                        name.replace(' ','~'),value[0],value[1],value[2]))

            for name, value in failed.items():
                fobj.write('{0:<32s} {1:s} {2:s} run{3:03d}\n'.format(
                        name.replace(' ','~'),value[0],value[1],value[2]))

        print('Wrote',len(Ultra.failures)+len(failed),'names to FAILED_TARGETS')
    else:
        print('There were no targets that could not be identified.')
        print('No new FAILED_TARGETS file was written.')

