#!/bin/csh -f
#
# !!begin
# !!title    uls
# !!author   T.R. Marsh
# !!created  24 Sep 2002
# !!root     uls
# !!index    uls
# !!descr    equivalent of 'ls' for server files
# !!class    Information
# !!class    Scripts
# !!css      ultracam.css
# !!head1    uls is the equivalent of 'ls' for server files
#
# !!emph{uls} is a script that uses 'curl' to ask the server what files it thinks it has available.
#
# !!head2 Invocation
#
# uls (file)
#
# !!head2 Arguments
#
# !!table
#
# !!arg{file}{Can be blank in which case you get a listing of the top-level directory, or can specify a
# directory path.}
# 
# !!table
#
# NB for this routine to work the environment variable ULTRACAM_DEFAULT_URL must be set to point to the URL of
# the fileserver.
#
# !!end

if($#argv < 1) then
  curl --silent ${ULTRACAM_DEFAULT_URL}\?action=dir | sed -e 's/<[^<]*>//g' | grep run | sort
else
  curl --silent ${ULTRACAM_DEFAULT_URL}$argv[1]\?action=dir | sed -e 's/<[^<]*>//g' | grep run | sort
endif

exit;
