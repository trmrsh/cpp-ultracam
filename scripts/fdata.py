#!/usr/bin/env python

# !!begin
# !!title    fdata
# !!author   T.R. Marsh
# !!created  10 July 2007
# !!revised  25 April 2008
# !!root     fdata
# !!index    fdata
# !!descr    finds all runs compatible with a particular run ID.
# !!class    Scripts
# !!css      style.css
# !!head1    fdata finds all runs compatible with a particular run ID
#
# !!emph{fmatch} searches an ultracam run directory for all ULTRACAM runs compatible
# with a specified run ID and then creates a set of directories and hard linked files
# connected to those it identifies. The script is quite restrictive about where it can
# be run to guard against irritating pollution with hard links.
#
# !!head2 Invocation
#
# fdata run id export
#
# !!head2 Arguments
#
# !!table
#
# !!arg{run}{The run in question, identified by a string of the form YYYY-MM, e.g. 2006-03}
# !!arg{id}{String identifying the data, i.e. a programme ID code. A regular expression match is applied so with
# some fiddling you should be able to allow for mistakes.}
# !!arg{expdir}{Name for the top-level of the exported directories which will be created in the directory that
# this script is invoked from, so be careful.}
#
# !!table
#
# !!end

import sys
import os
import re
import ultracam
import ultraspec

fwd = '/storage/astro2/www/phsaap/data'
wd = os.getcwd()    
if wd != fwd:
    print 'You must run this script from ' + fwd
    print 'The export directory must be a sub-directory of this one'

if len(sys.argv) != 4:
    print 'usage: rundir id expdir'
    exit(1)

# name the arguments
rdir   = sys.argv[1]
ident  = sys.argv[2]
expdir = sys.argv[3]

log_dir = '/storage/astro2/phsaap/ultracam/logs'

dtest = re.compile('\d\d\d\d-\d\d')
if dtest.match(rdir):
    rundir = os.path.join(log_dir, rdir)
else:
    print 'Run = ' + rdir + ' does not have the form YYYY-MM'
    exit(1)

if not os.path.isdir(rundir):
    print 'Run directory = %s does not exist' % rundir
    exit(1)

if expdir.find('/') > -1:
    print 'Export directory = %s is not a simple sub-directory' % expdir
    exit(1)

if not os.path.isdir(expdir):
    print 'Export directory = %s does not exist' % expdir
    exit(1)

def fdata(top, dirs):
    """
    Recursively find all data directories, i.e. any directory ending in data
    """
    for f in os.listdir(top):
        pathname = os.path.join(top, f)
        if os.path.isdir(pathname):
            if pathname.endswith('data'):
                dirs.append(pathname)
            fdata(pathname, dirs)

dirs = []
fdata(rundir, dirs)

first = True
# now read the directories loading the formats of all files of the form 'run[0-9][0-9][0-9].xml'
# also see if there are equivalent '.dat' and '.times' files present from which to get comments
rtest = re.compile('run[0-9][0-9][0-9]\.xml')
form ={}
for d in dirs:
    runs = [rn for rn in os.listdir(d) if rtest.match(rn)]
    for rn in runs:
        fpath = os.path.join(d,rn)
        if first:
            try:
                form[fpath[0:fpath.rfind('.xml')]] = ultracam.Ultracam(fpath)
                dtype = 'ULTRACAM'
            except ultracam.NotUltracamError:
                form[fpath[0:fpath.rfind('.xml')]] = ultraspec.Ultraspec(fpath)
                dtype = 'ULTRASPEC'
            first = False
        else:
            if dtype == 'ULTRACAM':
                form[fpath[0:fpath.rfind('.xml')]] = ultracam.Ultracam(fpath)
            else:
                form[fpath[0:fpath.rfind('.xml')]] = ultraspec.Ultraspec(fpath)
                
# look for logfile
    dup = d[0:d.rfind('/data')]
    logfile = dup + dup[dup.rfind('/'):] + '.dat'
    if os.path.isfile(logfile):
        f = open(logfile)
        for line in f:
            if line.startswith('run'):
                key     = os.path.join(d,line[0:6])
                if key in form:
                    form[key]['Comment'] = line[6:].strip()
        f.close()

# look for file of times
    tfile = dup + dup[dup.rfind('/'):] + '.times'
    if os.path.isfile(tfile):
        f = open(tfile)
        for line in f:
            if line.startswith('run'):
                (key,date,utstart,utend,nframe,exposure,sample) = line.split()
                key  = os.path.join(d,key)
                if key in form:
                    form[key]['Date']     = date
                    form[key]['UTstart']  = utstart
                    form[key]['UTend']    = utend
                    form[key]['nframe']   = nframe
                    form[key]['exposure'] = exposure
                    form[key]['sample']   = sample
        f.close()

# look for all runs which match the supplied identifier
match = {}
for (key,value) in form.iteritems():
    if value.match_id(ident):
        match[key] = value

# now compile list of all calibrations that match the runs located
all = {}
for (tkey,template) in match.iteritems():
    for (key,ucam) in form.iteritems():
        if key == tkey:
            all[key] = ucam
        elif ucam.is_calib() and template.is_subset_of(ucam):
            all[key] = ucam

# now a second pass to pick up suitable biases for flats and darks
# because the biases picked up in the first pass had to match the read speed
# of the test data while other calibrations did not.
final = {}
for (tkey,template) in all.iteritems():
    for (key,ucam) in form.iteritems():
        if key == tkey:
            final[key] = ucam
        elif template.is_not_bias() and template.is_calib() and ucam.is_bias() and template.is_subset_of(ucam):
            final[key] = ucam


# Create export directory names and then create the directories
dirs = [dname[len(log_dir)+1:dname.rfind('/data')] for dname in dirs]

for dname in dirs:
    pathname = os.path.join(expdir, dname)
    if not os.path.isdir(pathname):
        os.makedirs(pathname)

# Create hard links 
for key in sorted(final.keys()):
    newfile = os.path.join(expdir, key[len(log_dir)+1:key.find('/data')] + key[key.find('/data')+5:])
    if not os.path.isfile(newfile + '.xml'):
        os.link(key + '.xml', newfile + '.xml')
    if not os.path.isfile(newfile + '.dat'):
        os.link(key + '.dat', newfile + '.dat')

# Write out a log file
f = open(os.path.join(expdir, 'index.html'), 'w')

f.write("""
<html>
<head>
<title>ULTRACAM/ULTRASPEC log</title>
<link rel="stylesheet" type="text/css" href="ultracam_logs.css" />
</head>
<body>
""")

f.write('<h1>Log file of run identified by matching to ID = %s</h1>' % ident)

f.write("""

<p>
Dear Ultracam/Ultraspec user,

<p>
you will find below a log of runs identified as either belonging to you or as calibration files
compatible with your data. These were identified automatically according to header values and window 
formats. It is possible that the headers (to some extent entered by us at the telescope) could be in 
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

<tr>
<th>
<th>
<th>Size
<th>Run
""")

if dtype == 'ULTRACAM':
    f.write(ultracam.first_header() + '\n</tr>\n')
    f.write('<tr>\n<th>\n<th>\n<th>MB\n<th>\n' + ultracam.second_header() + '\n</tr>\n\n')
else:
    f.write(ultraspec.first_header() + '\n</tr>\n')
    f.write('<tr>\n<th>\n<th>\n<th>MB\n<th>\n' + ultraspec.second_header() + '\n</tr>\n\n')

old = ""
for key in sorted(final.keys()):
    start = key[len(log_dir)+1:key.find('/data')]
    if start != old: 
        if old != "":
            f.write('\n\n<tr><td>&nbsp;</tr>\n')
        old = start
        
    f.write('\n\n<tr>\n')
    newname = start + key[key.find('/data')+5:]
    f.write('<td><a href="' + newname + '.xml' + '">X</a></td>')
    f.write('<td><a href="' + newname + '.dat' + '">D</a></td>')
    fname = os.path.join(expdir, newname + '.dat')
    fstat = os.stat(fname)
    mbytes = fstat.st_size/(1024*1024)
    f.write('<td>' + str(mbytes) + '</td>')
    f.write('<td>' + key[key.find('/data')+6:] + '</td>' + final[key].to_html_table())
    f.write('\n</tr>')

f.write("""

</table>
</body>
</html>
""")

# Write out wget targets
f = open(os.path.join(expdir, 'links.lis'), 'w')

server = 'http://deneb.astro.warwick.ac.uk/phsaap/data' + expdir[expdir.find('www/phsaap/data')+15:]

for key in sorted(final.keys()):
    newname = key[len(log_dir)+1:key.find('/data')] + key[key.find('/data')+5:]
    f.write(server + '/' + newname + '.xml\n')
    f.write(server + '/' + newname + '.dat\n')
            
