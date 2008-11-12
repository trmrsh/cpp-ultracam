#!/bin/csh -f
#
# !!begin
# !!title    addaframe
# !!author   T.R. Marsh
# !!revised  26 Aug 2005
# !!root     addaframe
# !!index    addaframe
# !!descr    Adds a frame to a list of frames
# !!class    Arithematic
# !!class    Scripts
# !!css      ultracam.css
# !!head1    addaframe adds one frame to a list of frames
#
# !!emph{addaframe} adds the same frame to a list of frames. It is a script
# which uses !!ref{add.html}{add}.
#
# !!head2 Invocation
#
# addaframe frame file1 file2 ...
#
# !!head2 Arguments
#
# !!table
#
# !!arg{frame}{The frame to add}
# !!arg{file1 file2 ...}{A variable number of other arguments, possibly as a glob}
# 
# !!table
# !!end

if($?ULTRACAM == 0) then
  echo "ULTRACAM environment variable is not set"
  echo "This must point to the location of the ultracam executables for addaframe to work"
  exit 1
endif

if($#argv < 2) then
  echo "usage: addaframe frame file1 file2 ..."
  exit
endif

set frame = $argv[1]
shift argv

foreach file ($*)
  if($file:e != "ucm") then
    echo "Skipped $file as it does not end with .ucm"
  else if(-e $file) then
    set root = $file:r
    $ULTRACAM/add $root $frame $root
    echo "Added $frame to $root"
  endif
end

exit;
