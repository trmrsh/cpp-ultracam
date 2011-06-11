#!/usr/bin/env python

# !!begin
# !!title    export
# !!author   T.R. Marsh
# !!created  10 July 2007
# !!revised  16 Feb 2011
# !!root     export
# !!index    export
# !!descr    makes all runs compatible with a particular run ID available for download
# !!class    Scripts
# !!css      style.css
# !!head1    export makes all runs compatible with a particular run ID available for download
#
# !!emph{export} searches an ultracam run directory for all ULTRACAM runs compatible
# with a specified run ID and then creates a set of directories and hard linked files
# connected to those it identifies. The script is quite restrictive about where it can
# be run to guard against irritating pollution with hard links. If you try it from the 
# wrong directory it will let you know. [NB. This is not a general user script.] The script
# uses much of the same code used to generate the data logs and runs off the same directories,
# so every effort must have been made to get the logs into shape before running it.
#
# !!head2 Invocation
#
# export rundir progid
#
# !!head2 Arguments
#
# !!table
#
# !!arg{rundir}{The run in question, identified by a string of the form YYYY-MM, e.g. 2006-03}
# !!arg{progid}{String identifying the data, i.e. a programme ID code. A regular expression match is applied so with
# some fiddling you should be able to allow for mistakes. This will also be used to define the top-level directory
# where the data will be stored.}
#
# !!table
#
# !!end

import sys, os, re
import Ultra
import trm.subs as subs

fwd = '/storage/astro2/www/phsaap/data'
wd = os.getcwd()    
if wd != fwd:
    print 'You must run this script from ' + fwd
    print 'The export directory must be a sub-directory of this one'

if len(sys.argv) != 3:
    print 'usage: rundir progid'
    exit(1)

# name the arguments
rdir   = sys.argv[1]
progid = sys.argv[2]

idtest = re.compile(progid)

log_dir = '/storage/astro2/phsaap/ultracam/logs'
raw_dir = '/storage/astro2/phsaap/ultracam/raw_data'

dtest = re.compile('^\d\d\d\d-\d\d$')
if dtest.match(rdir):
    rundir = os.path.join(log_dir, rdir)
else:
    print 'Run = ' + rdir + ' does not have the form YYYY-MM'
    exit(1)

if os.path.exists(progid) and not os.path.isdir(progid):
    print progid,'exists but is not a directory; please fix.'
    exit(1)
elif not os.path.exists(progid):
    os.makedirs(progid)
    print 'Created directory =',progid

# Try to find the telescope.
try:
    f = open(os.path.join(rundir, 'telescope'))
    telescope = f.readline().rstrip()
    f.close()
    print 'Run directory =',rdir,', telescope =',telescope
except Exception, err:
    telescope = None
    print 'Run directory =',rdir,',',err

# get a list of night-by-night directories
ndirs = [d for d in os.listdir(rundir) if os.path.isdir(os.path.join(rundir, d))]
ndirs.sort()

# now read the directories loading the formats of all files of the form 'run[0-9][0-9][0-9].xml'
# also see if there are equivalent '.dat' and '.times' files present from which to get comments

first = True
xtest = re.compile('run[0-9][0-9][0-9]\.xml')
form  = {}

# read target data
targets = Ultra.Targets(os.path.join(log_dir, 'TARGETS'), os.path.join(log_dir, 'AUTO_TARGETS'))

# Targets to skip Simbad searches for; will be added to as more failures are found ensuring
# that searches for a given target are only made once.
sskip = ['Pluto','GRB','32K','Test data', 'GPS LED', 'GRB or 32K']

runs = []
for ndir in ndirs:

    # path to night-by-night directory
    npath = os.path.join(rundir, ndir)

    # Read night log (does not matter if none exists, although a warning will be printed)
    nlog = Ultra.Log(os.path.join(npath, ndir + '.dat'))

    # Read timing data (does not matter if none exists, although a warning will be printed)
    times = Ultra.Times(os.path.join(npath, ndir + '.times'))

    # get list of xml files
    dpath = os.path.join(npath, 'data')
    xmls = [os.path.join(dpath, xml) for xml in os.listdir(dpath) if xtest.match(xml)]
    for xml in xmls:
        try:
            run = Ultra.Run(xml, nlog, times, targets, telescope, ndir, rdir, sskip, True)

            # update targets to reduce simbad lookups
            if run.simbad:
                if run.id in targets:
                    targets[run.id]['match'].append((run.target, True))
                else:
                    targets[run.id] = {'ra' : subs.hms2d(run.ra), 'dec' : subs.hms2d(run.dec), 'match' : [(run.target, True),]}
            elif run.id is None:
                sskip.append(run.target)

            # store all but power ons
            if run.is_not_power_onoff():
                runs.append(run)

        except Exception, err:
            print 'XML error: ',err,'in',xml
                
print 'Read',len(runs),'runs.'

# look for all runs which match the supplied identifier
matches = {}
for i,run in enumerate(runs):
    if run.pid is None or idtest.match(run.pid):
        matches[run.night + ('%03d' % run.number)] = i

science = dict(matches)

# look for all calibrations that match the runs located
for key,value in science.iteritems():
    rdat = runs[value]
    for i,rcal in enumerate(runs):
        if rcal.is_calib() and rcal >= rdat:
            matches[rcal.night + ('%03d' % rcal.number)] = i

keys = matches.keys()
keys.sort()

# Chop down runs
runs = [runs[matches[key]] for key in keys]

# Create export directory names and then create the directories
edirs = [os.path.join(progid, rdir, night) for night in ndirs]

for edir in edirs:
    if not os.path.exists(edir):
        os.makedirs(edir)
 
# Create hard links 
for run in runs:
    r = 'run' + ('%03d' % (run.number,))
    source_xml = os.path.join(raw_dir, run.night, r + '.xml')
    source_dat = os.path.join(raw_dir, run.night, r + '.dat')
    link_xml   = os.path.join(progid, rdir, run.night, r  + '.xml')
    link_dat   = os.path.join(progid, rdir, run.night, r + '.dat')

    if os.path.exists(source_xml):
        if os.path.exists(link_xml):
            print link_xml,'already exists and will not be over-written'
        else:
            os.link(source_xml, link_xml)

    if os.path.exists(source_dat):
        if os.path.exists(link_dat):
            print link_dat,'already exists and will not be over-written'
        else:
            os.link(source_dat, link_dat)

# Write out a log file
f = open(os.path.join(progid, 'index.html'), 'w')

f.write("""
<html>
<head>
<title>ULTRACAM/ULTRASPEC log</title>
<link rel="stylesheet" type="text/css" href="../ultracam_logs.css" />
</head>
<body>
""")

f.write('<h1>Log file of run identified by matching to ID = %s</h1>' % progid)

f.write("""

<p>
Dear Ultracam/Ultraspec user,

<p>
you will find below a log of runs identified as either belonging to you or as calibration files
compatible with your data. The log below is partial; see the
<a href="http://deneb.astro.warwick.ac.uk/phsaap/ultracam/logs/index.htm">data logs</a>
for full information. The runs below were identified automatically according to header values and window 
formats. It is possible that the headers could be in 
error, so don't hesitate to contact us if you think there might be anything wrong. To help you download
your data, here is a <a href="links.lis">list of links</a> that can be used as input for 'wget', e.g.
<pre>
wget --user=username --password=password -x -nH --cut-dirs=3 -i links.lis
</pre>
where links.lis is the name of the list of links that you have saved to disk
and we will send you the username and password. However, <strong>do not
immediately start this</strong> because it is highly likely that you can save
yourself lots of time if you edit the list of links to avoid downloading files
that you don't think that you will need (you can always pick them up later if
you change your mind). There will typically be many more calibrations than are
strictly needed, for instance because the routine searches over all nights for
compatible frames and every unbinned full-frame run will be located for
example. It tries to err on the inclusive side, but this means that there may
well be many more files than you strictly need. Thus your first task should be
to prune the <a href="links.lis">list of links</a>.
In the list below file sizes are included so that you can judge this for yourself
and links are given so that you can individually download files if need be.

<p>
If you find that you need to repeat a download but want to avoid copying files that 
you already have, then you should check out the '-nc' option of 'wget'.

<p>
If you think you need extinction coefficients but do not have the data to determine them, 
again contact us.

<p>
For each file below there is a link 'X' that points to the equivalent .xml header file and 'D' 
that points to the data file. You will need both to be able to use the data, and they should be 
stored within the same directory. The XML files are small while the data files are large. 
The <a href="links.lis">list of links</a> contains both the xml and data links.

<p>
Once you have downloaded what you want, also save this page to keep as a log. The blank lines separate different nights.

<p>
<table cellpadding="2" cellspacing="2">

<tr><th>Links<th>Size<br>(MB)</th><th>Night</th><th>Run</th><th>Target</th><th>Auto ID</th>
<th>Date<br>Start of run</th><th>UT<br>start</th><th>UT<br>end</th>
<th>Dwell<br>sec.</th><th>Sample<br>sec.</th><th>Frame<br>no.</th>
<th>Filters</th><th>PID</th><th>PI</th><th>Comment</th></tr>
""")

server = 'http://deneb.astro.warwick.ac.uk/phsaap/data/'

for run in runs:
    root = os.path.join(progid, rdir, run.night, 'run' + ('%03d' % (run.number,)))

    s  = '<tr><td><a href="' + server + root + '.xml' + '">X</a>, '
    s += '<a href="' + server + root + '.dat' + '">D</a></td>'

    fname = root + '.dat'
    if os.path.exists(fname):                        
        fstat = os.stat(fname)
        mbytes = fstat.st_size // (1024*1024)
    else:
        mbytes = run.size() // (1024*1024)

    s += Ultra.td(str(mbytes))
    s += Ultra.td(run.night) 
    s += Ultra.td('%03d' % run.number)
    s += Ultra.td(run.target,'left')
    s += Ultra.td(run.id,'left')
    s += Ultra.td(run.date)
    s += Ultra.td(run.utstart)
    s += Ultra.td(run.utend)
    s += Ultra.td('%6.1f' % float(run.expose) if run.expose is not None else None, 'right')
    s += Ultra.td('%7.3f' % float(run.sample) if run.sample is not None else None, 'right')
    s += Ultra.td(run.nframe, 'right')
    s += Ultra.td(run.filters)
    s += Ultra.td(run.pid) 
    s += Ultra.td(run.pi)
    s += Ultra.td(run.comment,'left')
    s += '</tr>\n'
    f.write(s)

f.write("""

</table>
</body>
</html>
""")
f.close()

# Write out wget targets
f = open(os.path.join(progid, 'links.lis'), 'w')

for run in runs:
    root = os.path.join(progid, rdir, run.night, 'run' + ('%03d' % (run.number,)))
    f.write(server + root + '.xml\n')
    f.write(server + root + '.dat\n')
f.close()