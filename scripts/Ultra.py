#!/usr/bin/env python

"""
Support routines for Python analysis of ULtracam & Ultraspec
files. Mainly for database wotk.
"""

import os
import sys
from xml.dom import Node
from xml.dom.minidom import parse, parseString
import re
import trm.subs as subs
import traceback

class Log(object):
    """
    Class to read and store log file data. These come in two formats:

    1) Old style: run, target name, comment
    2) New style: run, comment (target names are in the xml files)

    The class just stores the data in a couple of dictionaries
    'comment' and 'target'; 'format' is an integer specifying 
    the format as above. 'target' is blank in the case of format == 2.
    """

    def __init__(self, fname):
        """
        Constructs a new Log given a file name. Makes empty
        dictionaries if none found and reports an error
        """
        self.format  = 2
        self.comment = {}
        self.target  = {}
        try:
            f  = open(fname)
            for line in f:
                if line.startswith('run'):
                    num = int(line[3:6])
                    self.comment[num] = line[6:].strip()
        except Exception, err:
            print 'Night log problem: ',err

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
            print 'Timing data problem: ',err,'file =',fname

class Targets(object):
    """
    Class to read and store the target positions and regular expressions

    Attributes (all dictionaries keyed on run number):

    position  -- dictionary of positions keyed by target name
    translate -- dictionary of target names keyed by regular expressions to match names from logs
    """

    def __init__(self, fname):
        """
        Constructs a new Targets file. Makes empty
        dictionaries if none found and reports an error
        """
        self.position  = {}
        self.translate = {}

        try:
            f  = open(fname)
            for line in f:
                if not line.startswith('#') and line[:1].isalnum():
                    if line.startswith('Translate:'):
                        try:
                            (d,target,rexpr) = line.split()
                        except:                            
                            (d,target,rexpr,flag) = line.split()
                        self.translate[re.compile(rexpr)] = target
                    else:
                        (target,rah,ram,ras,decd,decm,decs) = line.strip().split()
                        ra  = int(rah) + int(ram)/60. + float(ras)/3600.
                        if decd[:1] == '-':
                            decfac = -1.
                        elif decd[:1] == '+':
                            decfac = +1.
                        else:
                            print 'Target = ' + target + ' has no sign on its declination and will be skipped.'
                        dec = int(decd[1:]) + int(decm)/60. + float(decs)/3600.
                        self.position[target] = (ra,decfac*dec)
        except Exception, err:
            print 'Target data problem: ',err
            if line is not None: print 'Line: ' + line

class Run(object):
    """
    This is the main class for reading and storing data on a per run basis,
    including collating data from times and night log files. The aim is that
    it contains ALL the information for a given run.

    Static data Run.FUSSY controls the meaning of the equality operator '=='.
    If FUSSY = True then absolutely everyformat parameter must match. FUSSY = False
    then it will ignore differences in the avalanche gain of ULTRASPEC.
    """

    FUSSY = True

    def __init__(self, xml, log=None, times=None, targets=None, telescope=None, night=None, run=None):
        """
        xml       -- xml file name with format run###.xml
        log       -- previously read night log
        times     -- previously read timing data
        targets   -- previously read target position data
        telescope -- telescope; names from XML cannot be relied on
        night     -- date of night YYYY-MM-DD
        run       -- date of run YYYY-MM.

        At the end there are a whole stack of attributes. Not all will 
        be set, and if they are not they will be None. Some are specific
        to either ULTRACAM or ULTRASPEC

        fname      -- name of xml file the run was constructed from
        telescope  -- name of the telescope
        night      -- date of night (YYYY-MM-DD)
        run        -- date of run YYYY-MM
        number     -- integer run number
        target     -- target name
        instrument -- ULTRACAM or ULTRASPEC
        poweron    -- is this a power on
        poweroff   -- is this a power off
        x_bin      -- X binning factor
        y_bin      -- Y binnin factor
        nwindow    -- number of windows
        mode       -- application type (2-windows etc)
        date       -- date at start of run
        utstart    -- UT at start of run
        utend      -- UT at end of run
        expose     -- exposure time, seconds
        nframe     -- number of frames
        sample     -- sample time, seconds
        id         -- formal name of target
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
        if log is not None:
            if self.number in log.comment:
                self.comment = log.comment[self.number]
            else:
                #            print 'No comment found in log corresponding to ' + xml
                self.comment = ''

            if log.format == 1:
                if self.number in log.target:
                    self.target = log.target[self.number]
                else:
                    print 'No target found in log corresponding to ' + xml
                    self.target = None
            else:
                self.target = None

        self.poweron   = None
        self.poweroff  = None
        self.x_bin     = None
        self.y_bin     = None
        self.nwindow   = None
        self.mode      = None
        self.date      = None
        self.utstart   = None
        self.utend     = None
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

        try:

            if self.instrument is not None:
                if self.instrument.find('Ultracam') > -1:
                    self.instrument = 'ULTRACAM'
                elif self.instrument.find('Ultraspec') > -1:
                    self.instrument = 'ULTRASPEC'
                else:
                    print 'File =',self.fname,'failed to identify instrument'

            if self.telescope is None and telescope is not None:
                if telescope.find('Very Large Telescope') > -1:
                    self.telescope = 'VLT'
                elif telescope.find('William Herschel Telescope') > -1:
                    self.telescope = 'WHT'
                elif telescope.find('New Technology Telescope') > -1:
                    self.telescope = 'NTT'
                else:
                    print 'File =',self.fname,'failed to identify telescope'
            
            # identify power ons & offs
            self.poweron  = (self.application.find('poweron') > -1) or (self.application.find('pon_app') > -1)
            self.poweroff = (self.application.find('poweroff') > -1)

            if self.poweron:
                self.target = 'Power on'
            elif self.poweroff:
                self.target = 'Power off'
            else:
                self.x_bin = param['X_BIN_FAC'] if 'X_BIN_FAC' in param else param['X_BIN']
                self.y_bin = param['Y_BIN_FAC'] if 'Y_BIN_FAC' in param else param['Y_BIN']

                if user is not None:
                    if self.target is None and 'target' in user:
                        self.target = user['target']

                # Try to ID target with one of known position
                if self.target is not None and targets is not None:
                    for reg, target in targets.translate.iteritems():
                        if reg.match(self.target):
                            if self.id is None:
                                self.id  = target.replace('~',' ')
                                self.ra  = subs.d2hms(targets.position[target][0],2,':',2)
                                self.dec = subs.d2hms(targets.position[target][1],2,':',1,'yes')
                            else:
                                print 'Multiple match to target name = ' + self.target


                # Translate applications into meaningful mode names
                if self.application == 'ap8_250_driftscan' or self.application == 'ap8_driftscan' or self.application == 'ap_drift_bin2':
                    self.mode    = 'DRIFT'
                    self.nwindow = 2
                elif self.application == 'ap5_250_window1pair' or self.application == 'ap5b_250_window1pair' or \
                        self.application == 'ap5_window1pair' or self.application == 'ap_win2_bin8' or \
                        self.application == 'ap_win2_bin2':
                    self.mode    = '1-PAIR'
                    self.nwindow = 2
                elif self.application == 'ap6_250_window2pair' or self.application == 'ap6_window2pair' or \
                        self.application == 'ap_win4_bin1' or self.application == 'ap_win4_bin8':
                    self.mode    = '2-PAIR'
                    self.nwindow = 4
                elif self.application == 'ap7_250_window3pair' or self.application == 'ap7_window3pair':
                    self.mode    = '3-PAIR'
                    self.nwindow = 6
                elif self.application == 'ap3_250_fullframe' or self.application == 'ap3_fullframe':
                    self.mode    = 'FFCLR'
                    self.nwindow = 2
                elif self.application == 'ap3_250_fullframe':
                    self.mode    = 'FFCLR'
                    self.nwindow = 2
                elif self.application == 'ap9_250_fullframe_mindead' or self.application == 'ap9_fullframe_mindead':
                    self.mode    = 'FFNCLR'
                    self.nwindow = 2
                elif self.application == 'ccd201_winbin_con':
                    if int(param['X2_SIZE']) == 0:
                        self.mode    = '1-USPEC'
                        self.nwindow = 1
                    else:
                        self.mode    = '2-USPEC'
                        self.nwindow = 2
                elif self.application == 'ap4_frameover':
                    self.mode    = 'FFOVER'
                    self.nwindow = 2
                else:
                    print 'File =',self.fname,'failed to identify application = ',self.application

                if times is not None:
                    self.date    = times.date[self.number] if self.number in times.date else None
                    self.utstart = times.utstart[self.number] if self.number in times.utstart else None
                    self.utend   = times.utend[self.number] if self.number in times.utend else None
                    self.expose  = times.expose[self.number] if self.number in times.expose else None
                    self.expose  = self.expose if self.expose != 'UNDEF' else None
                    self.nframe  = times.nframe[self.number] if self.number in times.nframe else None
                    self.sample  = times.sample[self.number] if self.number in times.sample else None
                    self.sample  = self.sample if self.sample != 'UNDEF' else None

                if self.instrument == 'ULTRACAM':

                    self.speed   = param['GAIN_SPEED'] if 'GAIN_SPEED' in param else None

                    self.xleft[0]  = param['X1L_START'] if 'X1L_START' in param else None
                    self.xright[0] = param['X1R_START'] if 'X1R_START' in param else None
                    self.ystart[0] = param['Y1_START'] if 'Y1_START' in param else None
                    self.nx[0]     = param['X1_SIZE'] if 'X1_SIZE' in param else None
                    self.ny[0]     = param['Y1_SIZE'] if 'Y1_SIZE' in param else None

                    if self.nwindow > 2:
                        self.xleft[1]  = param['X2L_START'] if 'X2L_START' in param else None
                        self.xright[1] = param['X2R_START'] if 'X2R_START' in param else None
                        self.ystart[1] = param['Y2_START'] if 'Y2_START' in param else None
                        self.nx[1]     = param['X2_SIZE'] if 'X2_SIZE' in param else None
                        self.ny[1]     = param['Y2_SIZE'] if 'Y2_SIZE' in param else None

                    if self.nwindow > 4:
                        self.xleft[1]  = param['X3L_START'] if 'X3L_START' in param else None
                        self.xright[1] = param['X3R_START'] if 'X3R_START' in param else None
                        self.ystart[1] = param['Y3_START'] if 'Y3_START' in param else None
                        self.nx[1]     = param['X3_SIZE'] if 'X3_SIZE' in param else None
                        self.ny[1]     = param['Y3_SIZE'] if 'Y3_SIZE' in param else None

                elif self.instrument == 'ULTRASPEC':

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

        except Exception, err:
            print 'File =',self.fname,'; error initialising Run: ',err

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
            print 'Error reading observatory_status: ',err
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
            print 'Error reading instrument_status: ',err
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

        st = '<html>\n<head>\n<title> Night of '+ self.night + '</title>\n' + \
            '<link rel="stylesheet" type="text/css" href="../../ultracam_test.css" />\n' + \
            '</head>\n<body>' + \
            '<h1>' + 'Night of ' + self.night + '</h1>\n' + '<p>\n<table>\n' + \
            '<tr><td class="left">Telescope:</td>' + td(self.telescope,'left') + '</tr>\n' + \
            '<tr><td class="left">Instrument:</td>' + td(self.instrument,'left') + '</tr>\n' + \
            '<tr><td class="left">Run ID:</td>' + td(self.run,'left') + '</tr>\n</table>\n' + \
            '<p>\n<table>'
        st += '<tr>\n' + th('Run<br>no.') + th('Target','left') + th('Auto ID','left') + th('RA') + th('Dec') + \
            th('Date<br>Start of night') + th('UT<br>start') + th('UT<br>end') + th('Dwell<br>sec.') + \
            th('Sample<br>sec.') + th('Frame<br>no.') + th('Mode')

        if self.instrument == 'ULTRACAM':
            st += th('Gain<br>speed')
            st += th('X1') + th('Y1') + th('NX1') + th('NY1')
            st += th('X2') + th('Y2') + th('NX2') + th('NY2')
            st += th('X2') + th('Y2') + th('NX2') + th('NY2')
        elif self.instrument == 'ULTRASPEC':
            st += th('Speed')
            st += th('Clear')
            st += th("O'put")
            st += th('Gain')
            st += th('X1') + th('Y1') + th('NX1') + th('NY1')
            st += th('X2') + th('Y2') + th('NX2') + th('NY2')

        st += th('Nwin')
        st += th('Run<br>no.') + th('Comment','left') + '</tr>\n'
        return st

    def html_table_row(self):
        """
        Returns a row of table data
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
        st += td('%6.1f' % float(self.expose) if self.expose is not None else None, 'right')
        st += td('%7.3f' % float(self.sample) if self.sample is not None else None, 'right')
        st += td(self.nframe, 'right')
        st += td(self.mode)
        if self.instrument == 'ULTRACAM':
            if self.speed is not None:
                st += td(hex(int(self.speed)))
            else:
                st += td(self.speed)
        elif self.instrument == 'ULTRASPEC':
            st += td(self.speed)
            st += td(self.en_clr)
            st += td(self.output)
            st += td(self.hv_gain)
            st += td(self.xstart[0]) + td(self.ystart[0]) + td(self.nx[0]) + td(self.ny[0])
            st += td(self.xstart[1]) + td(self.ystart[1]) + td(self.nx[1]) + td(self.ny[1])

        st += td(self.nwindow)
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
                ((self.en_clr is None and other.en_clr) or self.en_clr == other.en_clr) and \
                (not Run.FUSSY or (self.hv_gain is None and other.hv_gain is None) or self.hv_gain == other.hv_gain) and \
                ((self.output is None and other.output is None) or self.output == other.output)

            if ok:
                if self.instrument == 'ULTRACAM' and self.nwindow is not None:
                    for i in range(self.nwindow // 2):
                        if self.xleft[i] != other.xleft[i] or self.xright[i] != other.xright[i] or \
                                self.ystart[i] != other.ystart[i] or self.nx[i] != other.nx[i] or \
                                self.ny[i] != other.ny[i]:
                            ok  = False
                            break
                elif self.instrument == 'ULTRASPEC' and self.nwindow is not None:
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

    def __str__(self):
        st = '%-20s %-9s %1d %1s %1s %7s' % (self.target,self.instrument,self.nwindow,self.x_bin,self.y_bin,self.mode)
        if self.instrument == 'ULTRASPEC':
            st += ' %s %s %s %s' % (self.speed,self.en_clr,self.hv_gain,self.output)
            for i in range(2):
                st += ' %4s %4s %4s %4s' % (self.xstart[i],self.ystart[i],self.nx[i],self.ny[i])
        return st

    def is_not_power_onoff(self):
        """
        Returns True if the run is not either a poweron or off. If in any
        doubt it returns False.
        """
        return (self.poweron is not None and self.poweroff is not None and not self.poweron and not self.poweroff)


def td(data, type='cen'):
    """Handle html table data whether defined or not"""
    return '<td class="' + type + '">' + str(data) + '</td>' if data is not None else '<td class="undef">&nbsp;</td>'

def th(data,type='cen'):
    """HTML table header entry"""
    return '<th class="' + type + '">' + str(data) + '</th>'

# Finally, lets do something.

if __name__ == "__main__":

    # The main program

    # Basic aim is to read as many xml and data files as possible. Errors
    # are converted to warnings if possible and ideally the user should fix up the files
    # until no such warnings appear. html files are made on a night-by-night
    # basis.

    # First read target data
    targets = Targets('TARGETS')

    # Create a list directories of runs to search through
    rdirs = [x for x in os.listdir(os.curdir) if os.path.isdir(x) and x.startswith('20')]
    for rdir in rdirs:

        # Try to find the telescope.
        try:
            f = open(os.path.join(rdir, 'telescope'))
            telescope = f.readline().rstrip()
            f.close()
            print 'Run directory =',rdir,', telescope =',telescope
        except Exception, err:
            telescope = None
            print 'Run directory =',rdir,',',err


        # Now the night-by-night directories
        ndirs = [x for x in os.listdir(rdir) if os.path.isdir(os.path.join(rdir,x)) and x.startswith('20')]

        # Write a guide
        fg = open(os.path.join(rdir, 'guide.htm'), 'w')
        fg.write("""
<html>
<head>
<link rel="stylesheet" type="text/css" href="../ultracam_test.css" />
</head>
<body>
""")

        for rd in rdirs:
            (year,month) = rd.split('-')
            fg.write('<p>\n<a href="../' + rd + '/guide.htm">' + subs.int2month(int(month)) + ' ' + year + '</a><br>\n')
            if rd == rdir:
                fg.write('<ul>\n')
                for ndir in ndirs:
                    fg.write('<li> <a href="' + ndir + '/' + ndir + '.htm' + '" target="dynamic">' + ndir + '</a></li>\n')
                fg.write('</ul>\n')

        fg.write('</body>\n</html>\n')
        fg.close()

        runs = []
        # now to the night-by-night files
        for ndir in ndirs:

            npath = os.path.join(rdir, ndir)
            
            # Read night log (does not matter if none exists, although a warning will be printed)
            nlog = Log(os.path.join(npath, ndir + '.dat'))

            # Read timing data (does not matter if none exists, although a warning will be printed)
            times = Times(os.path.join(npath, ndir + '.times'))

            # Start off html log file for the night
            fh = open(os.path.join(npath, ndir + '.htm'), 'w')

            # Read XML files for this night
            dpath = os.path.join(npath, 'data')
            xmls = [os.path.join(dpath,x) for x in os.listdir(dpath) if x.startswith('run') and x.endswith('.xml')]
            xmls.sort()
            first = True
            expose = 0.
            for xml in xmls:
                try:
                    run = Run(xml, nlog, times, targets, telescope, ndir, rdir)
                    if first:
                        fh.write('\n' + run.html_start() + '\n')
                        first = False
                    fh.write('\n' + run.html_table_row() + '\n')            
                    expose += float(run.expose) if run.expose is not None and run.expose != ' ' else 0.
                    runs.append(run)
                except Exception, err:
                    print 'XML error: ',err,'in',xml

            # Shut down html file
            fh.write('</table>\n\n' + '<p>Total exposure time = ' + str(int(100.*expose/3600.)/100.) + ' hours\n')
            fh.write('</body>\n</html>')
            fh.close()