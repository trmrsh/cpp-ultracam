#!/usr/bin/env python

# !!begin
# !!title    unique
# !!author   T.R. Marsh
# !!created  23 June 2007
# !!revised  11 June 2009
# !!root     unique
# !!index    unique
# !!descr    Lists all the unique formats in a directory of ULTRACAM or ULTRASPEC runs
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
import Ultra
from optparse import OptionParser

# define and parse options
usage = """
usage: %prog [options] -- tests for unique ULTRACAM / ULTRASPEC formats.

Arguments:

dir1 dir2 -- series of directories to look through.

"""

parser = OptionParser(usage)
parser.add_option("-f", "--fussy", dest="fussy", default=False, action="store_true",\
                  help="fussy test so that avalanche gain differences will be picked up")

(options, args) = parser.parse_args()

dirs = [d for d in args if os.path.isdir(d)]

Ultra.Run.Fussy = options.fussy

# get a list of runs
run_re = re.compile('^run[0-9][0-9][0-9]\.xml$')

# Accumulate a list of unique runs, skipping power ons & offs
uniq = {}

for dir in dirs:
    xmls = [rn for rn in os.listdir(dir) if run_re.match(rn) != None]

    for xml in xmls:
        run = Ultra.Run(os.path.join(dir,xml))

        if run.is_not_power_onoff():

            # compare with already stored formats
            new_format = True
            replaced = []
            for rn, rold in uniq.iteritems():
                if run == rold:
                    new_format = False

                    # get rid of biases in favour of more interesting runs and record any matching biases
                    if rold.target == 'Bias' and run.target != 'Bias':
                        replaced.append(rn)
                        new_format = True
            
            for rn in replaced:
                del uniq[rn]
        
            if new_format:
                uniq[os.path.join(dir,xml[0:xml.rfind('.xml')])] = run

print '\nFound',len(uniq),'unique formats in directories = ',args

print '\nThe formats are:\n'

for run in sorted(uniq.keys()):
#    if uniq[run]['target'] == 'Bias':
    print '%-20s %s' % (run,uniq[run])


