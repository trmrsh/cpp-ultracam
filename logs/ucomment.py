#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
script to generate a form for commenting on ULTRACAM runs.
"""

import re

# tuple of attributes
ATTRIBUTES = ('Unspecified','Junk','Good','Reduced',\
              'Acquisition','Bias','Dark','Science','Flat','Technical')

def check_inputs(date, run):

  # first the date
  dre = re.compile('(\d\d\d\d)-(\d\d)-(\d\d)$')
  m = dre.match(date)
  if m:
    year, month, day  = m.group(1,2,3)
    year  = int(year)
    month = int(month)
    day   = int(day)
    if year < 2002 or year > 2020:
      print 'Year =',year,'is out of range 2002 to 2020.<br>'
      return False
    if month < 1 or month > 12:
      print 'Month =',month,'is out of range 1 to 12.<br>'
      return False
    if day < 1 or month > 31:
      print 'Day =',day,'is out of range 1 to 31.<br>'
      return False
  else:
    print 'Date = "' + date + '" does not have a YYYY-MM-DD format.<br>'
    return False

  # second the run
  try:
    run = int(run)
    if run < 1 or run > 999:
      print 'Run =',run,'is out of range 1 to 999.<br>'
      return False
  except ValueError:
    print 'Run = "' + run + '" is not an integer.<br>'
    return False 
 
  return True

def main(argv):

  # If called from ucomment.php, this will always have 6 arguments. 
  # The != 6 case is for testing
  if len(argv) != 6:
    print 'ucomment.py was not correctly invoked. It needs 6 arguments.'
    print len(argv),'were supplied.'
    user, wtype, date, run, attribute, comment = \
        'Tom Marsh', 'short', '2000-01-01', '23', 'Junk', 'Test message'
  else:
    user, wtype, date, run, attribute, comment = argv

    if not check_inputs(date, run):
      return

  # Check whether this is the first pass
  if attribute != '':
    print '<h1>Data comment submission results</h1>\n\n<p>'
    ok = True

    # the calling page type
    if wtype != 'short' and wtype != 'full':
      print 'Page calling type should be "short" or "full".<br>'
      ok = False

    # the name
    if user == '' or user.isspace():
      print 'You must enter a name.<br>'
      ok = False

    # the attribute    
    if attribute not in ATTRIBUTES:
      print 'Data attribute = "' + attribute + '" is not recognised.<br>'
      ok = False

    # the comment
    if comment == '' or comment.isspace():
      print 'You must enter some sort of comment.<br>'
      ok = False
    else:
      comment = re.sub('[ \n\r\t]+',' ',comment)

    if ok: 
      print 'Thank you for your submission:<br><br>'
      print '<table>'
      print '<tr valign="top"><td class="left">User:</td><td class="left">' + user + '</td></tr>'
      print '<tr valign="top"><td class="left">Date:</td><td class="left">' + date + '</td></tr>'
      print '<tr valign="top"><td class="left">Run:</td><td class="left">' + str(run) + '</td></tr>'
      print '<tr valign="top"><td class="left">Data type:</td><td class="left">' + attribute + '</td></tr>'
      print '<tr valign="top"><td class="left">Comment:</td><td class="left">' + comment + '</td></tr>'
      print '</table>\n'

      # compose a message
      import smtplib

      msg = 'From: no-reply@warwick.ac.uk\r\n' + \
          'To: tom.r.marsh@gmail.com\r\n' + \
          'Subject: ULTRACAM data comment submission\r\n\r\n' + \
          'User:\n' + user + '\n\n' + \
          'Date:\n' + date + '\n\n' + \
          'Run:\n' + run + '\n\n' + \
          'Data type:\n' + attribute + '\n\n' + \
          'Comment:\n' + comment + '\n\n'

      server = smtplib.SMTP('mail-relay.warwick.ac.uk')
#      server.set_debuglevel(1)
      reply = server.sendmail('no-reply@warwick.ac.uk', 'tom.r.marsh@gmail.com', msg)
      server.quit()

      print '\n<br><br>\nAn e-mail has been sent.<br>\n\n'

      print '<form action="./%s/%s_%s.html">' % (date,date,wtype)
      print '<input type="submit" value="Return to logs">'
      print '</form>'

      return

    else:
      print '<strong>Invalid form input; please try again.</strong>'

  else:
    print '<h1>Comment on %s/run%03d from user = %s</h1>' % (date, int(run), user)

  # Print the form
  print """
<p><hr>

<p>
<form method="get" action="ucomment.php">
"""

  print '<input type="hidden" name="user" value="%s">'  % (user,)
  print '<input type="hidden" name="date" value="%s">'  % (date,)
  print '<input type="hidden" name="run" value="%s">'   % (run,)
  print '<input type="hidden" name="wtype" value="%s">' % (wtype,)

  print """
<table>
<tr><td class="left"><a href="help_data_type.html">Data type:</a></td> <td class="left">
<select name="attribute" size=1 title="Defines a new attribute of the run for automated 
classification. Click 'Data type' for details.">
"""
  for att in ATTRIBUTES:
    print '<option>' + att

  print """
</select>
</td></tr>
<tr valign="top"><td class="left">Comment:</td>
<td class="left">
<textarea name="comment" rows="5" cols="40" wrap="hard" title="Paragraph breaks will be ignored. Keep comments short."></textarea>
</td></tr>
</table>
<input type="submit" value="Submit">
"""
  print '<input type="button" value="Cancel" onClick="location.href=\'./%s/%s_%s.html\';">' % (date,date,wtype)
  print '</form>'

if __name__ == "__main__":
  import sys
  main(sys.argv[1:])
