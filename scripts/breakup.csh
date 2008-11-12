#!/bin/csh -f
#
# !!begin
# !!title    breakup
# !!author   T.R. Marsh
# !!revised  10 Dec 2003
# !!root     breakup
# !!index    breakup
# !!descr    breaks up large files into smaller pieces
# !!class    Scripts
# !!css      ultracam.css
# !!head1    breakup splits large files
#
# !!emph{breakup} splits up the large ultracam run***.dat files. This can be useful for backup and for generally managing
# the large files produced by ultracam. It also makes copies of the equivalent .xml files to go along with the .dats
# The routine ensures that the files are broken into integral numbers of exposures.
#
# !!head2 Invocation
#
# breakup file
#
# !!head2 Arguments
#
# !!table
# !!arg{file}{The file to break (without the .dat extension)}
# !!arg{maxsize}{The maximum size in bytes of the output files}
# !!table
# !!end

if($#argv != 2) then
  echo "usage: breakup file maxsize"
  exit
endif

# set maximum filesize to 600 Mbytes

set file     = $argv[1]
set MAXBYTES = $argv[2]

if(!(-e "$file.dat") || !(-e "$file.xml")) then
  echo "Could not find either or both of $file.dat or $file.xml!"
  exit
endif

# determine framesize in bytes from the XML file
set framesize = `grep 'framesize=\"' $file.xml | sed -e 's/^.*framesize=\"//' -e 's/".*//'`

if($framesize == "" || $framesize < 24) then
  echo "Could not determine framesize or framesize implausibly small = $framesize"
  exit
endif

@ nexp = $MAXBYTES / $framesize

if($nexp == 0) then
  @ nexp = 1
endif

@ nbytes = $framesize * $nexp

echo "Splitting files to have a maximum of $nexp exposures and $nbytes bytes each"

split --bytes=$nbytes $file.dat $file 
 
foreach fnew ("$file"[a-z]*)
  mv -f $fnew "$fnew.dat"
  set suffix = `echo $fnew | sed -e "s/$file//" -`
  cp -f "$file.xml" "$file$suffix.xml" 
  echo "Created $fnew.dat and $file$suffix.xml"
end

exit



