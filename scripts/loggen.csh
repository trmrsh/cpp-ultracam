#!/bin/csh -f
#
# !!begin
# !!title loggen
# !!author   T.R. Marsh
# !!created  24 Sep 2002
# !!revised  26 Aug 2005
# !!root     loggen
# !!index    loggen
# !!descr    generates a log of a set of runs using !!ref{oneline.html}{oneline}
# !!class    Information
# !!class    Scripts
# !!css      ultracam.css
# !!head1    loggen generates a log with one line per run.
#
# !!emph{loggen} is a script which uses the program !!ref{oneline.html}{oneline} to generate a basic log of a set of runs.
#
# !!head2 Invocation
#
# loggen file1 file2 file3 etc
#
# !!head2 Arguments
#
# !!table
#
# !!arg{file1 etc}{You can supply any number of arguments, or use a glob as in 'r*.dat'. The '.dat' parts are trimmed
# off before loggen passes the arguments to oneline}
# 
# !!table
#
# Typically you will want to save the output to a file. Use '>&' to redirect both normal output and errors which
# can be useful as well.
# !!end

if($?ULTRACAM == 0) then
  echo "ULTRACAM environment variable is not set"
  echo "This must point to the location of the ultracam executables for loggen to work"
  exit 1
endif

if($#argv < 2) then
  echo "usage: loggen file1 file2 ..."
  exit
endif

foreach file ($*)
  set root = $file:r
  $ULTRACAM/oneline $root tmax=0 \\
end

exit;
