#!/bin/csh -f
#
# !!begin
# !!title    extract_times
# !!author   T.R. Marsh
# !!revised  26 Aug 2005
# !!root     extract_times
# !!index    extract_times
# !!descr    extracts first 100 times from ultracam files
# !!class    Scripts
# !!css      ultracam.css
# !!head1    extract_times extracts first 100 times from ultracam files
#
# !!emph{extract_times} extracst the first 100 times from ultracam files held in directory dir
# the results will be written in whichever directory this is run.
#
# !!head2 Invocation
#
# extract_times directory
#
# !!head2 Arguments
#
# !!table
#
# !!arg{directory}{directory containing .dat and .xml files}
# 
# !!table
# !!end


if($?ULTRACAM == 0) then
  echo "ULTRACAM environment variable is not set"
  echo "This must point to the location of the ultracam executables for addaframe to work"
  exit 1
endif

if($#argv != 1) then
  echo "usage: extract_times directory"
  exit
endif 

set dir = $1
foreach file (`ls $dir/run*.xml`)
  grep -q PON $file
  if( $status ) then
    $ULTRACAM/times $file:r first=1 last=100 \\ >&! $file:t:r.times
  endif
end



