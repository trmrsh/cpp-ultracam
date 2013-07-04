#!/usr/bin/env python

# !!begin
# !!title    texport
# !!author   T.R. Marsh
# !!created  12 June 2012
# !!root     texport
# !!index    texport
# !!descr    makes all runs compatible with a given target available for download
# !!class    Scripts
# !!css      style.css
# !!head1    texport makes all runs compatible with a particular run ID available for download
#
# !!emph{texport} searches through an ultracam run directory for all ULTRACAM runs compatible
# with a specified target and then creates a set of directories and copies the files
# connected to those it identifies. The script is quite restrictive about where it can
# be run to guard against irritating pollution with hard links. If you try it from the 
# wrong directory it will let you know. [NB. This is not a general user script.] The script
# uses much of the same code used to generate the data logs and runs off the same directories,
# so every effort must have been made to get the logs into shape before running it.
#
# !!head2 Invocation
#
# texport rundir edir target
#
# !!head2 Arguments
#
# !!table
#
# !!arg{rundir}{The run in question, identified by a string of the form YYYY-MM, e.g. 2006-03}
# !!arg{edir}{Data export directory. This will be a sub-directory of the
#  directory from where the script is run.}
# !!arg{target}{String identifying the target (enclose in quotes if it has blanks). This will be treated
# as a regular expression to allow flexible matching. The matching will be made to the formal ID name of the
# target which should be more consistent than the name given at the telescope.}
# !!table
#
# !!end

import sys, os, re, shutil
import Ultra
import trm.subs as subs

fwd = '/storage/astro2/www/phsaap/data'

wd = os.getcwd()    
if wd != fwd:
    print 'You must run this script from ' + fwd
    print 'The export directory must be a sub-directory of this one'
    exit(1)

if len(sys.argv) != 4:
    print 'usage: rundir edir target'
    exit(1)

# name the arguments
rdir, edir, target   = sys.argv[1:4]

uroot   = '/storage/astro1/phsaap/ultracam'

log_dir = os.environ['ULTRACAM_LOGS']
raw_dir = os.environ['ULTRACAM_RAW']

dtest = re.compile('^\d\d\d\d-\d\d$')
if dtest.match(rdir):
    rundir = os.path.join(log_dir, rdir)
else:
    print 'Run = ' + rdir + ' does not have the form YYYY-MM'
    exit(1)

if os.path.exists(edir) and not os.path.isdir(edir):
    print edit,'exists but is not a directory; please fix.'
    exit(1)
elif not os.path.exists(edir):
    os.makedirs(edir)
    print 'Created directory =',edir

# load all runs of the given run
runs = Ultra.load_runs(rdir)
                
print 'Read',len(runs),'runs.'

# look for all runs which match the supplied target
tre = re.compile(target)
matches = {}
for i,run in enumerate(runs):
    if run.id is not None and tre.search(run.id):
        matches[run.night + ('%03d' % run.number)] = i
        print 'Match found: night =',run.night,', number =',run.number,', target =',run.target

if len(matches) == 0:
    print 'No matches found. Stopping immediately'
    exit(1)
else:
    print 'Found',len(matches),'matches.'

# copy these
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

# get a list of night-by-night directories
ndirs = [d for d in os.listdir(rundir) if os.path.isdir(os.path.join(rundir, d))]
ndirs.sort()

# Create export directory names and then create the directories
edirs = [os.path.join(edir, rdir, night) for night in ndirs]

for ed in edirs:
    if not os.path.exists(ed):
        os.makedirs(ed)

## Create hard links 
#for run in runs:
#    r = 'run' + ('%03d' % (run.number,))
#    source_xml = os.path.join(raw_dir, run.night, r + '.xml')
#    source_dat = os.path.join(raw_dir, run.night, r + '.dat')
#    link_xml   = os.path.join(edir, rdir, run.night, r  + '.xml')
#    link_dat   = os.path.join(edir, rdir, run.night, r + '.dat')
#
#    if os.path.exists(source_xml):
#        if os.path.exists(link_xml):
#            print link_xml,'already exists and will not be over-written'
#        else:
#            os.link(source_xml, link_xml)
#
#    if os.path.exists(source_dat):
#        if os.path.exists(link_dat):
#            print link_dat,'already exists and will not be over-written'
#        else:
#            os.link(source_dat, link_dat)

# Copy files
for run in runs:
    r = 'run' + ('%03d' % (run.number,))
    source_xml = os.path.join(raw_dir, run.night, r + '.xml')
    source_dat = os.path.join(raw_dir, run.night, r + '.dat')
    copy_xml   = os.path.join(edir, rdir, run.night, r  + '.xml')
    copy_dat   = os.path.join(edir, rdir, run.night, r + '.dat')

    if os.path.exists(source_xml):
        if os.path.exists(copy_xml):
            print copy_xml,'already exists and will not be over-written'
        else:
            shutil.copyfile(source_xml, copy_xml)
            print 'Copied',source_xml,'to',copy_xml

    if os.path.exists(source_dat):
        if os.path.exists(copy_dat):
            print copy_dat,'already exists and will not be over-written'
        else:
            shutil.copyfile(source_dat, copy_dat)
            print 'Copied',source_dat,'to',copy_dat

# Write out a log file
f = open(os.path.join(edir, 'index.html'), 'w')

f.write("""
<html>
<head>
<title>ULTRACAM/ULTRASPEC log</title>
<link rel="stylesheet" type="text/css" href="../ultracam_logs.css" />
</head>
<body>
""")

f.write('<h1>Log file of run identified by matching to target = %s</h1>' % target)

f.write("""

<p>
You will find below a log of runs identified as matching the target name supplied
above (as a regular expression) along with all compatible calibration frames. 
The log below is partial; see the 
<a href="http://deneb.astro.warwick.ac.uk/phsaap/ultracam/logs/index.html">data
logs</a> for full information (username ultracam, password blencathra). The
runs below were identified automatically according to header values and window
formats. It is possible that the headers could be in error, so don't hesitate
to contact me if you think there might be anything wrong. To help you download
your data, here is a <a href="links.lis">list of links</a> that can be used as
input for 'wget', e.g.  
<pre> 
wget -x -nH --cut-dirs=3 -i links.lis 
</pre> 
where links.lis is the name of the list of
links that you have saved to disk. However, <strong>do not immediately start this</strong> because it
is highly likely that you can save yourself lots of time if you edit the list
of links to avoid downloading files that you don't think that you will need
(you can always pick them up later if you change your mind). There will
typically be <strong>many</strong> more calibrations than you will strictly need, 
for instance because the routine searches over all nights for compatible frames and every
unbinned full-frame run will be located for example. It tries to err on the
inclusive side, but this means that there may well be many more files than you
require. Thus your first task should be to prune the <a
href="links.lis">list of links</a>.  In the list below file sizes are included
so that you can judge this for yourself and links are given so that you can
individually download files if need be.

<p>
If you find that you need to repeat a download but want to avoid copying files that 
you already have, then you should check out the '-nc' option of 'wget'.

<p>
For each file below there is a link 'X' that points to the equivalent .xml header file and 'D' 
that points to the data file. You will need both to be able to use the data, and they should be 
stored within the same directory. The XML files are small while the data files are large. 
The <a href="links.lis">list of links</a> contains both the xml and data links.

<p> Once you have downloaded what you want, also save this page to keep as a
log. The rules within the table separate runs from different nights.

<p>
<table cellpadding="2" cellspacing="2" rules="groups">

<thead>
<tr><th>Links<th>Size<br>(MB)</th><th>Night</th><th>Run</th><th>Target</th><th>Auto ID</th>
<th>Date<br>Start of run</th><th>UT<br>start</th><th>UT<br>end</th>
<th>Dwell<br>sec.</th><th>Sample<br>sec.</th><th>Frame<br>no.</th>
<th>Filters</th><th>PID</th><th>PI</th><th>Comment</th></tr>
</thead>

<tbody>
""")

server = 'http://deneb.astro.warwick.ac.uk/phsaap/data/'

nold = 'None'

for run in runs:
    root = os.path.join(edir, rdir, run.night, 'run' + ('%03d' % (run.number,)))

    if nold == 'None':
        nold = run.night
    elif nold != run.night:
        nold = run.night
        f.write('</tbody>\n\n<tbody>\n')

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
</tbody>

</table>
</body>
</html>
""")
f.close()

# Write out wget targets
f = open(os.path.join(edir, 'links.lis'), 'w')

for run in runs:
    root = os.path.join(edir, rdir, run.night, 'run' + ('%03d' % (run.number,)))
    f.write(server + root + '.xml\n')
    f.write(server + root + '.dat\n')
f.close()


# Finally, copy the data:



