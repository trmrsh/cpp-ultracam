#!/bin/csh -f
#
# !!begin
# !!title    debias
# !!author   T.R. Marsh
# !!revised  09 Jan 2006
# !!root     debias
# !!index    debias
# !!descr    Subtract a constant determined from a specified region from a list of frames
# !!class    Arithematic
# !!class    Scripts
# !!css      style.css
# !!head1    debias subtracts a constant determined from a specified region from a list of frames
#
# !!emph{debias} substracts a constant determined from a region of the CCDs 
# which is what one sometimes wants to do to frames if they have an overscan region
# for instance. It works by repeatedly running the program !!ref{stats.html}{stats}
# on the files which sets a variable in the the global defaults file which is then passed
# to !!ref{csub.html}{csub}.
#
# !!head2 Invocation
#
# debias region sigma file1 file2 ...
#
# !!head2 Arguments
#
# !!table
# !!arg{region}{The region to determine the constant over, a file as set e.g. by !!ref{setwin.html}{setwin}}
# !!arg{sigma}{The number of sigma to use region to determine the constant over, a file as set e.g. by !!ref{setwin.html}{setwin}}
# !!arg{file1 file2 ...}{The files to debias}
# !!table
#
# !!end

if($?ULTRACAM == 0) then
  echo "ULTRACAM environment variable is not set"
  echo "This must point to the location of the ultracam executables for addaframe to work"
  exit 
endif 

if($#argv < 3) then
  echo "usage: debias region sigma file1 file2 ..."
  exit
endif

# read and remove non-file arguments

set region = $argv[1]
set sigma  = $argv[2]
shift argv; shift argv

foreach file ($*)
  if($file:e != "ucm") then
      echo "Skipped $file as it does not end with .ucm"
  else
      $ULTRACAM/stats $file:r $region $sigma
      $ULTRACAM/csub  $file:r @stats_clipped_mean $file:r
      echo "Debiassed file = $file"
  endif
end

exit
