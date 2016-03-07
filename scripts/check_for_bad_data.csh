#!/bin/csh -f

# !!begin
# !!title    check_for_bad_data
# !!author   T.R. Marsh
# !!revised  06 Aug 2008
# !!root     check_for_bad_data
# !!index    check_for_bad_data
# !!descr    Checks for problem with all ULTRACAM chips going high
# !!class    Scripts
# !!css      ultracam.css
# !!head1    Checks for problem with all ULTRACAM chips going high
#
# Script to check for the problem with ultracam when all chips go to an almost constant high level
# It does this by looking for a high median value of the blue CCD which is the most unlikely one 
# ever to high naturally. If it encounters this problem it alerts the user by playing
# an irritating noise. This should be run on the ULTRACAM data reduction
# PC. It accesses a sound file in Vik's account. You must run it
# in a directory where you have write access as it creates a dot file 
# sub-directory for storage of pipeline default files and writes the values
# of the medians to a file called zzz_median_values. It also checks for the low
# red bias problem (<1000) and reports all the median values to the terminal which
# is useful during twilight.
#
# !!head2 Invocation
#
# The script runs as an infinite loop so you should run it in its own terminal and hit ctrl-C
# when you want to shut it down.
#
# To get rid of irritating diagnostic output invoke as:
#
# (check_for_bad_data > /dev/tty ) > & /dev/null 
#
# Current poll interval is 15 seconds.
#
# !!end

# start ultracam with separate default directory to avoid clashes with
# interactive session. Force user to define generic ultracam
 
setenv ULTRACAM_ENV .crash_default

if($?ULTRACAM == 0) then
  echo "ULTRACAM environment variable is not set"
  echo "This must point to the location of the ultracam executables for addaframe to work"
  exit 1
endif

# how many seconds to wait between checks. This determines how swiftly a
# problem will be picked up.
set poll_interval = 15
while (1)

    echo ""
    echo ""
 
    # determine current run
    set run = `uls | tail -n 1`
    echo "Checking "$run" for problems"

    # need to change source to 's' once this is working
    set file = `$ULTRACAM/grab source=s url=$run first=0 trim=no tmax=0 bias=no \\ | grep 'Written' | sed -e 's/Written //' -e 's/,.*//'`
    echo "Grabbed "$file"; now computing medians"

    stats data=$file window=ALL sigma=3 | grep Median > zzz_median_values
    set red_median   = `cat zzz_median_values | head -1 | sed -e 's/.* = //' -e 's/\..*//'`
    set green_median = `cat zzz_median_values | head -2 | tail -1 - | sed -e 's/.* = //' -e 's/\..*//'`
    set blue_median  = `cat zzz_median_values | tail -1 | sed -e 's/.* = //' -e 's/\..*//'`

    echo "Red, green, blue median values = "$red_median,$green_median,$blue_median

    if(($red_median != "") && ($blue_median != "") && ($green_median != "")) then

        # check for 32k problem
	if ($blue_median > 20000) then
	    echo ""
	    echo "ERROR! ERROR! ERROR! 32k problem may be occurring! Blue CCD median > 20000"
	    echo ""
	    echo "If this is not the result of twilight, you may need to stop the current exposure,"
	    echo "shut down udriver, the file and camera save windows (the last two in the order" 
	    echo "specified) and then go and reset controller. If this does not work the first time,"
	    echo "try an initialize from udriver before resetting the controller again. Please try to"
	    echo "diagnose the problem as far as you can because its a bugger"
	    echo ""
	    echo "ctrl-C to stop the alarm and quit the script"
	    /usr/bin/play /home/star/ultracam_sounds/alarm_pk.wav >& /dev/null
	endif

	# check for low red bias problem
	if ($red_median < 1000) then
	    echo ""
	    echo "ERROR! ERROR! ERROR! low red bias problem may be occurring! Red CCD median < 1000"
	    echo "Stopping the exposure and restarting usually does the trick. Good luck!"
	    echo ""
	    echo "ctrl-C to stop the alarm and quit the script"
	    /usr/bin/play /home/star/ultracam_sounds/alarm_pk.wav >& /dev/null
	endif

    endif

    # remove the temporary file
    rm $file".ucm"

    echo ""
    echo "Sleeping "$poll_interval" seconds before next check; hit ctrl-C to exit ..."

    sleep $poll_interval

end
