#!/usr/bin/env python

"""
Support routines for Python analysis of Ultracam & Ultraspec
files. Mainly for database work.
"""

import os, sys, re, traceback, math
from xml.dom import Node
from xml.dom.minidom import parse, parseString
import trm.subs as subs
import trm.simbad as simbad
import trm.sla as sla

# couple of helper functions
def cosd(deg):
    return math.cos(math.radians(deg))

def sind(deg):
    return math.sin(math.radians(deg))

class Log(object):
    """
    Class to read and store log file data. These come in two formats:

    1) Old style: run, target name, filters, comment
    2) New style: run, comment (target names are in the xml files)

    The class just stores the data in a couple of dictionaries
    'comment' and 'target'; 'format' is an integer specifying 
    the format as above. 'target' is blank in the case of format == 2.
    """

    def __init__(self, fname):
        """
        Constructs a new Log given a file. Makes empty
        dictionaries if none found and reports an error
        """
        self.format  = 2
        self.target  = {}
        self.filters = {}
        self.comment = {}

        try:
            rec    = re.compile('file\s+object\s+filter', re.I)
            old    = re.compile('\s*(\S+)\s+(\S+)\s+(.*)$')
            oldii  = re.compile('\s*(\S+)\s*$')
            f  = open(fname)
            for line in f:
                m = rec.search(line)
                if m:
                    self.format = 1
                    if len(self.comment):
                        raise Exception('Error in night log = ' + fname + ', line = ' + line)

                if line.startswith('run'):
                    num = int(line[3:6])
                    if self.format == 2:
                        self.comment[num] = line[6:].strip()
                    else:
                        m = old.search(line[6:])
                        if m:
                            self.target[num]  = m.group(1)
                            self.filters[num] = m.group(2)
                            self.comment[num] = m.group(3)
                        else:
                            m = oldii.search(line[6:])
                            if m:
                                self.target[num]  = m.group(1)

        except Exception, err:
            sys.stderr.write('Night log problem:' + str(err) + '\n')

class Times(object):
    """
    Class to read and store timing data files

    Attributes (all dictionaries keyed on run number):

    date    -- dates (start of run)
    utstart -- UT at start
    utend   -- UT at end
    nframe  -- number of exposures
    expose  -- total exposure, seconds
    sample  -- sampling time, seconds
    """

    def __init__(self, fname):
        """
        Constructs a new Times given a file name. Makes empty
        dictionaries if none found and reports an error
        """
        self.date    = {}
        self.utstart = {}
        self.utend   = {}
        self.nframe  = {}
        self.expose  = {}
        self.sample  = {}
        try:
            f  = open(fname)
            for line in f:
                if line.startswith('run'):
                    num = int(line[3:6])
                    self.date[num],self.utstart[num],self.utend[num],self.nframe[num],self.expose[num],self.sample[num] = line[6:].split()
        except Exception, err:
            sys.stderr.write('File = ' + fname + ', timing data problem: ' + str(err) + '\n')

class Targets(dict):
    """
    Class to read and store the target positions and regular expressions. 

    It is a dictionary keyed on target names. Each item is in turn a dictionary
    with the following keys:

    'ra'     -- RA (decimal hours)
    'dec'    -- Dec (decimal degrees)
    'match'  -- A list of tuples. Each tuples has a matching expression and a flag. If the flag is True, it indicates
                that an exact match is needed while it is false regular expression matchin is used. In this case the tuple
                aends with a compiled version of the regular expression to spped processing
    """

    def __init__(self, *fnames):
        """
        Constructs a new Targets object. Makes empty
        dictionaries if none found and reports an error. Multiple
        file names can be specified; any which do not exist will be skipped.
        The files must have the format
        
        name hh mm ss.ss dd mm ss.ss E expr1 R expr2 E expr3 ...
       
        where name is the name that will be attached to the target dd include the 
        declination sign. E = exact, R = regex matching using the following expression.
        These can come in any order.
        """

        try:
            for fname in fnames:
                if os.path.isfile(fname):
                    f  = open(fname)
                    for line in f:
                        if not line.startswith('#') and line[:1].isalnum():
                            tokens = line.strip().split()
                            if len(tokens) >= 9 and (len(tokens)-7) % 2 == 0:
                                (target,rah,ram,ras,decd,decm,decs) = tokens[:7]
                                match = []
                                for i in range(7, len(tokens), 2):
                                    if tokens[i] == 'E':
                                        exact = True
                                    elif tokens[i] == 'R':
                                        exact = False
                                    else:
                                        raise Exception('Invalid target line = ' + line + '; could not recognise match type = ' + tokens[i])
                                        
                                    if tokens[i+1] in match:
                                        raise Exception('Repeated match expression in line = ' + line)

                                    if exact:
                                        match.append((tokens[i+1].replace('~',' '), exact))
                                    else:
                                        match.append((tokens[i+1], exact, re.compile(tokens[i+1])))
                            else:
                                raise Exception('Invalid target line = ' + line)

                            (ra,dec,system) = subs.str2radec(rah + ' ' + ram + ' ' + ras + ' ' + decd + ' ' + decm + ' ' + decs)
                            target = target.strip().replace('~',' ')

                            if target in self:
                                raise Exception('Found target name = "' + target + '" more than once in ' + str(fnames))
                            self[target] = {'ra' : ra, 'dec' : dec, 'match' : match}
                    f.close()
                    print len(self),'targets after loading',fname
                else:
                    print 'No targets loaded from',fname,'as it does not exist.'
        except Exception, err:
            sys.stderr.write('Target data problem: ' + str(err) + '\n')
            if line is not None: sys.stderr.write('Line: ' + line + '\n')

    def write(self, fname):
        """
        Write targets out to disk file fname.
        """

        # write in RA order
        ras   = dict([(targ,entry['ra']) for targ, entry in self.iteritems()])
        targs = sorted(ras, key=ras.get)

        f = open(fname,'w')
        f.write("""
#
# File of targets written by Ultra.Targets.write
#

""")

        for targ in targs:
            entry = self[targ]
            pos = subs.d2hms(entry['ra'],sep=' ') + '   ' + subs.d2hms(entry['dec'],sep=' ',sign=True)
            f.write('%-35s %s' % (targ.replace(' ','~'),pos))
            for ent in entry['match']:
                f.write(' ' + ('E' if ent[1] else 'R') + ' ' + ent[0].replace(' ','~'))
            f.write('\n')
        f.close()

class Run(object):
    """
    This is the main class for reading and storing data on a per run basis,
    including collating data from times and night log files. The aim is that
    it contains ALL the information for a given run.

    Static data Run.FUSSY controls the meaning of the equality operator '=='.
    If FUSSY = True then absolutely every format parameter must match. FUSSY = False
    then it will ignore differences in the avalanche gain of ULTRASPEC.
    """

    FUSSY = True
    RESPC = re.compile('\s+')

    def __init__(self, xml, log=None, times=None, targets=None, telescope=None, night=None, run=None, sskip=None, warn=False):
        """
        xml       -- xml file name with format run###.xml
        log       -- previously read night log
        times     -- previously read timing data
        targets   -- previously read target position data
        telescope -- telescope; names from XML cannot be relied on
        night     -- date of night YYYY-MM-DD
        run       -- date of run YYYY-MM.
        sskip     -- list of targets to avoid Simbad searches for. Added to as a given target fails to avoid
                     stressing the Simbad server.
        warn      -- If True, and the data is thought to be science (not bias, dark, flat, etc) to get message 
                     of targets with no match in the 'targets' (all of them if targets=None, so only sensible 
                     to set this if you have a targets object defined)

        At the end there are a whole stack of attributes. Not all will 
        be set, and if they are not they will be None. Some are specific
        to either ULTRACAM or ULTRASPEC. If calling this constructor 
        repeatedly, 'targets' should be updated with any found in Simbad as 
        indicated by the simbad flag.

        fname      -- name of xml file the run was constructed from
        telescope  -- name of the telescope
        night      -- date of night (YYYY-MM-DD)
        run        -- date of run YYYY-MM
        number     -- integer run number
        target     -- target name
        flag       -- data type flag
        instrument -- UCM or USP
        poweron    -- is this a power on
        poweroff   -- is this a power off
        x_bin      -- X binning factor
        y_bin      -- Y binning factor
        nwindow    -- number of windows
        mode       -- application type (2-windows etc)
        date       -- date at start of run
        utstart    -- UT at start of run
        utend      -- UT at end of run
        hastart    -- Hour angle at start of run
        haend      -- Hour angle at start of run
        amassmin   -- minimum airmass during run
        amassmax   -- maximum airmass during run
        haend      -- Hour angle at start of run
        expose     -- exposure time, seconds
        nframe     -- number of frames
        sample     -- sample time, seconds
        id         -- formal name of target, identified from 'targets'
        ra         -- RA
        dec        -- Dec
        speed      -- readout speed
        en_clr     -- clear enabled (ULTRASPEC)
        hv_gain    -- Avalanche gain setting (ULTRASPEC)
        output     -- which output (ULTRASPEC)
        xleft      -- leftmost X pixels of left-hand windows (ULTRACAM)
        xright     -- leftmost X pixels of right-hand= 6*[None]
        xstart     -- leftmost X pixels of windows (ULTRASPEC)
        ystart     -- bottom Y pixels, per pair for ULTRACAM, per window ULTRASPEC
        nx         -- number of pixels in X, per pair ULTRACAM, per window ULTRASPEC
        ny         -- number of pixels in Y, per pair ULTRACAM, per window ULTRASPEC
        comment    -- log file comment
        simbad     -- flag: True if target data came from Simbad.
        """
        self.fname     = xml
        self.telescope = telescope
        self.night     = night
        self.run       = run
        self.number    = int(xml[-7:-4])

        # read in the XML
        dom = parse(xml)

        # strip out the important info
        self.observatory, telescope  = Run._get_observatory_status(dom)
        self.instrument, self.application, param = Run._get_instrument_status(dom)
        user = Run._get_user(dom)
        del dom

        # Add in information from the log file
        self.comment = None
        self.target  = None
        self.filters = None
        if log is not None:
            if self.number in log.comment:
                self.comment = log.comment[self.number]
            else:
                self.comment = ''

            if self.number in log.filters:
                self.filters = log.filters[self.number]

            if log.format == 1:
                if self.number in log.target:
                    self.target = log.target[self.number]
                else:
                    sys.stderr.write('File = ' + xml + ', no corresponding entry found in log.\n')
                    self.target = None
            else:
                self.target = None

        self.poweron   = None
        self.poweroff  = None
        self.flag      = None
        self.x_bin     = None
        self.y_bin     = None
        self.nwindow   = None
        self.mode      = None
        self.date      = None
        self.utstart   = None
        self.utend     = None
        self.hastart   = None
        self.haend     = None
        self.amassmin  = None
        self.amassmax  = None
        self.haend     = None
        self.expose    = None
        self.nframe    = None
        self.sample    = None
        self.id        = None
        self.ra        = None
        self.dec       = None
        self.speed     = None
        self.en_clr    = None
        self.hv_gain   = None
        self.output    = None
        self.xleft     = 3*[None]
        self.xright    = 3*[None]
        self.xstart    = 2*[None]
        self.ystart    = 3*[None]
        self.nx        = 3*[None]
        self.ny        = 3*[None]
        self.observers = ''
        self.pid       = ''
        self.pi        = ''
        self.simbad    = False

        try:

            if self.instrument is not None:
                if self.instrument.find('Ultracam') > -1:
                    self.instrument = 'UCM'
                elif self.instrument.find('Ultraspec') > -1:
                    self.instrument = 'USP'
                else:
                    sys.stderr.write('File = ' + self.fname + ', failed to identify instrument.\n')

            if self.telescope is None and telescope is not None:
                if telescope.find('Very Large Telescope') > -1:
                    self.telescope = 'VLT'
                elif telescope.find('William Herschel Telescope') > -1:
                    self.telescope = 'WHT'
                elif telescope.find('New Technology Telescope') > -1:
                    self.telescope = 'NTT'
                else:
                    sys.stderr.write('File = ' + self.fname + ' failed to identify telescope\n')
            
            # identify power ons & offs
            self.poweron  = (self.application.find('poweron') > -1) or (self.application.find('pon_app') > -1) or (self.application.find('appl1_pon_cfg') > -1)
            self.poweroff = (self.application.find('poweroff') > -1) or (self.application.find('appl2_pof_cfg') > -1)

            if self.poweron:
                self.target = 'Power on'
            elif self.poweroff:
                self.target = 'Power off'
            else:
                self.x_bin = param['X_BIN_FAC'] if 'X_BIN_FAC' in param else param['X_BIN']
                self.y_bin = param['Y_BIN_FAC'] if 'Y_BIN_FAC' in param else param['Y_BIN']

                if user is not None:
                    if self.target is None and 'target' in user:
                        self.target = user['target'].strip()
                    if 'flags' in user:
                        self.flag = user['flags'].strip()
                    if 'filters' in user:
                        self.filters = user['filters'].strip()
                    if 'PI' in user:
                        self.pi = user['PI'].strip()
                    if 'Observers' in user:
                        self.observers = user['Observers'].strip()
                    if 'ID' in user:
                        self.pid = user['ID'].strip()

                # Try to ID target with one of known position
                if self.target is not None:

                    # Search through target entries.
                    for target, entry in targets.iteritems():
                        for ent in entry['match']:
                            if (ent[1] and self.target == ent[0]) or \
                                    (not ent[1] and ent[2].match(self.target)):
                                if self.id is None:
                                    self.id  = target
                                    self.ra  = subs.d2hms(entry['ra'],2,':',2)
                                    self.dec = subs.d2hms(entry['dec'],2,':',1,'yes')
                                else:
                                    sys.stderr.write('Multiple matches to target name = ' + self.target + '\n')

                    # SIMBAD lookup if no ID at this stage. To save time, the 'targets' dictionary should
                    # be updated between multiple invocations of run and then this lookup will not be repeated.
                    if self.id is None and self.is_science() and self.target not in sskip:
                        print 'Making SIMBAD query for',self.target
                        qsim = simbad.Query(self.target).query()
                        if len(qsim) == 0:
                            sys.stderr.write('Error: SIMBAD returned no matches to ' + self.target + '\n')
                        elif len(qsim) > 1:
                            sys.stderr.write('Error: SIMBAD returned ' + str(len(qsim)) + ' (>1) matches to ' + self.target + '\n')
                        else:
                            name = qsim[0]['Name']
                            pos  = qsim[0]['Position']
                            if name.startswith('V* '):
                                name = name[3:]
                            name = name.strip()
                            name = re.sub(Run.RESPC, ' ', name)
                            print 'Matched with',name,pos
                            ms = pos.find('-')
                            if ms > -1:
                                self.id  = name
                                self.ra  = subs.d2hms(subs.hms2d(pos[:ms].strip()),2,':',2)
                                self.dec = subs.d2hms(subs.hms2d(pos[ms:].strip()),2,':',1,sign=True)
                                self.simbad = True
                            else:
                                mp = pos.find('+')
                                if mp > -1:
                                    self.id  = name
                                    self.ra  = subs.d2hms(subs.hms2d(pos[:mp].strip()),2,':',2)
                                    self.dec = subs.d2hms(subs.hms2d(pos[mp:].strip()),2,':',1,sign=True)
                                    self.simbad = True
                                else:
                                    sys.stderr.write('Could not parse the SIMBAD position\n')

                # Translate applications into meaningful mode names
                app = self.application
                if app == 'ap8_250_driftscan' or app == 'ap8_driftscan' or app == 'ap_drift_bin2' or \
                        app == 'appl8_driftscan_cfg':
                    self.mode    = 'DRIFT'
                    self.nwindow = 2
                elif app == 'ap5_250_window1pair' or app == 'ap5_window1pair' or app == 'ap_win2_bin8' or \
                        app == 'ap_win2_bin2' or app == 'appl5_window1pair_cfg':
                    self.mode    = '1-PAIR'
                    self.nwindow = 2
                elif app == 'ap5b_250_window1pair' or app == 'appl5b_window1pair_cfg':
                    self.mode    = '1-PCLR'
                    self.nwindow = 2
                elif app == 'ap6_250_window2pair' or app == 'ap6_window2pair' or \
                        app == 'ap_win4_bin1' or app == 'ap_win4_bin8' or app == 'appl6_window2pair_cfg':
                    self.mode    = '2-PAIR'
                    self.nwindow = 4
                elif app == 'ap7_250_window3pair' or app == 'ap7_window3pair' or app == 'appl7_window3pair_cfg':
                    self.mode    = '3-PAIR'
                    self.nwindow = 6
                elif app == 'ap3_250_fullframe' or app == 'ap3_fullframe':
                    self.mode    = 'FFCLR'
                    self.nwindow = 2
                elif app == 'ap3_250_fullframe' or app == 'appl3_fullframe_cfg':
                    self.mode    = 'FFCLR'
                    self.nwindow = 2
                elif app == 'appl4_frameover_cfg':
                    self.mode    = 'FFOVER'
                    self.nwindow = 2
                elif app == 'ap9_250_fullframe_mindead' or app == 'ap9_fullframe_mindead' or app == 'appl9_fullframe_mindead_cfg':
                    self.mode    = 'FFNCLR'
                    self.nwindow = 2
                elif app == 'ccd201_winbin_con':
                    if int(param['X2_SIZE']) == 0:
                        self.mode    = '1-USPEC'
                        self.nwindow = 1
                    else:
                        self.mode    = '2-USPEC'
                        self.nwindow = 2
                elif app == 'ap4_frameover':
                    self.mode    = 'FFOVER'
                    self.nwindow = 2
                else:
                    sys.stderr.write('File = ' + self.fname + ' failed to identify application = ' + app + '\n')

                if times is not None:
                    self.date    = times.date[self.number] \
                        if self.number in times.date and times.date[self.number] != 'UNDEF' else None
                    self.utstart = subs.d2hms(subs.hms2d(times.utstart[self.number]),1,':') \
                        if self.number in times.utstart and times.utstart[self.number] != 'UNDEF' else None
                    self.utend   = subs.d2hms(subs.hms2d(times.utend[self.number]),1,':') \
                        if self.number in times.utend and times.utend[self.number] != 'UNDEF' else None
                    self.expose  = times.expose[self.number] \
                        if self.number in times.expose and times.expose[self.number] else None
                    self.expose  = self.expose if self.expose != 'UNDEF' else None
                    self.nframe  = times.nframe[self.number] \
                        if self.number in times.nframe and times.nframe[self.number] != 'UNDEF' else None
                    self.sample  = times.sample[self.number] \
                        if self.number in times.sample and times.sample[self.number] != 'UNDEF' else None
                    self.sample  = self.sample if self.sample != 'UNDEF' else None

                    # Try to compute start and end hour angles
                    if self.ra is not None and self.dec is not None and self.telescope is not None and \
                            self.utstart is not None and self.utend is not None and self.date is not None and self.date != 'UNDEF':
                        try:
                            (tel,obs,longitude,latitude,height) = subs.observatory(self.telescope)
                            (ra,dec,system) = subs.str2radec(self.ra + ' ' + self.dec)
                            uts = subs.hms2d(self.utstart)
                            (d,m,y) = self.date.split('/')
                            mjd = sla.cldj(int(y), int(m), int(d))
                            (ams,alt,az,ha,pa,delz) = sla.amass(mjd+uts/24.,longitude,latitude,height,ra,dec)                    
                            if ha > 12.:
                                self.hastart = ha - 24.
                            else:
                                self.hastart = ha
                            ute = subs.hms2d(self.utend)
                            if ute < uts:
                                mjd += 1
                            (ame,alt,az,ha,pa,delz) = sla.amass(mjd+ute/24.,longitude,latitude,height,ra,dec)                    
                            if ha < self.hastart:
                                self.haend = ha + 24.
                            else:
                                self.haend = ha
                            amax = ams if ams > ame else ame 
                            if self.hastart < 0 and self.haend > 0.:
                                amin = 1./cosd(latitude-dec)
                            else:
                                amin = ams if ams < ame else ame 
                            self.amassmax = '%5.2f' % (amax,)
                            self.amassmin = '%5.2f' % (amin,)
                        except subs.SubsError, err:
                            print err

                if self.instrument == 'UCM':

                    self.speed   = hex(int(param['GAIN_SPEED']))[2:] if 'GAIN_SPEED' in param else None

                    if self.mode == 'FFCLR' or self.mode == 'FFNCLR':
                        self.ystart[0] = '1'
                        self.xleft[0]  = '1'
                        self.xright[0] = '513'
                        self.nx[0]     = '512'
                        self.ny[0]     = '1024'
                    elif self.mode == 'FFOVER':
                        self.ystart[0] = '1'
                        self.xleft[0]  = '1'
                        self.xright[0] = '541'
                        self.nx[0]     = '540'
                        self.ny[0]     = '1032'
                    else:

                        self.ystart[0] = param['Y1_START'] if 'Y1_START' in param else None
                        self.xleft[0]  = param['X1L_START'] if 'X1L_START' in param else None
                        self.xright[0] = param['X1R_START'] if 'X1R_START' in param else None
                        self.nx[0]     = param['X1_SIZE'] if 'X1_SIZE' in param else None
                        self.ny[0]     = param['Y1_SIZE'] if 'Y1_SIZE' in param else None

                        if self.nwindow > 2:
                            self.ystart[1] = param['Y2_START'] if 'Y2_START' in param else None
                            self.xleft[1]  = param['X2L_START'] if 'X2L_START' in param else None
                            self.xright[1] = param['X2R_START'] if 'X2R_START' in param else None
                            self.nx[1]     = param['X2_SIZE'] if 'X2_SIZE' in param else None
                            self.ny[1]     = param['Y2_SIZE'] if 'Y2_SIZE' in param else None

                        if self.nwindow > 4:
                            self.ystart[2] = param['Y3_START'] if 'Y3_START' in param else None
                            self.xleft[2]  = param['X3L_START'] if 'X3L_START' in param else None
                            self.xright[2] = param['X3R_START'] if 'X3R_START' in param else None
                            self.nx[2]     = param['X3_SIZE'] if 'X3_SIZE' in param else None
                            self.ny[2]     = param['Y3_SIZE'] if 'Y3_SIZE' in param else None

                elif self.instrument == 'USP':

                    self.speed    = ('F' if param['SPEED'] == '0' else \
                                         ('M' if param['SPEED'] == '1' else 'S')) if 'SPEED' in param else None
                    self.en_clr   = ('Y' if param['EN_CLR'] == '1' else 'N') if 'EN_CLR' in param else None
                    self.hv_gain  = param['HV_GAIN'] if 'HV_GAIN' in param else None
                    self.output   = ('N' if param['OUTPUT'] == '0' else 'A') if 'OUTPUT' in param else None

                    def proc_xstart(output, xstart, nx):
                        """
                        Applies corrections to xstart and nx needed to get to the output independent
                        coordinates used by Usdriver (version 1.1.0 and after) and the pipeline (8.1.19 
                        and after).

                        Arguments:

                        output -- 'A' or 'N' for avalanche or normal
                        xstart -- string from XML of xstart in Derek coords
                        nx     -- string from XML

                        Returns (xstart,nx) integers 
                        """

                    self.xstart[0] = param['X1_START'] if 'X1_START' in param else None
                    self.ystart[0] = param['Y1_START'] if 'Y1_START' in param else None
                    self.nx[0]     = param['X1_SIZE'] if 'X1_SIZE' in param else None
                    self.ny[0]     = param['Y1_SIZE'] if 'Y1_SIZE' in param else None

                    if self.nwindow > 1:
                        self.xstart[1] = param['X2_START'] if 'X2_START' in param else None
                        self.ystart[1] = param['Y2_START'] if 'Y2_START' in param else None
                        self.nx[1]     = param['X2_SIZE'] if 'X2_SIZE' in param else None
                        self.ny[1]     = param['Y2_SIZE'] if 'Y2_SIZE' in param else None

            if warn and self.id is None and self.is_science() and self.target not in sskip:
                sys.stderr.write('File = ' + self.fname + ', no match for: ' + self.target + '\n')

        except Exception, err:
              sys.stderr.write('File = ' + self.fname + ', error initialising Run: ' + str(err) + '\n')

# for debugging
#            traceback.print_exc(file=sys.stdout)


    # Series of one-off helper routines for ULTRACAM/ULTRASPEC XML
    # make static methods of the class to avoid any use outside it

    @staticmethod
    def _get_observatory_status(dom):
        try:
            node        = dom.getElementsByTagName('observatory_status')[0]
            observatory = node.getElementsByTagName('name')[0].childNodes[0].data
            telescope   = node.getElementsByTagName('telescope')[0].childNodes[0].data
        except Exception, err:
            sys.stderr.write('Error reading observatory_status: ' + str(err) + '\n')
            observatory = None
            telescope   = None
        return (observatory, telescope)

    @staticmethod
    def _get_instrument_status(dom):
        try:
            node        = dom.getElementsByTagName('instrument_status')[0]
            instrument  = node.getElementsByTagName('name')[0].childNodes[0].data
            application = [nd for nd in node.getElementsByTagName('application_status') if nd.getAttribute('id') == 'SDSU Exec'][0].getAttribute('name')
            param = {}
            for nd in node.getElementsByTagName('parameter_status'):
                param[nd.getAttribute('name')] = nd.getAttribute('value')
        except Exception, err:
            sys.stderr.write('Error reading instrument_status: ' + str(err) + '\n')
            instrument  = None
            application = None
            param       = None
        return (instrument, application, param)

    @staticmethod
    def _get_user(dom):
        try:
            nlist = dom.getElementsByTagName('user')
            if len(nlist):
                user = {}
                node = nlist[0]
                for nd in node.childNodes:
                    if nd.nodeType == Node.ELEMENT_NODE and nd.hasChildNodes():
                        user[nd.tagName] = nd.childNodes[0].data
            else:
                user = None
        except Exception, err:
            print 'Error reading user data: ',err
            user = None
        return user

    def html_start(self):
        """
        Returns string for initial part of html file. This interacts with a css file
        defined early on.
        """
        
        inst = 'ULTRACAM' if self.instrument == 'UCM' else 'ULTRASPEC' if self.instrument == 'USPC' else None

        st = '<html>\n<head>\n<title> Night of '+ self.night + '</title>\n' + \
            '<link rel="stylesheet" type="text/css" href="../../ultracam_test.css" />\n' + \
            '</head>\n<body>' + \
            '<h1>' + 'Night of ' + self.night + '</h1>\n' + '<p>\n<table>\n' + \
            '<tr><td class="left">Telescope:</td>' + td(self.telescope,'left') + '</tr>\n' + \
            '<tr><td class="left">Instrument:</td>' + td(inst,'left') + '</tr>\n' + \
            '<tr><td class="left">Run ID:</td>' + td(self.run,'left') + '</tr>\n</table>\n' + \
            '<p>\n<table cellpadding=2>'
        st += '<tr>\n' + th('Run<br>no.') + th('Target','left') + th('Auto ID','left') + th('RA') + th('Dec') + \
            th('Date<br>Start of run') + th('UT<br>start') + th('UT<br>end') + th('Amss<br>min') + th('Amss<br>max') + \
            th('Dwell<br>sec.') + th('Sample<br>sec.') + th('Frame<br>no.') 

        if self.instrument == 'UCM':
            st += th('Filts')

        st += th('Mode') + th('Speed') + th('Bin')

        if self.instrument == 'UCM':
            st += th('Size1') + th('XLl') + th('XR1') + th('YS1') 
            st += th('Size2') + th('XL2') + th('XR2') + th('YS2') 
            st += th('Size3') + th('XL2') + th('XR3') + th('YS3') 
        elif self.instrument == 'USP':
            st += th('Clear')
            st += th("O'put")
            st += th('Gain')
            st += th('X1') + th('Y1') + th('NX1') + th('NY1')
            st += th('X2') + th('Y2') + th('NX2') + th('NY2')
        
        st += th('ID') + th('PI') + th('Observers')
        st += th('Run<br>no.') + th('Comment','left') + '</tr>\n'
        return st

    def html_table_row(self):
        """
        Returns a row of table data. Must be kept consistent with previous header routine
        """

        st  = '<tr>'
        st += td('%03d' % self.number)
        st += td(self.target,'left')
        st += td(self.id,'left')
        st += td(self.ra)
        st += td(self.dec)
        st += td(self.date)
        st += td(self.utstart)
        st += td(self.utend)
        st += td(self.amassmin)
        st += td(self.amassmax)
        st += td('%6.1f' % float(self.expose) if self.expose is not None else None, 'right')
        st += td('%7.3f' % float(self.sample) if self.sample is not None else None, 'right')
        st += td(self.nframe, 'right')
        if self.instrument == 'UCM':
            st += td(self.filters)
        st += td(self.mode)
        st += td(self.speed)
        st += td2(self.x_bin, self.y_bin)
        if self.instrument == 'USP':
            st += td(self.en_clr)
            st += td(self.output)
            st += td(self.hv_gain)
            st += td(self.xstart[0]) + td(self.ystart[0]) + td(self.nx[0]) + td(self.ny[0])
            st += td(self.xstart[1]) + td(self.ystart[1]) + td(self.nx[1]) + td(self.ny[1])
        elif self.instrument == 'UCM':
            st += td2(self.nx[0], self.ny[0]) + td(self.xleft[0]) + td(self.xright[0]) + td(self.ystart[0])
            st += td2(self.nx[1], self.ny[1]) + td(self.xleft[1]) + td(self.xright[1]) + td(self.ystart[1])
            st += td2(self.nx[2], self.ny[2]) + td(self.xleft[2]) + td(self.xright[2]) + td(self.ystart[2])

        st += td(self.pid) + td(self.pi) + tdnw(self.observers.replace(' ', ''))
        st += td('%03d' % self.number)
        st += td(self.comment,'left')
        st += '</tr>'
        return st

    def __eq__(self, other):
        """
        Defines ==. Equality between runs is defined in terms of their formats which have to match identically
        (same instrument, windows, readout speed etc.). Differences in target names, exposure times do
        not matter
        """
	if (isinstance(other, Run)):
            ok = self.instrument == other.instrument and self.mode == other.mode and \
                ((self.x_bin is None and other.x_bin is None) or  self.x_bin == other.x_bin) and \
                ((self.y_bin is None and other.y_bin is None) or self.y_bin == other.y_bin) and \
                ((self.nwindow is None and other.nwindow is None) or self.nwindow == other.nwindow) and \
                ((self.speed is None and other.speed is None) or self.speed == other.speed) and \
                ((self.en_clr is None and other.en_clr is None) or self.en_clr == other.en_clr) and \
                (not Run.FUSSY or (self.hv_gain is None and other.hv_gain is None) or self.hv_gain == other.hv_gain) and \
                ((self.output is None and other.output is None) or self.output == other.output)

            
            if ok:
                if self.instrument == 'UCM' and self.nwindow is not None:
                    for i in range(self.nwindow // 2):
                        if self.xleft[i] != other.xleft[i] or self.xright[i] != other.xright[i] or \
                                self.ystart[i] != other.ystart[i] or self.nx[i] != other.nx[i] or \
                                self.ny[i] != other.ny[i]:
                            ok  = False
                            break
                elif self.instrument == 'USP' and self.nwindow is not None:
                    for i in range(self.nwindow):
                        if self.xstart[i] != other.xstart[i] or \
                                self.ystart[i] != other.ystart[i] or self.nx[i] != other.nx[i] or \
                                self.ny[i] != other.ny[i]:
                            ok  = False
                            break
                    
            return ok

        else:
            return NotImplemented

    def __ne__(self, other):
        """
        Defines !=, the opposite of the equality operator
        """
        equal_result = self.__eq__(other)
        if (equal_result is not NotImplemented):
            return not equal_result
        return NotImplemented

    def __ge__(self, other):
        """
        Defines >=. One run is said to be 'greater than or equal to' another run if it can be re-formatted to 
        match that run. This is to allow the identification of calibration frames that match a data frame. 
        This means that the windows must cover the target frames windows and be compatible in terms of binning.
        This does not insist on identical readout modes or speeds (because one can use a flat field of differing
        readout speed for example). The routine insists that windows in 'other' are wholly enclosed by windows in
        'self', i.e. it does not search for windows enclosed by multiple windows in self. This probably never happens
        in practice in any case.
        """
	if (isinstance(other, Run)):

            # Basic checks
            if self.instrument != other.instrument or self.x_bin is None or other.x_bin is None or \
                    self.y_bin is None or other.y_bin is None:
                return False

            xbin,ybin,sxbin,sybin = int(other.x_bin),int(other.y_bin),int(self.x_bin),int(self.y_bin)

            if (sxbin != xbin and sxbin > 1) or (sybin != ybin and sybin > 1):
                return False

            # OK, we need to look at the window formats. 
            for (xleft,xright,ystart,nx,ny) in zip(other.xleft,other.xright,other.ystart,other.nx,other.ny):
                if xleft is None:
                    break
                xleft,xright,ystart,nx,ny = int(xleft),int(xright),int(ystart),int(nx),int(ny)
                ok = False
                for (sxleft,sxright,systart,snx,sny) in zip(self.xleft,self.xright,self.ystart,self.nx,self.ny):
                    if sxleft is None:
                        break
                    sxleft,sxright,systart,snx,sny = int(sxleft),int(sxright),int(systart),int(snx),int(sny)
                    # check vertical overlap and left and right-hand window overlap
                    if (systart <= ystart and systart+sybin*sny >= ystart+ybin*ny and (ystart-systart) % sybin == 0) and \
                            ((sxleft <= xleft and sxleft+sxbin*snx >= xleft+xbin*nx and (xleft-sxleft) % sxbin == 0) or \
                                 (sxright <= xright and sxright+sxbin*snx >= xright+xbin*nx and (xright-sxright) % sxbin == 0)):
                        ok = True
                        break
            return ok

        else:
            return NotImplemented

    def __str__(self):
        st = '%-25s %3s %1d %1s %1s %7s' % (self.target,self.instrument,self.nwindow,self.x_bin,self.y_bin,self.mode)
        if self.instrument == 'USP':
            st += ' %s %s %s %s' % (self.speed,self.en_clr,self.hv_gain,self.output)
            for i in range(2):
                st += ' %4s %4s %4s %4s' % (self.xstart[i],self.ystart[i],self.nx[i],self.ny[i])
        elif self.instrument == 'UCM':
            st += ' %s' % (self.speed,)
            for i in range(self.nwindow/2):
                st += ' %4s %3s %4s %3s %4s' % (self.ystart[i],self.xleft[i],self.xright[i],self.nx[i],self.ny[i])
            for i in range(3-self.nwindow/2):
                st += ' %4s %3s %4s %3s %4s' % (' ',' ',' ',' ',' ')
            if self.flag is not None: st += ' ' + self.flag
        return st

    def is_not_power_onoff(self):
        """
        Returns True if the run is not either a poweron or off. If in any
        doubt it returns False.
        """
        return (self.poweron is not None and self.poweroff is not None and not self.poweron and not self.poweroff)

    def is_power_onoff(self):
        """
        Returns True if the run is not either a poweron or off. If in any
        doubt it returns False.
        """
        return not self.is_not_power_onoff()

    def is_bias(self):
        """
        Returns True if the run is thought to be a bias
        """
        reb = re.compile('bias',re.I)
        return self.target is not None and reb.search(self.target)

    def is_flat(self):
        """
        Returns True if the run is thought to be a flat
        """
        return self.target is not None and \
            (self.target == "Tungsten" or self.target == "tungsten" or self.target == "Flat" or \
                 self.target == "Twilight" or self.target == "Twilight flat" or \
                 self.target == "Skyflat" or self.target == "Flats" or \
                 self.target == "Sky_flat" or self.target == "Sky flat" or \
                 self.target == "Test/skyflat" or self.target == "Sky flats" or \
                 self.target == "twilight" or self.target == "Sky Flat" or self.target == "Tungsten flat" or \
                 self.target == "Sky_flats" or self.target == "Sky Flats" or self.target == "Twilight Flats" or \
                 self.target == "flats" or self.target == "Dome flat")

    def is_dark(self):
        """
        Returns True if the run is thought to be a dark
        """
        reb = re.compile('dark',re.I)
        return self.target is not None and reb.search(self.target)

    def is_calib(self):
        """
        Returns True if the run is identified as a calibration frame (for data access) or is thought to be a dark, flat
        or bias.
        """
        reb = re.compile('calib',re.I)
        return self.pid is None or reb.match(self.pid) or self.is_flat() or self.is_dark() or self.is_bias()

    def is_science(self):
        """
        Returns True if the run is thought to be a science frame.
        """
        return not (self.target is None or \
                        (self.is_power_onoff() or self.is_bias() or self.is_flat() or self.is_dark() or \
                             self.target == "&nbsp;" or self.target == "Vik_test" or self.target == "Noise" or \
                             self.target == "Timing x-bin" or self.target == "Junk" or self.target == "Timing nx" or \
                             self.target == "Timing ny" or self.target == "Lin_test" or self.target == "NoiseBad" or \
                             self.target == "Focus star" or self.target == "Test" or self.target == "Rubbish" or \
                             self.target == "Saturn" or self.target == "Comet" or self.target == "JUNK" or \
                             self.target == "junk" or self.target == "?" or self.target == "BS" or \
                             self.target == "32K Test" or self.target == "Noise Tests" or \
                             self.target == "Arc - CuAr CuNe" or self.target == "Drift Mode Test" or \
                             self.target == "Timing Test" or self.target == "Timing x-left" or \
                             self.target == "Timing x-right" or self.target == "Bright_star" or \
                             self.target == "CCD Tests" or self.target == "Timing y-start" or \
                             self.target == "Timing test" or self.target == "32k Test" or self.target == "Zenith" or \
                             self.target == "Timing y-bin" or self.target == "Blurred Std" or \
                             self.target == "Light Level Test" or self.target == "PSF Tests" or \
                             self.target == "Read out noise test" or self.target == "32K test" or \
                             self.target == "Fringe Frame" or self.target == "null" or \
                             self.target == "Pluto speed test" or self.target == "ugr" or \
                             self.target == "PowerOn" or self.target == "Slide test"))

    def size(self):
        """
        Returns size in bytes per frame
        """
        if self.is_power_onoff():
            return 0

        if self.nframe is not None:
            nbytes = 24            
            for nx,ny in zip(self.nx,self.ny):
                if nx is not None and ny is not None:
                    nx,ny = int(nx),int(ny)
                    nbytes += 4*nx*ny
            return nbytes*int(self.nframe)
        else:
            return None
        
def td(data, type='cen'):
    """Handle html table data whether defined or not"""
    return '<td class="' + type + '">' + str(data) + '</td>' if data is not None else '<td class="undef">&nbsp;</td>'

def tdnw(data, type='cen'):
    """Handle html table data whether defined or not, disable line breaking"""
    return '<td class="' + type + '" nowrap>' + str(data) + '</td>' if data is not None else '<td class="undef">&nbsp;</td>'
                     
def td2(data1, data2, type='cen'):
    """Handle html table data whether defined or not, put 'x' in between values"""
    if data1 is None and data2 is None:
        return '<td class="undef">&nbsp;</td>'
    elif data1 is None:
        return '<td class="' + type + '">?x' + str(data2) + '</td>'
    elif data2 is None:
        return '<td class="' + type + '">' + str(data1) + 'x?</td>'
    else:
        return '<td class="' + type + '">' + str(data1) + 'x' + str(data2) + '</td>'

def th(data,type='cen'):
    """HTML table header entry"""
    return '<th class="' + type + '">' + str(data) + '</th>'

