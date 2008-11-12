#!/usr/bin/env python

# !!begin
# !!title    missbias
# !!author   T.R. Marsh
# !!created  23 June 2007
# !!revised  23 June 2007
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

import sys
import os
import re
import ultracam

if len(sys.argv) < 2:
    print 'usage: dir1 dir2 ..'
    exit(1)

# remove first argument
sys.argv.pop(0)

# check arguments
for direct in sys.argv:
    if not os.path.isdir(direct):
        print 'Directory = ' + direct + ' does not exist, is not accessible or is not a directory'
        exit(1)
        
# how to identify right files

rtest = re.compile('^run[0-9][0-9][0-9]\.xml$')

ucams = {}

# Accumulate a list of unique formats skipping power ons and offs

for direct in sys.argv:

    runs = [rn for rn in os.listdir(direct) if rtest.match(rn) != None]

    for run in runs:
        fname = os.path.join(direct,run)
        tucam = ultracam.Ultracam(fname)

        if tucam.is_not_power_onoff():

            key = fname[0:fname.rfind('.xml')]

            # compare with already stored formats
            new_format = True
            for rn,ucam in ucams.iteritems():

                if ultracam.match(ucam, tucam):
                    new_format = False

                    # get rid of biases in favour of more interesting runs and record any matching biases
                    if ucam.is_bias() and tucam.is_not_bias():
                        tucam['bias'] = ucams[rn]['run']
                        del ucams[rn]
                        ucams[key] = tucam
                    elif ucam.is_not_bias() and tucam.is_bias():
                        ucam['bias'] = key
                    
                    break

            if new_format:
                ucams[key] = tucam

    # also runs on 'data' sub-directories
    datadir = os.path.join(direct, 'data')
    if os.path.isdir(datadir):

        runs = [rn for rn in os.listdir(datadir) if rtest.match(rn) != None]

        for run in runs:
            fname = os.path.join(datadir,run)
            tucam = ultracam.Ultracam(fname)

            if tucam.is_not_power_onoff():

                key = fname[0:fname.rfind('.xml')]

                # compare with already stored formats
                new_format = True
                for rn,ucam in ucams.iteritems():
                    
                    if ultracam.match(ucam, tucam):
                        new_format = False

                        # get rid of biases in favour of more interesting runs and record any matching biases
                        if ucam.is_bias() and tucam.is_not_bias():
                            tucam['bias'] = ucams[rn]['run']
                            del ucams[rn]
                            ucams[key] = tucam
                        elif ucam.is_not_bias() and tucam.is_bias():
                            ucam['bias'] = key
                    
                        break

                if new_format:
                    ucams[key] = tucam


# List any that apparently are without biases

print '\nRuns without apparent biases:\n'

print '\nRun                Target                Filters     Bin  Gain   Size1   XL1 XR1  YS1     Size2   XL2 XR2  YS2     Size3   XL3 XR3  YS3\n'

for run in sorted(ucams.keys()):
    if 'bias' not in ucams[run] and ucams[run].is_not_bias():
        print run.ljust(15) + '  ' + ucams[run].to_string()

print ''

            
