#!/usr/bin/env python

"""
Script to read messages from the ULTRACAM data comments folder to 
then compile into a database.
"""

import os, errno, mimetypes, imaplib, time
import email.Parser, email.Utils
import numpy as np
import pyfits

import trm.subs as subs
from trm import forms

print '\nTime = ',time.ctime()

# connect to imap server
imap_user   = 'tom.r.marsh@gmail.com'
imap_server = 'imap.gmail.com'
imap_pword  = 'Tom61%Fel62'
imap_port   = 993

# connect to the imap server
imap = imaplib.IMAP4_SSL(imap_server, imap_port)

# login
imap.login(imap_user, imap_pword)

fday    = {}
results = {}
targets = {}
parser  = email.Parser.Parser()

folder = 'UCAM data comment'
    
print 'Searching folder =',folder

# select folder to search
imap.select(folder)
    
# search for data comments messages
r, data = imap.search(None, 'SUBJECT', 'ULTRACAM data comment submission')

# process message by message, first skipping headers
# and then looking for the beginning of each message
# the results are saved to 'results'

if data[0] == '':
    print 'No data comments found.'
else:
    print len(data[0].split()),'data comments found'

day     = []
when    = []
dtype   = []
run     = []
user    = []
comment = []

for num in data[0].split():

    ret, mess = imap.fetch(num, '(RFC822)')
    msg = parser.parsestr(mess[0][1])

    wdate = msg.__getitem__('Date')
    when.append(wdate)
    day.append(forms.ttime(wdate))

    # now we work through the message
    for part in msg.walk():
        if part.get_content_maintype() == 'multipart':
            # skip the multipart bits
            continue

        elif part.get_content_type() == 'text/plain':
                
            # this is where the fields entered get parsed, fixed and checked.
            fields = forms.genparse(part, ('User', 'Date', 'Run', 'Data type', 'Comment') )
            dtype.append(fields['Data type'])
            run.append(fields['Date'] + '/run%03d' %(int(fields['Run']),))
            user.append(fields['User'])
            comment.append(fields['Comment'])
            
        else:
            print 'Content type = ' + part.get_content_type() + ' not recognised.'


print 'Finished with folder =',folder

# OK, close down and logout of IMAP folders
imap.close()
imap.logout()

day     = np.array(day)
when    = np.array(when)
dtype   = np.array(dtype)
run     = np.array(run)
user    = np.array(user)
comment = np.array(comment)

# Sort on arrival time
isort   = np.argsort(day)

day     = day[isort]

when    = when[isort]
dtype   = dtype[isort]
run     = run[isort]
user    = user[isort]
comment = comment[isort]

# Construct FITS file
crun     = pyfits.Column(name='Run', format=str(run.dtype.itemsize) + 'A', array=run)
cdtype   = pyfits.Column(name='Dtype', format=str(dtype.dtype.itemsize) + 'A', array=dtype)
cuser    = pyfits.Column(name='Username', format=str(user.dtype.itemsize) + 'A', array=user)
cwhen    = pyfits.Column(name='Submitted', format=str(when.dtype.itemsize) + 'A', array=when)
ccomment = pyfits.Column(name='Comment', format=str(comment.dtype.itemsize) + 'A', array=comment)

cols     = pyfits.ColDefs([crun,cdtype,cuser,cwhen,ccomment])

dbase = 'ultracam_comments.fits'
tbhdu = pyfits.new_table(cols)
hdu   = pyfits.PrimaryHDU()
hdul  = pyfits.HDUList([hdu, tbhdu])
hdul.writeto(dbase,clobber=True)

print len(run),'comments written to',dbase

