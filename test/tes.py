#!/usr/bin/env python

import numpy
from ppgplot import *

a = numpy.zeros((10,10),'int16')

a[5][5] = 20.
a[4][4] = 15.
a[3][3] = 10.

pgopen('/xs')
pgvstd()
pgwnad(0.,10.,0.,10.)
tr = numpy.array([0.,1.,0.,0.,0.,1.])
pggray(a,10,10,0,9,0,9,20.,0.,tr)
pgclos()
