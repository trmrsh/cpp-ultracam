#!/usr/bin/env python
#
# Script to check for GRB alert messages received on an imap server
#
# Written by Tom Marsh, April 2010

# these should all be standard Python modules
import imaplib
import traceback, sys
import time, datetime, calendar
import subprocess
import re
import math as m

# You may have to edit the next appropriately depending upon which
# imap account you want to go to. Don't use the password directly for
# sensitive accounts of course.

imap_user   = 'ultracamToO@gmail.com'
imap_server = 'imap.gmail.com'
imap_pword  = 'playa_nogales'
imap_port   = 993

# Define the sound command, any options and file to play in case of alerts
# You may have to edit this depending on your system.
sound_command = ['/usr/bin/play', '-q', '/home/star/ultracam_sounds/alarm_pk.wav']

# Number of seconds between IMAP polls. Careful not to make too short as it
# might stress out the server.
NSLEEP = 20

# Maximum time to bother reporting anything, minutes since burst
TMAX = 120.

# Longitude and latitude of observatory, radians. East = positive
OBS_NAM = 'NTT'
OBS_LNG = m.radians(-70.7317422)
OBS_LAT = m.radians(-29.2551222)

def mjd2gmst(mjd):
    """
    Returns GMST given UT1 in the form of an MJD

    The GMST is returned in radians from 0 to 2pi. This
    was converted from Fortran source code of SOFA
    """

    # Set some constants
    DS2R   = 7.272205216643039903848712e-5
    DJ0    = 2451545e0
    DJ1    = 2400000.5e0
    DAYSEC = 86400e0
    CENDAY = 36525e0
    A      = 24110.54841e0 - DAYSEC/2.
    B      = 8640184.812866e0
    C      = 0.093104e0
    D      = -6.2e-6

    if DJ1 < mjd:
        d1 = DJ1
        d2 = mjd
    else:
        d1 = mjd
        d2 = DJ1

    t = (mjd + (DJ1-DJ0 ))/CENDAY

    f = DAYSEC*(0.5 + m.fmod(mjd,1.))

    return m.fmod(DS2R*((A+(B+(C+D*t)*t)*t)+f),2.*m.pi)

def tohms(num, nosign=True):
    """
    Convert to an HH MM SS.SS format
    """
    if num < 0:
        sign = '-'
        num *= -1
    else:
        sign = '+'

    h = int(num)
    m = int(60.*(num - h))
    s = 60.*(60.*(num-h)-m)
    st = '%02.2d:%02.2d:%05.2f' % (h,m,s)
    if nosign:
        return st
    else:
        return sign + st

print '\n\n    ******* DEAR OBSERVER, PLEASE READ THIS *******'
print '\nThis script will connect to the gmail imap server every',NSLEEP,'seconds'
print 'where it will look for email in the UltracamToO account. If it finds'
print 'a message it will play an alarm and suggest coordinates to place the'
print 'GRB in a good area of the CCDs as well as what actions you should'
print 'take. Bear in mind that speed is of the essence if a GRB is'
print 'triggered.'
print '\nIf you want to stop the alarm from re-triggering when you re-start the'
print 'script, you will need to delete the messages from the gmail account,'
print 'but they will in any case be ignored if the burst occurred more than'
print TMAX,'minutes ago.The script is currently set for the',OBS_NAM + '.'
print

# Regular expression matches
ToOok   = re.compile('ToO:\s*\[OK')
ToOnok  = re.compile('ToO:\s*\[not OK')
RA      = re.compile('RA\s*\(J2000\)\s*=\s*(\d\d):(\d\d):(\d\d(?:\.\d*))')
DEC     = re.compile('DEC\s*\(J2000\)\s*=\s*([-+])(\d\d):(\d\d):(\d\d(?:\.\d*))')
GRBline = re.compile('GRB\s+([\d\.]+)\s+GRB-time:\s+([\d:\.]+)\s+UT')
Error   = re.compile('Error radius:\s*(.*)\s*\r\n')
AV      = re.compile('A\_V\s*=\s*(\d+\.\d*)')
MOON    = re.compile('D\(Moon\)\s*=\s*([\.\d]+)')

KNOWN   = re.compile('\*+ Known Source \*+')
PNTGRB  = re.compile('\*+ Probably NOT a GRB \*+')
DNTGRB  = re.compile('\*+ Definitely NOT a GRB \*+')
COSRAY  = re.compile('\*+ Cosmic Ray \*+')

# Start loop to search for alerts and triggers

# block to trap exceptions. KeyboardInterrupt exceptions (ctrl-C) are explicitly dealt
# with; any others will lead to the script halting and it will play a sound as
# a warning to re-start the script in this case.

try:
    imap_login = False
    imap_open  = False
    ncheck = 1
    while 1:

        # sleep for a while so we don't lock up the server.
        time.sleep(NSLEEP)

        # Connect to the imap server
        imap = imaplib.IMAP4_SSL(imap_server, imap_port)

        # Login
        imap.login(imap_user, imap_pword)
        imap_login = True

        # Select the inbox for searching
        imap.select('INBOX')
        imap_open = True

        # Get current time, convert to various related quantities.
        gmt    = time.gmtime()
        gmtnow = calendar.timegm(gmt)
        ctime  = time.asctime(gmt)
        print '\n\n***** Check no. ' + str(ncheck) + ', time = ' + ctime + ' UT *****'
        ncheck += 1
        mjd    = 40587. + gmtnow / 86400.
        gmst   = mjd2gmst(mjd)
        lst    = gmst + OBS_LNG

        sound = False

        # First search for alerts
        r, data = imap.search(None, '(FROM "grbtoo@dark.dark-cosmology.dk (GRB Alert)")')

        if data[0] == '':
            print '\nNo GRB alerts found.'
        else:

            # Read body of messages (but leave as new with PEEK so it up to user to actually read them in full
            # if he/she wants to stop these messages)

            code_red = False
            nburst = 0
            for num in data[0].split():
                ret, mess = imap.fetch(num, '(BODY.PEEK[TEXT])')
                if ret == 'OK':
                    mstr = mess[0][1]

                    # Look for the type of the alert. BAT are first, but with only
                    # rough positions. XRT come later with good coords.
                    if mstr.find('Swift (SWIFT_BAT)') > -1:
                        alert = 'BAT'
                    elif mstr.find('Swift (SWIFT_XRT)') > -1:
                        alert = 'XRT'
                    elif mstr.find('Swift (SWIFT_UVOT)') > -1:
                        alert = 'UVOT'
                    else:
                        alert = 'UNKNOWN'
                    message = '\n>>>>>>\n' + alert + ' alert found.'

                    # Look for line defining name and time of GRB. Extract
                    # the time so we can report how long ago it occurred.
                    mch = GRBline.search(mstr)
                    if mch:
                        date   = mch.group(1)
                        utc    = mch.group(2)
                        year   = int('20' + date[:2])
                        month  = int(date[2:4])
                        day    = int(date[4:6])
                        hour   = int(utc[:2])
                        minute = int(utc[3:5])
                        second = int(utc[6:8])
                        dt = datetime.datetime(year, month, day, hour, minute, second)
                        gmtburst = time.mktime(dt.timetuple())
                        delay = (gmtnow-gmtburst)/60.
                        delay = round(10.*delay)/10.
                        message += ' GRB ' + date + ', detected at ' + utc + ' UT, ' + str(delay) + ' mins ago.'
                        found_time = True
                    else:
                        message += ' ** could not parse for GRB ID & time; please read your e-mail asap **'
                        delay = 0.
                        found_time = False

                    # Only report bursts that have occurred within TMAX of current time.
                    if delay < TMAX:
                        nburst += 1

                        mch = MOON.search(mstr)
                        if mch:
                            m_message = ', D(Moon) = ' + mch.group(1) + ' deg'
                            found_moon = True
                            moon_dist = float(mch.group(1))
                        else:
                            m_message = ', D(Moon) = ??? deg'
                            
                        mch = AV.search(mstr)
                        if mch:
                            av = float(mch.group(1))
                            if av > 1:
                                message += m_message + '\nGalactic extinction ' + mch.group() + ' which is too large to be worth observing.'    
                            else:
                                message += ' ' + mch.group() + m_message
                        else:
                            message += m_message + '\nCould not interpret galactic extinction A_V, so will assume OK to observe; read your e-mail to check.'
                            av = 0.
                        print message

                        not_a_grb = False
                        if KNOWN.search(mstr):
                            message = ' Known source.'
                            not_a_grb = True
                        if DNTGRB.search(mstr):
                            message += ' Definitely not a GRB.'
                            is_a_grb = True
                        elif PNTGRB.search(mstr):
                            message += ' Probably not a GRB.'
                            not_a_grb = True
                        if COSRAY.search(mstr):
                            message += ' Probably a cosmic ray'
                            not_a_grb = True
                        if not_a_grb:
                            print 'However, one or more flags suggest this is not a GRB:' + message

                        # Dig out coordinates and error radius
                        mch = RA.search(mstr)
                        if mch:
                            rar   = m.radians(15.*(float(mch.group(1))+float(mch.group(2))/60+float(mch.group(3))/3600.))
                            rastr = mch.group(1) + ':' + mch.group(2) + ':' + mch.group(3)
                            found_ra = True
                        else:
                            rastr = '??:??:??.??'
                            found_ra = False

                        mch = DEC.search(mstr)
                        if mch:
                            decr   = m.radians(float(mch.group(2))+float(mch.group(3))/60+float(mch.group(4))/3600.)
                            if mch.group(1) == '-': decr *= -1
                            decstr = mch.group(1) + mch.group(2) + ':' + mch.group(3) + ':' + mch.group(4)
                            found_dec = True
                        else:
                            decstr = '???:??:??.?'
                            found_dec = True

                        message = alert + ' coordinates = ' + rastr + ' ' + decstr 

                        mch = Error.search(mstr)
                        if mch:
                            message += ', error radius = ' + mch.group(1)
                        else:
                            message += ', error radius = UNKNOWN'

                        print message

                        if found_ra and found_dec:
                            # Offset telescope to west so that target moves East which at PA=0 means towards
                            # right-hand side of chips in region with no major defects.
                            rar -= m.radians(82./3600.)
                            print '\nSet the PA=0, point telescope at RA, Dec =',tohms(m.degrees(rar)/15.),tohms(m.degrees(decr),False)
                            print 'Look for the GRB at pixel (x,y) ~ 580, 516'

                        # Work out whether we might have to go for RRM from coordinates
                        if found_ra and found_dec:
                            har    = m.fmod(lst - rar, 2.*m.pi)
                            if har < -m.pi: har += 2.*m.pi
                            if har >  m.pi: har -= 2.*m.pi
                            sinalt = m.sin(OBS_LAT)*m.sin(decr) + m.cos(OBS_LAT)*m.cos(decr)*m.cos(har)
                            alt    = round(10.*m.degrees(m.asin(sinalt)))/10.
                            message = '\nAltitude = ' + str(alt)
                            if har > 0.:
                                message += ', setting.'
                            else:
                                message += ', rising.'

                            halt  = round(10.*(90.-m.degrees(abs(decr-OBS_LAT))))/10.
                            coshc = (m.sin(m.radians(23.))-m.sin(OBS_LAT)*m.sin(decr))/(m.cos(OBS_LAT)*m.cos(decr))
                            if alt > 23.:
                                message += ' Highest altitude = ' + str(halt) + '. This is observable (> 23d) from the ' + OBS_NAM + ' right NOW!'
                                if coshc >= 1.: 
                                    message += '\nAltitude always > 23 deg.'
                                    code_red = True
                                else:
                                    hc = m.acos(coshc)
                                    wstart = round(100.*m.degrees(-hc-har)/15.)/100.
                                    wend   = round(100.*m.degrees( hc-har)/15.)/100.
                                    message += '\nAltitude > 23 deg for ' + str(wstart) + ' to ' + str(wend) + ' hours relative to present time.' 
                                    if wend > 0.5 and not not_a_grb:
                                        code_red = True                                            
                                if found_moon and moon_dist < 15:
                                    message += '\nNB target is < 15 degrees from the Moon which may preclude observations.'
                                elif not found_moon:
                                    message += '\nNB could not determin Moon distance: please read e-mail'

                            else:
                                if halt < 30.:
                                    message += ' Highest altitude = ' + str(halt) + ' is too low (< 30) to try observing.'
                                else:
                                    hc = m.acos(coshc)
                                    if har > 0.: har -= 2.*m.pi
                                    wstart = round(100.*m.degrees(-hc-har)/15.)/100.
                                    wend   = round(100.*m.degrees( hc-har)/15.)/100.
                                    message += ' This is not yet observable (> 23) but will be from ' + str(wstart) + ' to ' + str(wend) + ' hours relative to present time.' 
                                    if wstart < 0.5 and not not_a_grb:
                                        code_red = True                            
                                    if found_moon and moon_dist < 15:
                                        message += '\n NB target is < 15 degrees from the Moon which may preclude observations.'
                                    elif not found_moon:
                                        message += '\n NB could not determin Moon distance: please read e-mail'
                        else:
                            message = 'Failed to interpret coordinates; please read e-mail asap.'
                        print message

                        # OK, RRM imminent, so let's try to get some attention ...
                        if code_red and av <= 1.:
                            if alert == 'BAT':
                                print '\nRED ALERT!! Swift/BAT has found an immediately observable GRB!!!'
                                print 'Be on standby for XRT (accurate) coordinates and/or a trigger message from the GRB override team.'
                                print 'Get the BAT (approx) coordinates to the TO, but do not stop what you are doing yet.'
                            elif alert == 'XRT':
                                print '\nRED ALERT!! Swift/XRT has refined the coords for an immediately observable GRB!!!'
                                print 'If the TO has the BAT coords, and you are happy to break observing, you can move'
                                print 'immediately (as long as it is safe to do so). If you only want to move when sure'
                                print 'that there is a trigger from the GRB team, get the XRT coords to the TO, and standby'
                                print 'for a trigger. There is a small chance of error (< 5%), so if you are really unhappy'
                                print 'to move now, you may want to phone GRB expert on-call first.'

                else:
                    print 'Failed to read message number',num
 
            # If any alert reported, play a sound
            if nburst:
                pid = subprocess.Popen(sound_command).pid
                sound = True
            else:
                print '\nNo GRB alerts found.'

        # search for trigger messages, but am not quite sure of format, so don't do 
        # much other than report them.
        r, data = imap.search(None, '(SUBJECT "085.D-0662 ToO request")')
        if data[0] == '':
            print '\nNo GRB triggers found.'
        elif len(data[0].split()) == 1:
            print '\n>>>>>>\nRED ALERT!! a GRB trigger was found ... read your e-mail asap! '
            print 'If you have already entered coords from immediately preceding alerts, move NOW!!'
            print '(but make sure it is safe to do so).'
            if not sound: pid = subprocess.Popen(sound_command).pid
        else:
            print '\nRED ALERT!!',len(data[0].split()),'GRB triggers were found ... read your e-mail asap!'
            print 'If you have already entered coords from immediately preceding alerts, move NOW!!'
            print '(but make sure it is safe to do so).'
            if not sound: pid = subprocess.Popen(sound_command).pid

        # close down the connection to the server and logout
        imap.close()
        imap_open  = False
        imap.logout()
        imap_login = False


except KeyboardInterrupt:

    if imap_open:
        print '\nCtrl-C detected; will close down mailbox and logoff IMAP connection'
    elif imap_login:
        print '\nCtrl-C detected; will logoff IMAP connection'
    else:
        print '\nCtrl-C detected'

    # OK, close down mailbox and logout of IMAP
    if imap_open: imap.close()
    if imap_login: imap.logout()

    print 'Finished monitoring for GRB alerts and triggers.'
    print 'Please report any problems with this script to t.r.marsh@warwick.ac.uk'
    print 'Bye.'

except:

    print 'An unexpected exception was encountered'
    print '-'*60
    traceback.print_exc(file=sys.stdout)

    pid = subprocess.Popen(sound_command).pid
    print '\n\n------- You should probably re-start the script ----------'

    # OK, close down mailbox and logout of IMAP
    if imap_open: imap.close()
    if imap_login: imap.logout()
