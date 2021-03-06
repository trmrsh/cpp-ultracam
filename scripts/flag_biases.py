#!/usr/bin/env python
#
# Uses results from procbias to flag up which frames are biases,
# have been processed, and therefore can be moved. Run in top level
# ultracam data directory.

import sys, os, subprocess

bpath = 'calib/biases'

fcheck = [os.path.join(bpath, 'Gold.html'), os.path.join(bpath, 'Silver.html')]

runs = []
for fname in fcheck:
   if os.path.isfile(fname):
      fin = open(fname)
      for line in fin:
         if line.startswith('<tr><td><a href'):
            first = line.find('"')
            if first == -1:
               print 'Could not find first " in ',line
               exit(1)
            last = line[first+1:].find('"')
            if  last == -1:
               print 'Could not find second " in ',line
               exit(1)
            last += first + 1
            fname = line[first+1:last]
            if not os.path.isfile(os.path.join(bpath, fname)):
               print 'File =',fname,'is listed but does not exist in',bpath
               exit(1)
            runs.append(line[first+1:last])
      fin.close()

for run in runs:
   year  = int(run[:4])
   month = int(run[4:6])
   day   = int(run[6:8])
   rno   = int(run[10:13])
   dir   = '%04d-%02d-%02d' % (year,month,day)
   rn    = 'run%03d' % rno

   xml = os.path.join(dir, rn + '.xml')
   mov = os.path.join(dir, rn + '.mov')
   jnk = os.path.join(dir, rn + '.jnk')
   if os.path.exists(jnk):
      print 'Unexpected presence of',jnk,'prevents a .mov file from being created'
   elif not os.path.exists(xml):
      print 'Unexpected absence of',xml,'prevents a .mov file from being created'
   elif not os.path.exists(mov):
#         fout = open(mov,'w')
#         fout.write(run + ' is thought to be a bias and has been combined into one\n')
#         fout.write('called ' + run + '\n')
#         fout.write('Therefore the data file can be archived.\n')
#         fout.write('This file was generated by' + sys.argv[0] + '\n')
      print 'Written',mov,run

