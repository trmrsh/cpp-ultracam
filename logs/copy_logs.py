#!/usr/bin/env python

"""
Copies files needed for online logs for ultraspec, ultracam
This includes the night logs and some extras. They are
copied into a single directory at the server end. The script
should be invoked from the respective logs directory (checked)
"""
import os, re, shutil

cwd = os.getcwd()
if cwd.find('ultracam') > -1:
    instrument = 'ultracam'
elif cwd.find('ultraspec') > -1:
    instrument = 'ultraspec'
else:
    print 'Could not recognise instrument from ' + cwd
    print 'please fix.'
    exit(1)

if not cwd.endswith(os.path.join(instrument,'logs')) > -1:
    print 'This script must be run from the logs directory'
    exit(1)

wpath = os.path.join(os.environ['WEB_PATH'], instrument, 'logs')

extras = ('index.html', 'guide.html', 'ultra.css', 'ultra_logs.js',
          'ultra_search.html', 'ultra_search.js', 'ultra.json',
          'targets.html', 'targets.js')

dre = re.compile('^\d\d\d\d-\d\d-\d\d$')

ndirs = [dir for dir in os.listdir('.') if dre.match(dir)]

# copy night logs first (only if they have changed)
for dir in ndirs:
    llog = os.path.join(dir, dir + '.html')
    if os.path.exists(llog):
        wlog = os.path.join(wpath, dir + '.html')
        if not os.path.exists(wlog) or \
                os.path.getmtime(llog) > os.path.getmtime(wlog) + 0.01:
            print llog,'-->',wlog
            shutil.copy2(llog, wlog)
            os.chmod(wlog, 0o644)

# copy extras (which could well be links)
for lname in extras:
    wname = os.path.join(wpath, lname)
    tname = os.path.realpath(lname)
    if not os.path.exists(wname) or \
            os.path.getmtime(tname) > os.path.getmtime(wname) + 0.01:
        print lname,'-->',wname
        shutil.copy2(lname, wname)
        os.chmod(wname, 0o644)


