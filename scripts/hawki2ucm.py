#!/usr/bin/env python

"""
Converts HAWK-I frames into ucm format.
"""

import sys
import pyfits
from trm import subs, ucm

if len(sys.argv) < 2:
    print 'usage: fits1 fits2 ...'
    exit(1)

# loop through files
for fname in sys.argv[1:]:

    if fname.endswith('.fits'):
        uname = fname[:-4] + 'ucm'
    elif fname.endswith('.fit'):
        uname = fname[:-4] + 'ucm'
    else:
        print 'File =',fname,'does not end in .fit or .fits'
        print 'bllluurrgggh'
        exit(1)

    # open frame
    hdul = pyfits.open(fname)

    # image contains single image
    image  = hdul[0].data
    header = hdul[0].header

    hdul.close()

    # dimensions
    ny, nx = image.shape

    # split into the four windows:
    win1 = image[:ny/2,:nx/2]
    win2 = image[:ny/2,nx/2:]
    win3 = image[ny/2:,:nx/2]
    win4 = image[ny/2:,nx/2:]

    # build into the ucm format. See trm.ucm.Ucm for details
    # basically it is a matter of creating data, a header and offsets
    # of the right format for the Ucm constructor. These are called "data",
    # "head" and "off" below. The header is probably the trickiest part:

    head = subs.Odict()

    head['Object'] = {'comment': 'object name from HAWK-I file', \
                          'type' : ucm.ITYPE_STRING, 'value' : header['OBJECT']}

    # convert time -- don't know if this is really exoposure centre (??)
    mjd  = header['MJD-OBS']
    imjd = int(mjd)
    hour = 24.*(mjd-imjd)

    head['UTC_date'] = {'comment' : 'UT at centre of exposure', 'type' : ucm.ITYPE_TIME, \
                            'value' : (imjd, hour)}

    # exposure time
    head['Exposure'] = {'comment' : 'exposure time, seconds', 'type' : ucm.ITYPE_FLOAT, \
                            'value' : header['EXPTIME']}

    # maximum dimensions
    nxtot, nytot = 4096, 4096

    # binning factors (??)
    xbin, ybin = 1, 1

    # list of lists. Outermost level is per detector. HAWK-I has
    # 4, but in this case, will pretend its a single detector, so
    # just one element, an empty list in this list
    data = [[],]

    # add in the windows
    data[0].append(win1)
    data[0].append(win2)
    data[0].append(win3)
    data[0].append(win4)

    # now extract and apply x, y offsets -- interpretation needs checking (??)
    startx  = header['STARTX']
    starty  = header['STARTY']
    starty2 = header['STARTY2']

    # just like data, a list of lists for the window offsets, with
    # just one set of offsets for one detector. Each offset is (llx,lly)
    # giving the coordinate of the lower-left pixel of a window.

    off = [[],]
    off[0].append((startx,starty))
    off[0].append((startx+nxtot/2,starty))
    off[0].append((startx,starty2))
    off[0].append((startx+nxtot/2,starty2))

    # OK, all is ready to create a Ucm
    ufile = ucm.Ucm(head, data, off, xbin, ybin, nxtot, nytot)
    ufile.write(uname)

    print fname,'read, converted and written to',uname
