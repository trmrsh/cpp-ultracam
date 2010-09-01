# python module for reading ULTRACAM xml files
# Defines a class for representing them. this should probably use
# some sort of xml module but doesn't
# T.R.Marsh 23/06/2007

import re

class Ultraspec(dict):
    "Represents of ULTRASPEC formats"
    
    def __init__(self, run):
        """Initialisation by reading an xml file"""
        dict.__init__(self)
        self.read(run)

    def is_power_onoff(self):
        """Says whether an application is a power on or off """
        return self['application'] == 'ccd201_pon_app'

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

    def number_windows(self):
        """Computes the number of windows."""
        if self.is_power_onoff():
            return 0
        elif int(self['x1_size']) > 0 and int(self['x2_size']) > 0:
            return 2
        elif int(self['x1_size']) > 0:
            return 1
        else:
            raise Exception, 'Could not determine number of windows'

    def read(self, run):
        """Reads an ultraspec format from an xml file.
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
                if line[n1:n2] != 'Ultraspec':
                    raise Exception, 'Run ' + run + ' is not an Ultraspec file.'
                
            elif line.find('SPEED') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['speed'] = line[n1:n2]
                
            elif line.find('X_BIN') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x_bin'] = line[n1:n2]
                
            elif line.find('Y_BIN') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['y_bin'] = line[n1:n2]
                
                # first window 
                
            elif line.find('X1_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x1_start'] = line[n1:n2]
                
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
                
                # second window
                
            elif line.find('X2_START') >= 0:
                n1 = line.index('value=') + 7
                n2 = line.index('"', n1)
                self['x2_start'] = line[n1:n2]
                
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
                
            elif line.find('<target>') >= 0:
                n1 = line.index('target') + 7
                n2 = line.index('<', n1)
                self['target'] = line[n1:n2]

            elif line.find('<grating>') >= 0:
                n1 = line.index('grating') + 8
                n2 = line.index('<', n1)
                self['grating'] = line[n1:n2]

            elif line.find('<slit_width>') >= 0:
                n1 = line.index('slit_width') + 11
                n2 = line.index('<', n1)
                self['slit_width'] = line[n1:n2]

            elif line.find('<slit_angle>') >= 0:
                n1 = line.index('slit_angle') + 11
                n2 = line.index('<', n1)
                self['slit_angle'] = line[n1:n2]
                
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

            elif line.find('<comment>') >= 0:
                n1 = line.index('comment') + 8
                n2 = line.index('<', n1)
                self['comment'] = line[n1:n2]
                

        # check that we have found what we expected to find
        if 'application' not in self:
            raise Exception, 'Failed to find application name in ' + run

        if self.is_not_power_onoff():

            if 'x_bin' not in self:
                raise Exception, 'Failed to find X_BIN in ' + run

            if 'y_bin' not in self:
                raise Exception, 'Failed to find Y_BIN in ' + run

            if 'x1_start' not in self:
                raise Exception, 'Failed to find X2_START in ' + run
                    
            if 'x1_size' not in self:
                raise Exception, 'Failed to find X2_SIZE in ' + run
                    
            if 'y1_start' not in self:
                raise Exception, 'Failed to find Y2_START in ' + run
                    
            if 'y1_size' not in self:
                raise Exception, 'Failed to find Y2_SIZE in ' + run
            
            if 'x2_start' not in self:
                raise Exception, 'Failed to find X2_START in ' + run
                    
            if 'x2_size' not in self:
                raise Exception, 'Failed to find X2_SIZE in ' + run
                    
            if 'y2_start' not in self:
                raise Exception, 'Failed to find Y2_START in ' + run
                    
            if 'y2_size' not in self:
                raise Exception, 'Failed to find Y2_SIZE in ' + run
                
            if 'target' not in self:
                self['target'] = 'UNKNOWN'

            if 'filters' not in self:
                self['filters'] = '---'

            if 'grating' not in self:
                self['grating'] = '---'

            if 'slit_width' not in self:
                self['slit_width'] = '---'

            if 'slit_angle' not in self:
                self['slit_angle'] = '---'

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
            out = self['target'].ljust(20) + '  ' + self['filters'].ljust(11) + ' ' + self['x_bin'] + 'x' + self['y_bin'] + '  ' + gain[2:].upper()
                  
            
        if self.number_windows() > 0:
            out += '  ' + self['x1_size'].rjust(4) + 'x' + self['y1_size'].ljust(4) + ' ' + self['x1_start'].ljust(3) + ' ' + self['y1_start'].ljust(4)
        if self.number_windows() > 1:
            out += '  ' + self['x2_size'].rjust(4) + 'x' + self['y2_size'].ljust(4) + ' ' + self['x2_start'].ljust(3) + ' ' + self['y2_start'].ljust(4)
            
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
            out = nwtd + '<strong>' + self['target'].ljust(20) + '</strong>' + etd

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
            speed = self['speed']
            out += ctd + self['filters'].ljust(11) + etd + ctd + self['x_bin'] + 'x' + self['y_bin'] + etd + ctd + speed + etd          
            
        if self.number_windows() > 0:
            out += ctd + self['x1_size'].rjust(4) + 'x' + self['y1_size'].ljust(4) + etd + td + self['x1_start'].ljust(3) + etd + td + self['y1_start'].ljust(4) + etd
        else:
            out += (td + etd)*3
            
        if self.number_windows() > 1:
            out += ctd + self['x2_size'].rjust(4) + 'x' + self['y2_size'].ljust(4) + etd + td + self['x2_start'].ljust(3) + etd + td + self['y2_start'].ljust(4) + etd
        else:
            out += (td + etd)*3

        if 'grating' in self:
            out += ctd + self['grating'] + etd
        else:
            out += td + etd

        if 'slit_width' in self:
            out += ctd + self['slit_width'] + etd
        else:
            out += td + etd

        if 'slit_angle' in self:
            out += ctd + self['slit_angle'] + etd
        else:
            out += td + etd
            
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
        if self.number_windows() > 0:
            if int(xstart) >= int(self['x1_start']) and int(xstart)+int(nx) <= int(self['x1_start'])+int(self['x1_size']) and \
                   int(ystart) >= int(self['y1_start']) and int(ystart)+int(ny) <= int(self['y1_start'])+int(self['y1_size']) and \
                   int(xbin) % int(self['x_bin']) == 0 and int(ybin) % int(self['y_bin']) == 0 and \
                   (int(xstart) - int(self['x1_start'])) %  int(xbin) == 0 and (int(ystart) - int(self['y1_start'])) %  int(ybin) == 0:
                return True

        if self.number_windows() > 1:
            if int(xstart) >= int(self['x2_start']) and int(xstart)+int(nx) <= int(self['x2_start'])+int(self['x2_size']) and \
                   int(ystart) >= int(self['y2_start']) and int(ystart)+int(ny) <= int(self['y2_start'])+int(self['y2_size']) and \
                   int(xbin) % int(self['x_bin']) == 0 and int(ybin) % int(self['y_bin']) == 0 and \
                   (int(xstart) - int(self['x2_start'])) %  int(xbin) == 0 and (int(ystart) - int(self['y2_start'])) %  int(ybin) == 0:
                return True

        return False

            
    def is_subset_of(self, uspec):
        """Checks that the Ultraspec is a subset of another, uspec, in the sense that it would be possible
        to get from the format of uspec to that of self and that they match in readout speed unless uspec is
        any calibration frame other than a bias in which the read speeds need not match. This is because a
        flat can still be used as a flat even if its read speed differed from the frame in question.
        """
            
        if self.is_power_onoff() or uspec.is_power_onoff():
            return False
    
        if (uspec.is_bias() or not uspec.is_calib()) and self['speed'] != uspec['speed']:
            return False

        if int(self['x_bin']) % int(uspec['x_bin']) != 0 or int(self['y_bin']) % int(uspec['y_bin']) != 0:
            return False

        if self.number_windows() > 0:

            if not uspec.contains_window(self['x1_start'], self['y1_start'], self['x1_size'], self['y1_size'], self['x_bin'], self['y_bin']):
                return False

        if self.number_windows() > 1:

            if not uspec.contains_window(self['x2_start'], self['y2_start'], self['x2_size'], self['y2_size'], self['x_bin'], self['y_bin']):
                return False

        return True

# Functions below are not class members

def match(uspec1, uspec2):
    """Compares two Ultraspecs returning true if they match.
    uspec1 and uspec2 are the names of two Ultracam objects to be compared.
    'Matching' means having compatible window formats. This means the same readout speed,
    binning factors, numbers of windows and window dimensions.
    """
    
    if uspec1.is_power_onoff() and uspec2.is_power_onoff():
        return True
    
    if uspec1.number_windows() != uspec2.number_windows():
        return False
    
    if uspec1['speed'] != uspec2['speed'] or \
       uspec1['x_bin'] != uspec2['x_bin'] or \
       uspec1['y_bin'] != uspec2['y_bin']:
        return False
    
    if uspec1.number_window_pairs() > 0:
        
        if uspec1['x1_start'] != uspec2['x1_start'] or \
               uspec1['x1_size'] != uspec2['x1_size'] or \
               uspec1['y1_start'] != uspec2['y1_start'] or \
               uspec1['y1_size'] != uspec2['y1_size']:
            return False
        
    if uspec1.number_window_pairs() > 1:

        if uspec1['x2_start'] != uspec2['x2_start'] or \
               uspec1['x2_size'] != uspec2['x2_size'] or \
               uspec1['y2_start'] != uspec2['y2_start'] or \
               uspec1['y2_size'] != uspec2['y2_size']:
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
<th>X1
<th>Y1
<th>NX2xNY2
<th>X2
<th>Y2
<th>Grat.
<th>Slit
<th>Slit
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
<th>width
<th>angle
<th>
<th>
<th>
<th>
"""



    

