#!/usr/bin/env python

# !!begin
# !!title    unique
# !!author   T.R. Marsh
# !!created  23 June 2007
# !!revised  23 June 2007
# !!root     unique
# !!index    unique
# !!descr    Lists all the unique formats in a directory of ULTRACAM runs
# !!class    Scripts
# !!class    Observing
# !!css      style.css
# !!head1    unique lists all the unique formats in a directory of ULTRACAM runs
#
# !!emph{unique} identifies the unique formats in a directory and
# is useful for working out what biases to carry out on the following night.
# It only requires the run***.xml files to be available. It will list all the different
# formats once and try to identify that already have biases available. 'Try' because
# it only goes on the name being 'Bias', which is not 100% reliable.
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
# !!arg{directory}{Directory containing run*.xml files}
#
# !!table
#
# Related commands: !!ref{fmatch.html}{fmatch}, !!ref{missbias.html}{missbias}
#
# !!end

import sys
import os
import re
import ultracam

if len(sys.argv) != 2:
    print 'usage: directory'
    exit(1)

direct = sys.argv[1]

# get a list of runs
rtest = re.compile('^run[0-9][0-9][0-9]\.xml$')

runs = [rn for rn in os.listdir(direct) if rtest.match(rn) != None]

# Accumulate a list of unique runs, skipping power ons & offs
uniq = {}

for run in runs:
    fname = os.path.join(direct,run)
    tucam = ultracam.Ultracam(fname)

    if tucam.is_not_power_onoff():

        # compare with already stored formats
        new_format = True
        for rn,ucam in uniq.iteritems():
            if ultracam.match(ucam, tucam):
                new_format = False

                # get rid of biases in favour of more interesting runs and record any matching biases
                if ucam['target'] == 'Bias' and tucam['target'] != 'Bias':
                    tucam['bias'] = uniq[rn]['run']
                    del uniq[rn]
                    uniq[run[0:run.rfind('.xml')]] = tucam
                elif tucam['target'] == 'Bias' and ucam['target'] != 'Bias':
                    ucam['bias'] = run[0:run.rfind('.xml')]

                break
        
        if new_format:
            uniq[run[0:run.rfind('.xml')]] = tucam


# list them in run order. First the biases themselves.

title = '\n Run     Target                Filters     Bin  Gain   Size1   XL1 XR1  YS1     Size2   XL2 XR2  YS2     Size3   XL3 XR3  YS3\n'

print '\n--------------------------------------------------------------------------------------------------------------------------'
print '\nBiases:\n'

print title

for run in sorted(uniq.keys()):
    if uniq[run]['target'] == 'Bias':
        print ' ' + run + '  ' + uniq[run].to_string()


# now runs with biases in the specified directory
print '\n--------------------------------------------------------------------------------------------------------------------------'
print '\nRuns with probable biases (not necessarily good):\n'

print title

nbias = 0

for run in sorted(uniq.keys()):
    if 'bias' in uniq[run]:
        nbias += 1
        print ' ' + run + '  ' + uniq[run].to_string()

if nbias > 0:
    print ''
    
for run in sorted(uniq.keys()):
    if 'bias' in uniq[run]:
        print ' ' + run + '  has a corresponding bias = ' + uniq[run]['bias']
        
# now runs without biases in the specified directory which may need some to be taken

print '\n--------------------------------------------------------------------------------------------------------------------------'
print '\nRuns without biases:\n'

print title

for run in sorted(uniq.keys()):
    if 'bias' not in uniq[run] and uniq[run]['target'] != 'Bias':
        print ' ' + run + '  ' + uniq[run].to_string()

print ''

            
