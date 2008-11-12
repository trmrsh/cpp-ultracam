#!/bin/csh -f
#
# !!begin
# !!title    ucat
# !!author   T.R. Marsh
# !!created  24 Sep 2002
# !!root     ucat
# !!index    ucat
# !!descr    equivalent of 'cat' for server .xml files
# !!class    Information
# !!class    Scripts
# !!css      ultracam.css
# !!head1    ucat is the equivalent of 'cat' for server .xml files
#
# !!emph{ucat} is a script that uses 'curl' to print the contents of a particular xml file
# to standard output. It allows you to see exactly what the pipeline programs see.
#
# !!head2 Invocation
#
# ucat file
#
# !!head2 Arguments
#
# !!table
#
# !!arg{file}{Can include a directory path if wanted, but otherwise a name such as run0012}
# 
# !!table
#
# NB for this routine to work the environment variable ULTRACAM_DEFAULT_URL must be set to point to the URL of
# the fileserver.
#
# !!end

if($#argv < 1) then
  echo "usage: ucat file"
else
  curl ${ULTRACAM_DEFAULT_URL}$argv[1]\?action=get_xml
endif

exit;
