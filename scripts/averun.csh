#!/bin/csh -f
#
# !!begin
# !!title    averun
# !!author   T.R. Marsh
# !!created  29 Apr 2010
# !!root     averun
# !!index    averun
# !!descr    averun
# !!class    Scripts
# !!class    Observing
# !!css      style.css
# !!head1    averun produces an average of a specified number of frames of a run
#
# A common operation while observing is to grab and combine the first few runs of a series
# before setting up reduction apertues. !!emph{averun} combines these operations into
# a single sequence. NB it uses whatever settings you have in place for grab, e.g. such as
# bias subtraction. It combines with clipped mean settings.
#
# !!head2 Invocation
#
# averun run n1 n2
#
# !!head2 Arguments
#
# !!table
# !!arg{run}{The run number to average}
# !!arg{n1}{The first frame to average, typically = 2 if you are in noclear mode}
# !!arg{n2}{The last frame to average}
# !!table
#
# !!end

if($?ULTRACAM == 0) then
  echo "ULTRACAM environment variable is not set"
  echo "This must point to the location of the ultracam executables for addaframe to work"
  exit 1
endif

if( $# != 3 ) then
  echo "usage: run first last"
  exit 1
endif

set run  = $1:t

set n1   = $2
set n2   = $3

if( $n2 < $n1 ) then
  echo "Last frame must at least be equal to the first frame"
  exit 1
endif

$ULTRACAM/grab $1 first=$n1 last=$n2 \\

\ls ${run}_*.ucm >! ${run}.lis

$ULTRACAM/combine ${run}.lis method=c sigma=4 careful=yes adjust=i output=$run

echo " "
echo "Result stored in file = ${run}.ucm"

