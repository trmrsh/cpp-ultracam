#!/bin/csh -f
#
# !!begin
# !!title    sumframes
# !!author   T.R. Marsh
# !!created  26 Aug 2005
# !!root     sumframes
# !!index    sumframes
# !!descr    Adds a list of frames together
# !!class    Arithematic
# !!class    Scripts
# !!css      ultracam.css
# !!head1    sumframes adds a list of frames together
#
# !!emph{sumframes} adds a list of files together. See also the routine !!ref{combine.html}{combine}
# for an alternative.
#
# !!head2 Invocation
#
# sumframes sum file1 file2 ...
#
# !!head2 Arguments
#
# !!table
#
# !!arg{sum}{The output summed frame}
# !!arg{file1 file2 ...}{A variable number of other arguments, possibly as a glob}
# 
# !!table
# !!end

if($?ULTRACAM == 0) then
  echo "ULTRACAM environment variable is not set"
  echo "This must point to the location of the ultracam executables for sumframes to work"
  exit 1
endif

if($#argv < 2) then
  echo "usage: sumframes sum frame1 frame2 ..."
  exit
endif

set sum    = $argv[1]
shift argv
set frame1 = $argv[1]
shift argv

cp -f $frame1 ${sum}.ucm
echo "Added $frame1 to $sum"

foreach file ($*)
  if($file:e != "ucm") then
    echo "Skipped $file as it does not end with .ucm"
  else if(-e $file) then
    set root = $file:r
    $ULTRACAM/add $sum $root $sum
    echo "Added $root to $sum"
  endif
end

exit;
