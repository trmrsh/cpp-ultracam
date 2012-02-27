#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Script to generate a form for commenting on ULTRACAM runs. Rather horrible
logic as follows:

This should be called from the file ucomment.php which is little more than a
wrapper.  The php file should be called the 'date', 'run', 'user' and 'wtype'
fields set to avoid unecessary prompting. The 'attribute' field should not be
set. This indicates that it is the first call to this python script.

The script generates a form requiring a few entries from the user. This
includes some javascript in the php wrapper script to hide/reveal a file
upload option if the 'Reduced' option is selected. On submission the new
fields are passed to ucomment.php again. This time the attribute field should
be set and, if all checks on input fields are passed, the values are sent by
email to me.
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

  # If called from ucomment.php, this will always have 8 arguments. 
  # The != 8 case is for testing
  if len(argv) != 8:
    print 'ucomment.py was not correctly invoked. It needs 8 arguments.'
    print len(argv),'were supplied.'
    user, wtype, date, run, attribute, comment, ofile, tfile = \
        'Tom Marsh', 'short', '2000-01-01', '23', 'Junk', 'Test message', '', ''
  else:
    user, wtype, date, run, attribute, comment, ofile, tfile = argv

    if not check_inputs(date, run):
      return

  # Check whether this is the first pass. If attribute is set, assume that it is not.
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
    if attribute == 'Noselection':
      print 'You must select an attribute; choose "Unspecified" if your comment is not'
      print 'indicating a change of data attribute.'
      ok = False
    elif attribute not in ATTRIBUTES:
      print 'Data attribute = "' + attribute + '" is not recognised.<br>'
      ok = False

    # the comment
    if comment == '' or comment.isspace():
      print 'You must enter some sort of comment.<br>'
      ok = False
    else:
      comment = re.sub('[ \n\r\t]+',' ',comment)

    # the file, if any
    if ofile != '' and not ofile.endswith('.fits.gz'):
      print 'Only .fits.gz files can be uploaded.<br>'
      ok = False

    if ok: 
      print 'Thank you for your submission:<br><br>'
      print '<table>'
      print '<tr valign="top"><td class="left">User:</td><td class="left">' + user + '</td></tr>'
      print '<tr valign="top"><td class="left">Date:</td><td class="left">' + date + '</td></tr>'
      print '<tr valign="top"><td class="left">Run:</td><td class="left">' + str(run) + '</td></tr>'
      print '<tr valign="top"><td class="left">Data type:</td><td class="left">' + attribute + '</td></tr>'
      print '<tr valign="top"><td class="left">Comment:</td><td class="left">' + comment + '</td></tr>'
      if ofile == '':
        print '<tr valign="top"><td class="left">Reduce log:</td><td class="left">None uploaded</td></tr>'
      else:
        print '<tr valign="top"><td class="left">Reduce log:</td><td class="left">' + ofile + '</td></tr>'
      print '</table>\n'

      # compose message with an optional attachment
      from email import Encoders
      from email.MIMEMultipart import MIMEMultipart
      from email.MIMEBase import MIMEBase
      from email.MIMEText import MIMEText

      msg = MIMEMultipart()
      msg['From']    = 'no-reply@warwick.ac.uk'
      msg['To']      = 'tom.r.marsh@gmail.com'
      msg['Subject'] = 'ULTRACAM data comment submission'

      msg.attach(MIMEText('User:\n' + user + '\n\n' + \
                            'Date:\n' + date + '\n\n' + \
                            'Run:\n' + run + '\n\n' + \
                            'Data type:\n' + attribute + '\n\n' + \
                            'File:\n' + ofile + '\n\n' + \
                            'Comment:\n' + comment + '\n\n', 'plain'))
 
      # Now read and pack the file
      if ofile != '' and tfile != '': 
        import os
        if os.path.exists(tfile):
          fp  = open(tfile, 'rb')
          b   = fp.read()
          fp.close()

          part = MIMEBase('application', "octet-stream")
          part.set_payload(b)
          Encoders.encode_base64(part)
          part.add_header('Content-Disposition', \
                            'attachment; filename="%s"' % ofile)
          msg.attach(part)
        else:
          print 'Cannot find temporary file =',tfile,'<br>'
            
      # send the message, inform the user.
      import smtplib
      server = smtplib.SMTP('mail-relay.warwick.ac.uk')
#      server.set_debuglevel(1)
      reply = server.sendmail('no-reply@warwick.ac.uk', \
                              'tom.r.marsh@gmail.com', msg.as_string())
      server.quit()

      print '\n<br><br>\nAn e-mail has been sent.<br>\n\n'

      print '<form action="./%s/%s_%s.html">' % (date,date,wtype)
      print '<input type="submit" value="Return to logs">'
      print '</form>'

      return

    else:
      print '<strong>Invalid form input; please try again.</strong>'

  else:
    print '<h1>Comment on %s/run%03d</h1>' % (date, int(run),)

  if user == 'ultracam':
    print """
<p><hr>

<p>
Dear ULTRACAM user, this link would normally take you to a form which would allow you to submit a comment on 
a run, however the facility is not available under the generic access account 'ultracam'. If you are reducing 
ultracam data and find the logs to be wrong in some way and therefore think that a comment is warranted, please 
contact me for a specific account. I welcome such comments for improvement of the archive so would be more than 
happy to set one up for you.
"""
  else:
    print """
<p><hr>

<p>
<form method="post" action="ucomment.php" enctype="multipart/form-data" name="dcform"
onsubmit="return validate();">
"""
    print '<input type="hidden" name="user" value="%s">'  % (user,)
    print '<input type="hidden" name="date" value="%s">'  % (date,)
    print '<input type="hidden" name="run" value="%s">'   % (run,)
    print '<input type="hidden" name="wtype" value="%s">' % (wtype,)

  # this is the data attribute line
    print """
<input type="hidden" name="MAX_FILE_SIZE" value="8000000" />

<table>

<tr>
<td class="left"><a href="help_data_type.html">Data type:</a></td> 
<td class="left">
<select name="attribute" size=1 id="attsel" 
title="Defines a new attribute of the run for automated classification. Click 'Data type' for details." 
onchange="display(this);" />
"""
    print '<option value="Noselection">' + ' -- select attribute --'
    for att in ATTRIBUTES:
      print '<option value="%s"> %s' % (att,att)

  # coming up is a file upload option that should only be displayed if the 
  # reduced option has been chosen. Finally there is arbitrary comment field.

    print """
</select>
</td></tr>

<tr id="fupload" valign="top" style="display: none;">
<td class="left">Reduce log:</td>
<td class="left">
<input type="file" name="redfile" size="40" id="upload"
title="Upload the reduce log file in .fits.gz format">
</td></tr>

<tr valign="top">
<td class="left">Comment:</td>
<td class="left">
<textarea name="comment" rows="5" cols="40" wrap="hard" id="comment"
title="Paragraph breaks will be ignored. Keep comments short.">
</textarea>
</td></tr>

</table>
<input type="submit" value="Submit">
"""

    print '<input type="button" value="Cancel" onClick="location.href=\'./%s/%s_%s.html\';">' % (date,date,wtype)
    print '</form>'

if __name__ == "__main__":
  import sys
  main(sys.argv[1:])
