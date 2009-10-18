#!/bin/csh -f
#
# !!begin
# !!title    makebias
# !!author   T.R. Marsh
# !!created  17 Mar 2006
# !!revised  16 Nov 2007
# !!root     makebias
# !!index    makebias
# !!descr    Makes a bias frame out of a run
# !!class    Scripts
# !!class    Observing
# !!calss    Calibration
# !!css      style.css
# !!head1    makebias makes a bias frame out of a run
#
# !!emph{makebias} is a simple shortcut for the common operation of grabbing a series of frames,
# making a list of them and then combining the result to make a bias. It does this with no trimming,
# and uses all of the frames in the bias except the first and the last. If you want to do something different, do it by hand.
# As well as the output file, the list of files used is left in a file of the form 'run003.lis';
# the intermediate ucm frames are deleted. It eliminates the first frame because often its exposure time is
# shorter than all subsequent frames and thus is a bad frame for setting the exposure time as needed for proper 
# dark subtraction. It eliminates the last because the user might have terminated it early and therefore it
# does not represent a good bias either.
#
# !!head2 Invocation
#
# makebias run first output 
#
# !!head2 Arguments
#
# !!table
#
# !!arg{run}{The name of the run}
# !!arg{first}{first frame to use. Normally 1, but > 1 for drift mode runs}
# !!arg{output}{the name of the output file}
# 
# !!table
# !!end

if($?ULTRACAM == 0) then
  echo "ULTRACAM environment variable is not set"
  echo "This must point to the location of the ultracam executables for mulaframe to work"
  exit 1
endif

if($#argv != 3) then
  echo "usage: makebias run first output"
  exit
endif

set run    = $argv[1]
set first  = $argv[2]
set output = $argv[3]
set root   = $run:t

# delete files
rm ${root}_[0-90-90-90-90-9].ucm

# grab the frames
$ULTRACAM/grab $run ndigit=5 first=$first last=0 trim=no bias=no



echo "root = $root"

# delete the last files 
rm `ls ${root}_[0-90-90-90-90-9]*ucm | tail -n 1`

# make a file list
ls ${root}_[0-90-90-90-90-9]*ucm >! ${root}.lis 

# combine them all
$ULTRACAM/combine ${root}.lis method=c sigma=2.8 careful=yes adjust=b output=$output

# delete intermediate ucm files
foreach file (`cat ${root}.lis`)
  rm $file
end

exit;
