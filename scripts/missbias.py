#!/usr/bin/env python

# !!begin
# !!title    missbias
# !!author   T.R. Marsh
# !!created  23 June 2007
# !!revised  29 April 2010
# !!root     missbias
# !!index    missbias
# !!descr    Lists all runs missing corresponding biases
# !!class    Scripts
# !!class    Observing
# !!css      style.css
# !!head1    missbias lists all runs missing corresponding biases
#
# !!emph{missbias} reads all the runs in the directories specified and tries to work out if there
# are any non-biases without corresponding biases. This is a crude test and does not verify that
# runs identified as 'Bias' are what they say they are or that they are any good. As well as the
# directories specified, the script also looks for subdirectories call 'data'
#
# The script invoked is a python script; if you use it will automatically attempt to
# generate 'byte compiled' versions ending in .pyc in the same directory as the script in order
# to speed up startup. It can only do so if you have write access to the installation directory.
# If the script is slow, this may be the cause.
#
# !!head2 Invocation
#
# unique directory
#
# !!head2 Arguments
#
# !!table
#
# !!arg{dir1, dir2, ..}{List of directories containing the .xml files, e.g. all the nights of the run}
#
# !!table
#
# Related commands: !!ref{fmatch.html}{fmatch}, !!ref{unique.html}{unique}
#
# !!end

import os, re, Ultra
from optparse import OptionParser

# define and parse options
usage = """
usage: %prog [options] -- looks for ULTRACAM / ULTRASPEC runs without biases

Arguments:

dir1 dir2 -- series of directories to look through.

"""

parser = OptionParser(usage)
parser.add_option("-f", "--fussy", dest="fussy", default=False, action="store_true",\
                  help="fussy test so that avalanche gain differences will be picked up")
parser.add_option("-i", "--ignore", dest="ignore", default=False, action="store_true",\
                  help="ignore runs marked 'data caution' when listing runs without biases.")

(options, args) = parser.parse_args()

if len(args) == 0:
    print 'You must supply some directories with run###.xml files'
    exit(1)

dirs = [d for d in args if os.path.isdir(d)]

Ultra.Run.Fussy = options.fussy
ignore = options.ignore

run_re = re.compile('^run[0-9][0-9][0-9]\.xml$')

# Accumulate a list of unique biases and non-biases, skipping power ons & offs
nonbias = {}
bias    = {}

for dir in dirs:
    xmls = [rn for rn in os.listdir(dir) if run_re.match(rn) != None]

    for xml in xmls:
        run = Ultra.Run(os.path.join(dir,xml), noid=True)

        if run.is_not_power_onoff():

            if run.is_bias():
                # compare with already stored formats
                new_format = True
                for rn, rold in bias.iteritems():
                    if run == rold:
                        new_format = False
                        break
            
                if new_format:
                    bias[os.path.join(dir,xml[0:xml.rfind('.xml')])] = run

            else:
                # compare with already stored formats
                new_format = True
                for rn, rold in nonbias.iteritems():
                    if run == rold:
                        if rold.flag == 'data caution' and run.flag != 'data caution':
                            del nonbias[rn]
                        else:
                            new_format = False
                        break
            
                if new_format:
                    nonbias[os.path.join(dir,xml[0:xml.rfind('.xml')])] = run

remove = []
for rn, run in nonbias.iteritems():
    match = False

    for rnb, runb in bias.iteritems():
        if run == runb:
            match = True
            break
    if match: remove.append(rn)

# Remove ones that seem to have biases:
for run in remove:
    del nonbias[run]


# List any that apparently are without biases

print '\nRuns without apparent biases are as follows:\n'

for run in sorted(nonbias.keys()):
    if not ignore or nonbias[run].flag != 'data caution':
        print '%-20s %s' % (run,nonbias[run])

if not ignore:
    print '\nRuns marked "[data caution]" are probably acquisition runs which don\'t need biases; use -i to ignore these'
else:
    print

print 'This script does not check how good the biases are or whether the runs that it considers to be biases truly are,'
print 'so take care.'
