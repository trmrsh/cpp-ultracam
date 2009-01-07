# python module for reading ULTRACAM xml files
# Defines a class for representing them. this should probably use
# some sort of xml module but doesn't
# T.R.Marsh 23/06/2007

import re

class Ultracam(dict):
    "Represents ULTRACAM formats"
    
    def __init__(self, run):
        """Initialisation by reading an xml file"""
        dict.__init__(self)
        self.read(run)

    def is_power_onoff(self):
        """Says whether an application is a power on or off """
        return self['application'] == 'ap1_250_poweron' or self['application'] == 'ap2_250_poweroff'

    def is_not_power_onoff(self):
        """Says whether an application is not a power on or off"""
        return not self.is_power_onoff()

    def is_bias(self):
        """Determine from target name whether this is a bias"""
        if self.is_power_onoff():
            return False
        btest = re.compile('\Wbias\W|^bias\W|^bias$|\Wbias$', re.IGNORECASE)
        return btest.search(self['target']) != None

    def is_calib(self):
        """Determines whether this is a calibration frame by seeing whether either the run ID
        or PI is some form of 'calib'"""
        if self.is_power_onoff():
            return False
        btest = re.compile('calib', re.IGNORECASE)
        return btest.search(self['ID']) != None or btest.search(self['PI']) != None

    def is_not_bias(self):
        """Determine from target name whether this is not a bias"""
        return not self.is_bias()
    
    def is_full_frame(self):
        """Says whether an application is a full frame one"""
        return self['application'] == 'ap3_250_fullframe' or self['application'] == 'ap9_250_fullframe_mindead'

    def is_overscan(self):
        """Says whether an application is an overscan one"""
        return self['application'] == 'ap4_250_frameover'

    def number_window_pairs(self):
        """Computes the number of window pairs."""

        if self['application'] == 'ap1_250_poweron':
            return 0
        elif self['application'] == 'ap2_250_poweroff':
            return 0
        elif self['application'] == 'ap3_250_fullframe':
            return 1
        elif self['application'] == 'ap4_250_frameover':
            return 1
        elif self['application'] == 'ap5_250_window1pair':
            return 1
        elif self['application'] == 'ap5b_250_window1pair':
            return 1
        elif self['application'] == 'ap6_250_window2pair':
            return 2
        elif self['application'] == 'ap7_250_window3pair':
            return 3
        elif self['application'] == 'ap8_250_driftscan':
            return 1
        elif self['application'] == 'ap9_250_fullframe_mindead':
            return 1
        else:
            raise Exception, 'Application ' + self['application'] + ' was not recognised.\nUpdate number_window_pairs.'


    def read(self, run):
        """Reads an ultracam format from an xml file.
        run is the name of an xml file, e.g. run043.xml.
        """
        # read the file
        self['run'] = run[0:run.rfind('.xml')]
        f = open(run)
        for line in f:
            
            if line.find('SDSU Exec') >= 0:
                n1 = line.index('name=') + 6
                n2 = line.index('"', n1)
                self['application'] = line[n1:n2]

            elif line.find('<detector_status') >= 0:
                n1 = line.index('name=') + 6
                n2 = line.index('"', n1)
                if line[n1:n2] == 'Ultraspec':
                    raise NotUltracamError, 'Run ' + run + ' is not an ULTRACAM file; could it be Ultraspec?'
                
            elif line.find('GAIN_SPEED') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['gain_speed'] = line[n1:n2]
                
            elif line.find('X_BIN_FAC') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x_bin_fac'] = line[n1:n2]
                
            elif line.find('Y_BIN_FAC') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['y_bin_fac'] = line[n1:n2]
                
                # first window pair
                
            elif line.find('X1L_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x1l_start'] = line[n1:n2]
                
            elif line.find('X1R_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x1r_start'] = line[n1:n2]
                
            elif line.find('X1_SIZE') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x1_size'] = line[n1:n2]
                
            elif line.find('Y1_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['y1_start'] = line[n1:n2]
                
            elif line.find('Y1_SIZE') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['y1_size'] = line[n1:n2]
                
                # second window pair
        
            elif line.find('X2L_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x2l_start'] = line[n1:n2]
                
            elif line.find('X2R_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x2r_start'] = line[n1:n2]
                
            elif line.find('X2_SIZE') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x2_size'] = line[n1:n2]
                
            elif line.find('Y2_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['y2_start'] = line[n1:n2]
                
            elif line.find('Y2_SIZE') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['y2_size'] = line[n1:n2]
                
                # third window pair
                
            elif line.find('X3L_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x3l_start'] = line[n1:n2]

            elif line.find('X3R_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x3r_start'] = line[n1:n2]
                
            elif line.find('X3_SIZE') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x3_size'] = line[n1:n2]
                
            elif line.find('Y3_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['y3_start'] = line[n1:n2]
                
            elif line.find('Y3_SIZE') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['y3_size'] = line[n1:n2]

            elif line.find('<target>') >= 0:
                n1 = line.index('target') + 7
                n2 = line.index('<', n1)
                self['target'] = line[n1:n2]
                
            elif line.find('<filters>') >= 0:
                n1 = line.index('filters') + 8
                n2 = line.index('<', n1)
                self['filters'] = line[n1:n2]

            elif line.find('<ID>') >= 0:
                n1 = line.index('ID') + 3
                n2 = line.index('<', n1)
                self['ID'] = line[n1:n2]

            elif line.find('<PI>') >= 0:
                n1 = line.index('PI') + 3
                n2 = line.index('<', n1)
                self['PI'] = line[n1:n2]
                

        # check that we have found what we expected to find
        if 'application' not in self:
            raise Exception, 'Failed to find application name in ' + run

            
        if self.is_full_frame():
            self['x1l_start'] = '1'
            self['x1r_start'] = '513'
            self['x1_size']   = '512'
            self['y1_start']  = '1'
            self['y1_size']   = '1024'
            
        elif self.is_overscan():
            self['x1l_start'] = '-27'
            self['x1r_start'] = '513'
            self['x1_size']   = '540'
            self['y1_start']  = '1'
            self['y1_size']   = '1032'
            
        if self.is_not_power_onoff():

            if 'x_bin_fac' not in self:
                raise Exception, 'Failed to find X_BIN_FAC in ' + run

            if 'y_bin_fac' not in self:
                raise Exception, 'Failed to find Y_BIN_FAC in ' + run

            
            if 'gain_speed' not in self:
                raise Exception, 'Failed to find GAIN_SPEED in ' + run
            
            if self.number_window_pairs() > 0:
        
                if 'x1l_start' not in self:
                    raise Exception, 'Failed to find X1L_START in ' + run
                
                if 'x1r_start' not in self:
                    raise Exception, 'Failed to find X1R_START in ' + run
                
                if 'x1_size' not in self:
                    raise Exception, 'Failed to find X1_SIZE in ' + run
                
                if 'y1_start' not in self:
                    raise Exception, 'Failed to find Y1_START in ' + run
                
                if 'y1_size' not in self:
                    raise Exception, 'Failed to find Y1_SIZE in ' + run
                
            if self.number_window_pairs() > 1:
                
                if 'x2l_start' not in self:
                    raise Exception, 'Failed to find X2L_START in ' + run
                
                if 'x2r_start' not in self:
                    raise Exception, 'Failed to find X2R_START in ' + run
                
                if 'x2_size' not in self:
                    raise Exception, 'Failed to find X2_SIZE in ' + run
                
                if 'y2_start' not in self:
                    raise Exception, 'Failed to find Y2_START in ' + run
                
                if 'y2_size' not in self:
                    raise Exception, 'Failed to find Y2_SIZE in ' + run
                
            if self.number_window_pairs() > 2:
                    
                if 'x3l_start' not in self:
                    raise Exception, 'Failed to find X3L_START in ' + run
                
                if 'x3r_start' not in self:
                    raise Exception, 'Failed to find X3R_START in ' + run
                
                if 'x3_size' not in self:
                    raise Exception, 'Failed to find X3_SIZE in ' + run
                
                if 'y3_start' not in self:
                    raise Exception, 'Failed to find Y3_START in ' + run
                    
                if 'y3_size' not in self:
                    raise Exception, 'Failed to find Y3_SIZE in ' + run

            if 'target' not in self:
                self['target'] = 'UNKNOWN'

            if 'filters' not in self:
                self['filters'] = 'UNKNOWN'

            if 'ID' not in self:
                self['ID'] = 'UNKNOWN'

            if 'PI' not in self:
                self['PI'] = 'UNKNOWN'

    def match_id(self, id):
        """Checks that the run ID matches that supplied as an argument. The id is
        taken to be a regular expression to allow for multiple IDs"""
        btest = re.compile(id, re.IGNORECASE)
        return 'ID' in self and btest.search(self['ID']) != None
            

    def to_string(self):
        """Returns format as a string"""
        if self.is_power_onoff():
            return 'Power On/Off'
        else:
            gain = str(hex(int(self['gain_speed'])))
            out = self['target'].ljust(20) + '  ' + self['filters'].ljust(11) + ' ' + self['x_bin_fac'] + 'x' + self['y_bin_fac'] + '  ' + gain[2:].upper()
                  
            
        if self.number_window_pairs() > 0:
            out += '  ' + self['x1_size'].rjust(4) + 'x' + self['y1_size'].ljust(4) + ' ' + self['x1l_start'].ljust(3) + ' ' + self['x1r_start'].ljust(4) + ' ' + self['y1_start'].ljust(4)
        if self.number_window_pairs() > 1:
            out += '  ' + self['x2_size'].rjust(4) + 'x' + self['y2_size'].ljust(4) + ' ' + self['x2l_start'].ljust(3) + ' ' + self['x2r_start'].ljust(4) + ' ' + self['y2_start'].ljust(4)
        if self.number_window_pairs() > 2:
            out += '  ' + self['x3_size'].rjust(4) + 'x' + self['y3_size'].ljust(4) + ' ' + self['x3l_start'].ljust(3) + ' ' + self['x3r_start'].ljust(4) + ' ' + self['y3_start'].ljust(4)
            
        if 'Comment' in self:
            out += ' ' + self['Comment']
        return out
    
    def to_html_table(self):
        """Returns format as a row in an html table"""
        td   = '<td>'
        nwtd = '<td nowrap="true">'
        ftd  = '<td class="format">'
        ctd  = '<td class="cen">'
        etd  = '</td>'
        
        if self.is_power_onoff():
            out = td + 'Power On/Off' + etd
        else:
            out = td + '<strong>' + self['target'].ljust(20) + '</strong>' + etd

        if 'Date' in self:
            out += ctd + self['Date'] + etd
        else:
            out += td + etd

        if 'UTstart' in self:
            out += ctd + self['UTstart'] + etd
        else:
            out += td + etd

        if 'UTend' in self:
            out += ctd + self['UTend'] + etd
        else:
            out += td + etd

        if 'exposure' in self:
            out += ctd + self['exposure'] + etd
        else:
            out += td + etd

        if 'sample' in self:
            out += ctd + self['sample'] + etd
        else:
            out += td + etd

        if 'nframe' in self:
            out += ctd + self['nframe'] + etd
        else:
            out += td + etd
            
        if self.is_power_onoff():
            out += (td + etd)*3
        else:
            gain = str(hex(int(self['gain_speed'])))
            out += ctd + self['filters'].ljust(11) + etd + ctd + self['x_bin_fac'] + 'x' + self['y_bin_fac'] + etd + ctd + gain[2:].upper() + etd          
            
        if self.number_window_pairs() > 0:
            out += ctd + self['x1_size'].rjust(4) + 'x' + self['y1_size'].ljust(4) + etd + td + self['x1l_start'].ljust(3) + etd + td + self['x1r_start'].ljust(4) + etd + td + self['y1_start'].ljust(4) + etd
        else:
            out += (td + etd)*4
            
        if self.number_window_pairs() > 1:
            out += ctd + self['x2_size'].rjust(4) + 'x' + self['y2_size'].ljust(4) + etd + td + self['x2l_start'].ljust(3) + etd + td + self['x2r_start'].ljust(4) + etd + td + self['y2_start'].ljust(4) + etd
        else:
            out += (td + etd)*4
        
        if self.number_window_pairs() > 2:
            out += ctd + self['x3_size'].rjust(4) + 'x' + self['y3_size'].ljust(4) + etd + td + self['x3l_start'].ljust(3) + etd + td + self['x3r_start'].ljust(4) + etd + td + self['y3_start'].ljust(4) + etd
        else:
            out += (td + etd)*4

        if 'ID' in self:
            out += ctd + self['ID'] + etd
        else:
            out += td + etd

        if 'PI' in self:
            out += ctd + self['PI'] + etd
        else:
            out += td + etd
        
        if 'Comment' in self:
            out += nwtd + self['Comment'] + etd
        else:
            out += td + etd

        return out

    def contains_window(self, xstart, ystart, nx, ny, xbin, ybin):
        """Checks that a given window is contained somewhere by the object. All arguments are strings
        xstart, ystart the lower-left pixel coordinates. nx, ny the unbinned dimensions, xbin, ybin the
        binning factors.
        """
        if self.number_window_pairs() > 0:
            if int(xstart) >= int(self['x1l_start']) and int(xstart)+int(nx) <= int(self['x1l_start'])+int(self['x1_size']) and \
                   int(ystart) >= int(self['y1_start']) and int(ystart)+int(ny) <= int(self['y1_start'])+int(self['y1_size']) and \
                   int(xbin) % int(self['x_bin_fac']) == 0 and int(ybin) % int(self['y_bin_fac']) == 0 and \
                   (int(xstart) - int(self['x1l_start'])) %  int(xbin) == 0 and (int(ystart) - int(self['y1_start'])) %  int(ybin) == 0:
                return True

            if int(xstart) >= int(self['x1r_start']) and int(xstart)+int(nx) <= int(self['x1r_start'])+int(self['x1_size']) and \
                   int(ystart) >= int(self['y1_start']) and int(ystart)+int(ny) <= int(self['y1_start'])+int(self['y1_size']) and \
                   int(xbin) % int(self['x_bin_fac']) == 0 and int(ybin) % int(self['y_bin_fac']) == 0 and \
                   (int(xstart) - int(self['x1r_start'])) %  int(xbin) == 0 and (int(ystart) - int(self['y1_start'])) %  int(ybin) == 0:
                return True

        if self.number_window_pairs() > 1:
            if int(xstart) >= int(self['x2l_start']) and int(xstart)+int(nx) <= int(self['x2l_start'])+int(self['x2_size']) and \
                   int(ystart) >= int(self['y2_start']) and int(ystart)+int(ny) <= int(self['y2_start'])+int(self['y2_size']) and \
                   int(xbin) % int(self['x_bin_fac']) == 0 and int(ybin) % int(self['y_bin_fac']) == 0 and \
                   (int(xstart) - int(self['x2l_start'])) %  int(xbin) == 0 and (int(ystart) - int(self['y2_start'])) %  int(ybin) == 0:
                return True

            if int(xstart) >= int(self['x2r_start']) and int(xstart)+int(nx) <= int(self['x2r_start'])+int(self['x2_size']) and \
                   int(ystart) >= int(self['y2_start']) and int(ystart)+int(ny) <= int(self['y2_start'])+int(self['y2_size']) and \
                   int(xbin) % int(self['x_bin_fac']) == 0 and int(ybin) % int(self['y_bin_fac']) == 0 and \
                   (int(xstart) - int(self['x2r_start'])) %  int(xbin) == 0 and (int(ystart) - int(self['y2_start'])) %  int(ybin) == 0:
                return True

        if self.number_window_pairs() > 2:
            if int(xstart) >= int(self['x3l_start']) and int(xstart)+int(nx) <= int(self['x3l_start'])+int(self['x3_size']) and \
                   int(ystart) >= int(self['y3_start']) and int(ystart)+int(ny) <= int(self['y3_start'])+int(self['y3_size']) and \
                   int(xbin) % int(self['x_bin_fac']) == 0 and int(ybin) % int(self['y_bin_fac']) == 0 and \
                   (int(xstart) - int(self['x3l_start'])) %  int(xbin) == 0 and (int(ystart) - int(self['y3_start'])) %  int(ybin) == 0:
                return True

            if int(xstart) >= int(self['x3r_start']) and int(xstart)+int(nx) <= int(self['x3r_start'])+int(self['x3_size']) and \
                   int(ystart) >= int(self['y3_start']) and int(ystart)+int(ny) <= int(self['y3_start'])+int(self['y3_size']) and \
                   int(xbin) % int(self['x_bin_fac']) == 0 and int(ybin) % int(self['y_bin_fac']) == 0 and \
                   (int(xstart) - int(self['x3r_start'])) %  int(xbin) == 0 and (int(ystart) - int(self['y3_start'])) %  int(ybin) == 0:
                return True

        return False

            
    def is_subset_of(self, ucam):
        """Checks that the Ultracam is a subset of another, ucam, in the sense that it would be possible
        to get from the format of ucam to that of self and that they match in readout speed unless ucam is
        any calibration frame other than a bias in which the read speeds need not match. This is because a
        flat can still be used as a flat even if its read speed differed from the frame in question.
        """
            
        if self.is_power_onoff() or ucam.is_power_onoff():
            return False
    
        if (ucam.is_bias() or not ucam.is_calib()) and self['gain_speed'] != ucam['gain_speed']:
            return False

        if int(self['x_bin_fac']) % int(ucam['x_bin_fac']) != 0 or int(self['y_bin_fac']) % int(ucam['y_bin_fac']) != 0:
            return False

        if self.number_window_pairs() > 0:

            if not ucam.contains_window(self['x1l_start'], self['y1_start'], self['x1_size'], self['y1_size'], self['x_bin_fac'], self['y_bin_fac']):
                return False
            if not ucam.contains_window(self['x1r_start'], self['y1_start'], self['x1_size'], self['y1_size'], self['x_bin_fac'], self['y_bin_fac']):
                return False

        if self.number_window_pairs() > 1:

            if not ucam.contains_window(self['x2l_start'], self['y2_start'], self['x2_size'], self['y2_size'], self['x_bin_fac'], self['y_bin_fac']):
                return False
            if not ucam.contains_window(self['x2r_start'], self['y2_start'], self['x2_size'], self['y2_size'], self['x_bin_fac'], self['y_bin_fac']):
                return False

        if self.number_window_pairs() > 2:

            if not ucam.contains_window(self['x3l_start'], self['y3_start'], self['x3_size'], self['y3_size'], self['x_bin_fac'], self['y_bin_fac']):
                return False
            if not ucam.contains_window(self['x3r_start'], self['y3_start'], self['x3_size'], self['y3_size'], self['x_bin_fac'], self['y_bin_fac']):
                return False
        
        return True

# Functions below are not class members

def match(ucam1, ucam2):
    """Compares two Ultracams returning true if they match.
    ucam1 and ucam2 are the names of two Ultracam objects to be compared.
    'Matching' means having compatible window formats. This means the same readout speed,
    binning factors, numbers of windows and window dimensions.
    """
    
    if ucam1.is_power_onoff() and ucam2.is_power_onoff():
        return True
    
    if ucam1.number_window_pairs() != ucam2.number_window_pairs():
        return False
    
    if ucam1['gain_speed'] != ucam2['gain_speed'] or \
       ucam1['x_bin_fac'] != ucam2['x_bin_fac'] or \
       ucam1['y_bin_fac'] != ucam2['y_bin_fac']:
        return False
    
    if ucam1.number_window_pairs() > 0:
        
        if ucam1['x1l_start'] != ucam2['x1l_start'] or \
               ucam1['x1r_start'] != ucam2['x1r_start'] or \
               ucam1['x1_size'] != ucam2['x1_size'] or \
               ucam1['y1_start'] != ucam2['y1_start'] or \
               ucam1['y1_size'] != ucam2['y1_size']:
            return False
        
    if ucam1.number_window_pairs() > 1:

        if ucam1['x2l_start'] != ucam2['x2l_start'] or \
               ucam1['x2r_start'] != ucam2['x2r_start'] or \
               ucam1['x2_size'] != ucam2['x2_size'] or \
               ucam1['y2_start'] != ucam2['y2_start'] or \
               ucam1['y2_size'] != ucam2['y2_size']:
            return False
        
    if ucam1.number_window_pairs() > 2:

        if ucam1['x3l_start'] != ucam2['x3l_start'] or \
               ucam1['x3r_start'] != ucam2['x3r_start'] or \
               ucam1['x3_size'] != ucam2['x3_size'] or \
               ucam1['y3_start'] != ucam2['y3_start'] or \
               ucam1['y3_size'] != ucam2['y3_size']:
            return False
        
    return True


def first_header():
    """Returns first header line corresponding to the to_html_table function above"""
    return """
<th>Target
<th>Date
<th colspan="2">UT
<th>Exp
<th>Cycle
<th>No. of
<th>Filters
<th>XxY
<th>Speed
<th>NX1xNY1
<th>X1L
<th>X1R
<th>YS
<th>NX2xNY2
<th>X2L
<th>X2R
<th>YS2
<th>NX3xNY3
<th>X3L
<th>X3R
<th>Y32
<th>ID
<th>PI
<th align="left">Comment
"""


def second_header():
    """Returns second header line corresponding to the to_html_table function above"""
    return """
<th>
<th>start
<th>start
<th>end
<th>(secs)
<th>time
<th>frames
<th>
<th>bin
<th>
<th>
<th>
<th>
<th>
<th>
<th>
<th>
<th>
<th>
<th>
<th>
<th>
<th>
<th>
<th>
"""

class NotUltracamError(Exception):
    """For indicating that we are not on an Ultracam"""
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)
    

