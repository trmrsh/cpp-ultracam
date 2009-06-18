#!/usr/bin/env python

import sys
import os.path
import numpy as npy
import numdisplay as ndpy
import trm.ucm as ucm

if len(sys.argv) < 2:
    file = raw_input('Name of ucm file: ')
else:
    file = sys.argv[1]

file = file if file.endswith('.ucm') else file + '.ucm'

if not os.path.isfile(file):
    print file,' is not a file.'
    exit(1)

file = file if file.endswith('.ucm') else file + '.ucm'

ucm = ucm.rucm(file)

print 'read ucm file'

if len(sys.argv) < 3 and len(ucm.data) > 1:
    nccd = int(raw_input('CCD number to display: '))
elif len(ucm.data) > 1:
    nccd = int(sys.argv[2])
else:
    nccd = 1

xbin = ucm.xbin
ybin = ucm.ybin
image = npy.zeros((ucm.nxtot//xbin,ucm.nytot//ybin))

for i in range(len(ucm.data[nccd-1])):
    win = ucm.data[nccd-1][i]
    (llx,lly) = ucm.off[nccd-1][i]
    x1 = (llx-1)/xbin+1
    y1 = (lly-1)/ybin+1
    image[y1:y1+win.shape[0],x1:x1+win.shape[1]] = win


ndpy.display(image)
