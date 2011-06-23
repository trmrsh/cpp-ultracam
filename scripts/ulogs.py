#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""
script to generate a form for accessing the ULTRACAM run database
Requires pyfits, numpy, simbad & subs to be available.
"""

import re, pyfits, numpy as np

def set(inp, default):
  """translates arguments into floats safely"""
  try:
    return float(inp)
  except:
    return default

def str2radec(position):
    """
    (ra,dec,system) = str2radec(position) -- translates an astronomical coordinate string to double precision RA and Dec.

    'ra' is the RA in decimal hours; 'dec' is the declination in degrees; 'system' is one of
    'ICRS', 'B1950', 'J2000'. Entering coordinates is an error-prone and often irritating chore.
    This routine is designed to make it as safe as possible while supporting a couple of common formats.

    Here are example input formats, both good and bad:

    12 34 56.1 -23 12 12.1     -- OK. 'system' will default to ICRS
    234.5 34.2  B1950          -- OK. RA, Dec entered in decimal degrees, B1950 to override default ICRS
    11 02 12.1 -23.2 J2000     -- NOK. Cannot mix HMS/DMS with decimals
    11 02 12.1 -23 01 12 J4000 -- NOK. Only 'ICRS', 'B1950', 'J2000' allowed at end.
    1.02323e2 -32.5            -- NOK. Scientific notation not supported.
    11 02 12.1 23 01 12        -- NOK. In HMS mode, the sign of the declination must be supplied
    11 02 12.1 -00 00 02       -- OK. - sign will be picked up
    25 01 61.2 +90 61 78       -- NOK. various items out of range.
    12:32:02.4 -12:11 10.2     -- OK. Colon separators allowed.

    A SubsError is raised on failure
    """
    
    # Try out three types of match
    m = re.search(r'^\s*(\d{1,2})(?:\:|\s+)(\d{1,2})(?:\:|\s+)(\d{1,2}(?:\.\d*)?)\s+([\+-])(\d{1,2})(?:\:|\s+)(\d{1,2})(?:\:|\s+)(\d{1,2}(?:\.\d*)?)(?:\s+(\w+))?\s*$', position)
    if m:
        (rah,ram,ras,decsign,decd,decm,decs,system) = m.groups()
        rah  = int(rah)
        ram  = int(ram)
        ras  = float(ras)
        decd = int(decd)
        decm = int(decm)
        decs = float(decs)
        if rah > 23 or ram > 59 or ras >= 60. or (decd > 89 and decm > 0 and decs > 0.) or decm > 59 or decs > 60.:
            raise Exception('one or more of the entries in the astronomical coordinates "' + position + '" is out of range')

        if not system: system = 'ICRS'
        if system != 'ICRS' and system != 'J2000' and system != 'B1950':
            raise Exception('astronomical coordinate system must be one of ICRS, B1950, J2000; ' + system + ' is not recognised.')
        
        ra  = rah + ram/60. + ras/3600.
        dec = decd + decm/60. + decs/3600.
        if decsign == '-': dec = -dec
        return (ra,dec,system)

    # No arcseconds of dec as sometimes is the case with coords from simbad
    m = re.search(r'^\s*(\d{1,2})(?:\:|\s+)(\d{1,2})(?:\:|\s+)(\d{1,2}(?:\.\d*)?)\s+([\+-])(\d{1,2})(?:\:|\s+)(\d{1,2}\.\d*)(?:\s+(\w+))?\s*$', position)
    if m:
        (rah,ram,ras,decsign,decd,decm,system) = m.groups()
        rah  = int(rah)
        ram  = int(ram)
        ras  = float(ras)
        decd = int(decd)
        decm = float(decm)
        if rah > 23 or ram > 59 or ras >= 60. or (decd > 89 and decm > 0 and decs > 0.) or decm >= 60.:
            raise Exception('one or more of the entries in the astronomical coordinates "' + position + '" is out of range')

        if not system: system = 'ICRS'
        if system != 'ICRS' and system != 'J2000' and system != 'B1950':
            raise Exception('astronomical coordinate system must be one of ICRS, B1950, J2000; ' + system + ' is not recognised.')
        
        ra  = rah + ram/60. + ras/3600.
        dec = decd + decm/60.
        if decsign == '-': dec = -dec
        return (ra,dec,system)

    m = re.search(r'^\s*(\d{1,3}(?:\.\d*)?)\s+([+-]?\d{1,2}(?:\.\d*)?)\s+(\w+)?\s*$', position)
    if m:
        print 'matched decimal entries'
        (rad,dec,system) = m.groups()
        ra   = float(rad)/15.
        dec  = float(dec)
        if ra >= 24. or dec < -90. or dec > 90.:
            raise Exception('one or more of the entries in the astronomical coordinates "' + position + '" is out of range')
        
        if not system: system = 'ICRS'
        if system != 'ICRS' and system != 'J2000' and system != 'B1950':
            raise Exception('astronomical coordinate system must be one of ICRS, B1950, J2000; ' + system + ' is not recognised.')

        return (ra,dec,system)
        
    raise Exception('could not interpret "' + position + '" as astronomical coordinates')

def main(argv):

  # deal with arguments. Could either be indicating a simbad or manual entry with different numbers of arguments

  simbad_failed = False
  if len(argv) == 0:
    # set defaults
    ra1, ra2, dec1, dec2, emin, unique, slimits, target, delta = 13., 14., -20., 20., 10., False, 'manual', '', 2.
  else:
    slimits = argv.pop(0)

    if slimits == 'simbad' and len(argv) >= 3:
      # simbad: arguments expected target, delta, emin, [unique]
      target, delta, emin = argv[:3]
      if len(argv) > 3:
        unique = True
      else:
        unique = False
      
      # attempt simbad lookup
      try:

        from trm import simbad

        query = simbad.Query(target)
        sl = query.query(10)

        if len(sl) == 1:
          ra,dec,system = str2radec(sl[0]['Position'])
          cosd = np.cos(np.radians(dec))
          delta = set(delta, 2.)
          ra1  = ra - delta/60./15./cosd
          ra2  = ra + delta/60./15./cosd
          dec1 = dec - delta/60.
          dec2 = dec + delta/60.

        else:
          simbad_failed = True
          ra1, ra2, dec1, dec2, emin, unique, slimits, target, delta = 13., 14., -20., 20., 10., False, 'manual', '', 2.
      except:
        simbad_failed = True
        ra1, ra2, dec1, dec2, emin, unique, slimits, target, delta = 13., 14., -20., 20., 10., False, 'manual', '', 2.

    elif slimits == 'manual' and len(argv) >= 7:

      # manual: arguments expected ra1, ra2, dec1, dec2, emin, [unique]
      ra1, ra2, dec1, dec2, emin, target, delta = argv[:7]
      if len(argv) > 7:
        unique = True
      else:
        unique = False

    else:
      # set defaults
      ra1, ra2, dec1, dec2, emin, unique, slimits, target, delta = 13., 14., -20., 20., 10., False, 'manual', '', 2.

    ra1   = set(ra1, 13.)
    ra2   = set(ra2, 14.)
    dec1  = set(dec1, -20.)
    dec2  = set(dec2, 30.)
    emin  = set(emin, 10.)
    delta = set(delta, 2.)

  #print first row of table:
  print '<h2>Selection criteria:</h2>\n<p>'
  print '<form method="get" action="ulogs.php">'
  print 'Set position limits using <input type="radio" name="slimits" value="simbad">SIMBAD lookup or <input type="radio" name="slimits" value="manual" checked>manually<br><br><br>'
  print 'SIMBAD lookup:<br><br>'
  print 'Object name: <input id="target"  name="target" type="text" value="%s" size="20" maxlength="20">' % target
  print 'Search range (arc minutes): <input id="delta"  name="delta" type="text" value="%6.2f" size="6" maxlength="6"><br><br><br>' % delta
  print 'Manual entry:<br><br>'
  print '<table>'
  print '<tr><td>Lower RA limit (hours):</td><td><input id="RA1"  name="RA1" type="text" value="%6.3f" size="6" maxlength="6"></td>' % ra1
  print '<td>Upper RA limit (hours): </td><td> <input id="RA1"  name="RA2" type="text" value="%6.3f" size="6" maxlength="6"><td></tr>' % ra2
  print '<tr><td>Lower Declination limit (deg): </td><td> <input id="Dec1"  name="Dec1" type="text" value="%+6.2f" size="6" maxlength="6"></td>' % dec1
  print '<td>Upper Declination limit (deg): </td><td> <input id="Dec2"  name="Dec2" type="text" value="%+6.2f" size="6" maxlength="6"></td></tr>' % dec2
  print '<tr><td>Min. exposure time (mins): </td><td> <input id="emin"  name="emin" type="text" value="%5.1f" size="6" maxlength="6">' % emin
  print '</td><td></td><td></td></tr>'
  print '<tr><td colspan="4"> Print just one run per night per object: <input type="checkbox" id="unique" name="unique" value="unique"',
  if unique: print ' checked',
  print '></td></tr>'
  print '</table>\n'
  print '<p><input type="submit" value="Search">'
  print '</form><br>\n<br>\n'

  print '<p>\n<hr>\n\n<h2>Matches:</h2>\n<p>'

  # read database file
  hdu     = pyfits.open('http://deneb.astro.warwick.ac.uk/phsaap/ultracam/logs/ultracam_dbase.fits')
#  hdu     = pyfits.open('ultracam_dbase.fits')
  table   = hdu[1].data
  ras     = table.field('RA')
  decs    = table.field('Dec')
  runs    = table.field('Run')
  nights  = table.field('Night')
  numbers = table.field('Num')
  targets = table.field('Target')
  ids     = table.field('ID')
  exps    = table.field('Exposure')
  coms    = table.field('Comment')

  ok = (ras > ra1) & (ras < ra2) & (decs > dec1) & (decs < dec2) & (exps > emin)
  if simbad_failed:
    print 'SIMBAD lookup failed; reverted to default manual search limits.'

  print 'There were',len(ras[ok]),'matches out of a possible ' + str(len(ras)) + '. '

  if len(ras[ok]):

    if unique:
      keys = np.array([str(ra) + str(dec) + night for ra,dec,night in zip(ras[ok],decs[ok],nights[ok])])
      u, uk = np.unique(keys, return_index=True)
      print 'A subset of ' + str(len(ras[ok][uk])) + ' of these will be printed.\n<p>'
    else:
      print '\n<p>'

    print '<table border="2" cellpadding="2" cellspacing="1">'
    print '<tr><th>RA<br>hours</th><th>Dec<br>deg</th><th>Run<br>yyyy-mm</th><th>Night<br>yyyy-mm-dd</th><th class="cen">Number<br>of run</th><th>Target</th><th>ID</th>'
    print '<th>Exp.<br>mins</th><th>Comment</th></tr>'

    if unique:
      for ra,dec,run,night,number,target,iden,expo,comm in \
            zip(ras[ok][uk],decs[ok][uk],runs[ok][uk],nights[ok][uk],numbers[ok][uk],targets[ok][uk],ids[ok][uk],exps[ok][uk],coms[ok][uk]):
        print '<tr><td> %6.3f </td><td> %7.3f </td><td> %s </td><td> <a href="%s/%s/%s.htm" target="_blank">%s</a></td><td class="cen"> %s </td><td> %s </td><td> %s </td>' \
            % (ra,dec,run,run,night,night,night,number,target,iden)
        print '<td> %5.1f </td><td class="left"> %s </td></tr>' % (expo,comm)
    else:
      for ra,dec,run,night,number,target,iden,expo,comm in zip(ras[ok],decs[ok],runs[ok],nights[ok],numbers[ok],targets[ok],ids[ok],exps[ok],coms[ok]):
        print '<tr><td> %6.3f </td><td> %7.3f </td><td> %s </td><td> <a href="%s/%s/%s.htm" target="_blank">%s</a> </td><td class="cen"> %s </td><td> %s </td><td> %s </td>' \
            % (ra,dec,run,run,night,night,night,number,target,iden)
        print '<td> %5.1f </td><td class="left"> %s </td></tr>' % (expo,comm)
    print '</table>'
  else:
    print 'Try slackening your criteria.'

if __name__ == "__main__":
  import sys
  main(sys.argv[1:])
