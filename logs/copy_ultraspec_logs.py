#!/usr/bin/env python

"""
Copies files needed for online logs for ultraspec.
This includes the night logs and some extras. They are
copied into a single directory at the server end.
"""

import os, re, shutil

wpath = os.path.join(os.environ['WEB_PATH'], 'ultraspec', 'logs')

extras = ('index.html', 'guide.html', 'ultra.css', 'ultra.js',
          'ultra_logs.js', 'ultra_search.html', 'ultra_search.js')

dre = re.compile('^\d\d\d\d-\d\d-\d\d$')

ndirs = [dir for dir in os.listdir('.') if dre.match(dir)]

# copy night logs first (only if they have changed)
for dir in ndirs:
    llog = os.path.join(dir, dir + '.html')
    if os.path.exists(llog):
        wlog = os.path.join(wpath, dir + '.html')
        if not os.path.exists(wlog) or \
                os.path.getmtime(wlog) < os.path.getmtime(llog):
            print llog,'-->',wlog
            shutil.copy2(llog, wlog)
            os.chmod(wlog, 0o644)

# copy extras
for lname in extras:
    wname = os.path.join(wpath, lname)
    if not os.path.exists(wname) or \
            os.path.getmtime(wname) < os.path.getmtime(lname):
        print lname,'-->',wname
        shutil.copy2(lname, wname)
        os.chmod(wname, 0o644)


