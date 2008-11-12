#!/bin/csh -f
#
# !!begin
# !!title    mulaframe
# !!author   T.R. Marsh
# !!revised  26 Aug 2005
# !!root     mulaframe
# !!index    mulaframe
# !!descr    Multiplies a list of frames by one frame
# !!class    Arithematic
# !!class    Scripts
# !!css      style.css
# !!head1    mulaframe multiplies a list of frames by one frame
#
# !!emph{mulaframe} multiplies a list of frames by one frame, as one might want to
# do if applying a flat field. It is a script that invokes !!ref{mul.html}{mul}.
#
# !!head2 Invocation
#
# mulaframe frame file1 file2 ...
#
# !!head2 Arguments
#
# !!table
#
# !!arg{frame}{The frame to multiply by}
# !!arg{file1 file2 ...}{A variable number of other arguments, possibly as a glob}
# 
# !!table
# !!end

if($?ULTRACAM == 0) then
  echo "ULTRACAM environment variable is not set"
  echo "This must point to the location of the ultracam executables for mulaframe to work"
  exit 1
endif

if($#argv < 2) then
  echo "usage: mulaframe frame file1 file2 ..."
  exit
endif

set frame = $argv[1]
shift argv

foreach file ($*)
  if($file:e != "ucm") then
    echo "Skipped $file as it does not end with .ucm"
  else if(-e $file) then
    set root = $file:r
    $ULTRACAM/mul $root $frame $root
    echo "Multiplied $root by $frame"
  endif
end

exit;
