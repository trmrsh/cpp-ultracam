#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""
script to generate a form for accessing the ULTRACAM run database
"""

import pyfits
import numpy as np

def main(argv):

  print '<p> Arguments =' + '|'.join(argv) + '|\n<p>'
  
  if len(argv)  >= 4:
    ra1, ra2, dec1, dec2 = argv[:4]

    def set(inp, default):
      try:
        return float(inp)
      except:
        return default

    ra1  = set(ra1, 13.)
    ra2  = set(ra2, 14.)
    dec1 = set(dec1, -20.)
    dec2 = set(dec2, 30.)
    if len(argv) >= 5:
      unique = True
    else:
      unique = False

  else:
    ra1, ra2, dec1, dec2, unique = 13., 14., -20., 20., False

  #print first row of table:
  print '<h2>Selection criteria:</h2>\n<p>'
  print '<form method="get" action="ulogs.php">'
  print '<table>'
  print '<tr><td>Lower RA limit:</td><td><input id="RA1"  name="RA1" type="text" value="%6.3f" size="6" maxlength="6" onKeyPress="return submitenter(this,event)"></td>' % ra1
  print '<td>Upper RA limit: </td><td> <input id="RA1"  name="RA2" type="text" value="%6.3f" size="6" maxlength="6" onKeyPress="return submitenter(this,event)"><td></tr>' % ra2
  print '<tr><td>Lower Dec limit: </td><td> <input id="Dec1"  name="Dec1" type="text" value="%7.3f" size="7" maxlength="7" onKeyPress="return submitenter(this,event)"></td>' % dec1
  print '<td>Upper Dec limit: </td><td> <input id="Dec2"  name="Dec2" type="text" value="%7.3f" size="7" maxlength="7" onKeyPress="return submitenter(this,event)"></td></tr>' % dec2
  print '<tr><td colspan="4"> Print only one run per night per object? <input type="checkbox" id="unique" name="unique" value="unique"',
  if unique: print ' checked',
  print '></td></tr>'
  print '</table>'
  print '</form><br>\n<br>\n'

  print '<p>\n<h2>Matches:</h2>\n<p>'

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

  ok = (ras > ra1) & (ras < ra2) & (decs > dec1) & (decs < dec2)
  print 'There were',len(ras[ok]),'matches out of a possible',len(ras)

  if len(ras[ok]):

    if unique:
      keys = np.array([str(ra) + str(dec) + night for ra,dec,night in zip(ras[ok],decs[ok],nights[ok])])
      u, uk = np.unique(keys, return_index=True)
      print 'A subset of ' + str(len(ras[ok][uk])) + ' of these will be printed.\n<p>'
    else:
      print '\n<p>'

    print '<table border="2" cellpadding="2" cellspacing="1">'
    print '<tr><th>RA<br>hours</th><th>Dec<br>deg</th><th>Run<br>yyyy-mm</th><th>Night<br>yyyy-mm-dd</th><th class="cen">Number<br>of run</th><th>Target</th><th>ID</th></tr>'

    if unique:
      for ra,dec,run,night,number,target,iden in zip(ras[ok][uk],decs[ok][uk],runs[ok][uk],nights[ok][uk],numbers[ok][uk],targets[ok][uk],ids[ok][uk]):
        print '<tr><td> %6.3f </td><td> %7.3f </td><td> %s </td><td> <a href="%s/%s/%s.htm">%s</a></td><td class="cen"> %s </td><td> %s </td><td> %s </td></tr>' \
            % (ra,dec,run,run,night,night,night,number,target,iden)
    else:
      for ra,dec,run,night,number,target,iden in zip(ras[ok],decs[ok],runs[ok],nights[ok],numbers[ok],targets[ok],ids[ok]):
        print '<tr><td> %6.3f </td><td> %7.3f </td><td> %s </td><td> <a href="%s/%s/%s.htm">%s</a> </td><td class="cen"> %s </td><td> %s </td><td> %s </td></tr>' \
            % (ra,dec,run,run,night,night,night,number,target,iden)
    print '</table>'
  else:
    print 'Try slackening your criteria.'

if __name__ == "__main__":
  import sys
  main(sys.argv[1:])
