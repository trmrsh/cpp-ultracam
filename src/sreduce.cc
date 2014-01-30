/*

!!begin
!!title   sreduce
!!author  T.R. Marsh
!!created 31 May 2006
!!revised 06 July 2007
!!root    sreduce
!!index   sreduce
!!descr   reduces time-series spectroscopy
!!class   Reduction
!!class   Observing
!!class   Spectra
!!css     style.css
!!head1   sreduce for reducing time-series spectroscopy

!!emph{sreduce} is the main ultracam program for reducing spectra. It can extract spectra of
multiple objects from multiple CCDs. These can be dumped to molly files.

Most of the customisation of !!emph{sreduce} is done through an ASCII data file read in at the start,
although other options that one often wants to vary (and which make no difference to the final
reduced data) are entered through the command input section.  Look at !!ref{sreduce.sre}{sreduce.sre}
for an example file. See the descriptions below for what each parameter does.

!!head2 Invocation

sreduce [source] rfile logfile ((url)/(file) first trim [(ncol nrow) twait tmax])/(flist) plot!!break

!!head2 Command line arguments

!!table

!!arg{source}{Data source, either 'l' for local, 's' for server or 'u' for ucm files. 'Local' means the
usual .xml and .dat files accessed directly. Do not add either .xml or .dat to the file name; these are assumed.
'u' means you will need to specify a list of files which should all be .ucm files (either with or without
the extension)}

!!arg{rfile}{ASCII File defining the reduction. The extension ".sre" is added by default.}

!!arg{logfile}{File to record the results of the reduction. The extension ".log" is added by default. Molly files
with the same root will also be created along with the CCD and aperture number as is 'run034_1_2.mol' which would
be the second aperture of the CCD 1. The CCD number is omitted if there is only one of them, but the aperture
number is always added. Do not add the '.log' yourself.}

!!arg{url/file}{If source = 'S', this argument specifies the name of the file of
interest (it can be a full URL or just the file name in which case the program will attempt to fill in
the rest using either the environment variable ULTRACAM_DEFAULT_URL or a URL appropriate to a server
running on the local host). If source = 'L', this should be a plain file name without .xml or .dat}

!!arg{first}{If source = 'S' or 'L', this argument specifies the first frame to reduce starting from 1}

!!arg{trim}{If source = 'S' or 'L', set trim=true to enable trimming of potential junk rows and
columns of each window}

!!arg{ncol}{If trim, then this specifies the number of columns nearest the readouts of each window to be snipped
off as these can be corrupted.}

!!arg{nrow}{If trim, then this specifies the number of rows to snip off the bottom of each window as these
can be corrupted.}

!!arg{twait}{If source = 'S' or 'L', time to wait between attempts to find a new exposure (seconds).}

!!arg{tmax}{If source = 'S' or 'L', maximum time to wait before giving up (seconds). Set = 0 to quit
as soon as a frame is not found.}

!!arg{flist}{If source = 'U', this is the name of a list of ULTRACAM files to reduce. These should be arranged in
temporal order to help the reduction move from one exposure to the next successfully.}

!!arg{splot}{true/false according to whether you want line plots of the spectra}

!!arg{tplot}{true/false according to whether you want trailed spectrum plots}

!!arg{hplot}{true/false according to whether you want a hard copy of the trailed spectrum plots at the end.}

!!table

!!head2 Reduction file

As well as the command line parameters, there are many others required to define the reduction, so many in fact that
file input is preferable, as many of them do not change much and for later checking on what was done. The !!ref{sreduce.sre}{file}
is in ASCII with a series of lines of the form  "option = value". Both "option" and "value" must contain no blanks. Lines
will only be read if they start with a non-blank character, with the exception of '#' which acts as a comment flag. Some
inputs are required, others are optional. These are indicated below. The option is read as the
first string before the '=' sign. The value is read as everything after
the '=' sign up to the first '#' (not escaped) or the end of the string.

In more-or-less alphabetical order (some grouping by purpose too), the options are as follows:

!!table

!!arg{abort_behaviour}{How to deal with problems. 'fussy' = give up at the first hint of
trouble. 'relaxed' = continue regardless.!!emph{Required}.}

!!arg{region_file}{Name of a master spectrum extraction region file, e.g. as set up by
!!ref{setreg.html}{setreg}. !!emph{Required}.}

!!arg{region_reposition_mode}{The method to be used to reposition the regions from frame
to frame. Options: !!emph{fixed} = no change. !!emph{individual} = move
each region separately. !!emph{reference} all objects are moved according to the movement of
just one of them, the reference object. !!emph{Required}.}

!!arg{region_fwhm}{If regions are to be shifted, then their position is measured by measuring the
position of the respective object and using this to adjust all regions. The measurement is carried out
using corss-correlation with a gaussian. The FWHM of this is fxed and specified by this parameter. It
is measured in unbinned pixels but will be truncated to at least 2 unbinned pixels.
!!emph{Required if region_reposition_mode = 'individual' or 'reference'}.}

!!arg{region_max_shift}{When the extraction regions are adjusted, this parameter imposes a maximum
upon the shift of the regions. It is measured in unbinned pixels. !!emph{Required if region_reposition_mode =
'individual' or 'reference'}.}

!!arg{region_hwidth}{Regions are adjusted by measurement of the object position in a collapsed profile.
This option allows you to median filter in X when making this profile to remove cosmic rays.}

!!arg{clobber}{[yes/no] Can the log file overwrite any previously existing file of the same name or not?
For safety say no, and the program will fall over if it finds the same name. Say yes if this irritates you.
!!emph{Required}.}

!!arg{coerce}{[yes/no] Should calibration frames be cropped to match data frames?
!!emph{Required} if any calibration has been enabled. Normally yes. You should never
coerce biases when it involves a change of binning factors. When coercing flat fields,
gain frames and readout noise frames, the programs scales to give an average rather than
a sum. It warns when it does this in case it is not what you want.}

!!arg{naccum}{Accumulate data by adding groups of frames prior to extraction}

!!arg{threshold}{Apply a threshold to give photon counts or not. Note that if you choose this mode then
any dark frame must correctly converted to photons as well. The thresholding is applied immediately after
bias subtraction is carried out. The gain should be set equal to 1 everywhere in this case.}

!!arg{photon}{If threshold = true, this is the threshold level above which a pixel counts
as 1 photon, below = 0}

!!arg{bias}{Name of bias frame.  If not specified, no bias subtraction is carried out.}

!!arg{dark}{Name of dark frame. If not specified, no dark subtraction
is carried out. The frame, if specified, should be bias subtracted. Typically this should
be the average of a large number of dark frames to reduce noise.}

!!arg{flat}{Name of flat field frame (divide into data to correct). If
not specified, not flat fielding is carried out. The frame, if specified, should
be bias-subtracted.}

!!arg{bad_pixel}{Name of a bad pixel mask file. Ignored if not specified. This is an ordinary
ULTRACAM file set to 0 everywhere except for 'bad' pixels. These should have positive integer values which rise
according to how 'bad' they are. It is up to you, the user, to define your own levels of badness.
Programs !!ref{smooth.html}{smooth} and
!!ref{badgen.html}{badgen} are supplied to help with the development of bad pixel masks.}

!!arg{gain}{Gain, electrons/count, or if preceded by '@', the name of a gain frame
frame giving gain for every pixel. !!emph{Required}.}

!!arg{readout}{Readout noise, or if preceded by '@', the name of readout noise
frame giving the readout noise for every pixel !!emph{in terms of variance} (counts**2).
!!emph{Required}.}

!!arg{spectrum_device}{Device to be used for line plots of spectra. !!emph{Required.}}

!!arg{trail_device}{Device to be used for trailed plots of spectra. !!emph{Required.}}

!!arg{trail_start}{The trail plots will start with this number of slots, and will be extended by the same number if necessary}

!!arg{hard_device}{Device to be used for hard copy of trailed spectra at the end. !!emph{Required.}}

!!arg{saturation}{Pixels in the output spectra can be masked if any pixel within the object region
is saturated.}

!!arg{sky_fit}{Whether you want to subtract the sky or not. Not for example if you are trying to extract arc spectra.
Note that you should also not re-position the apertures in this case.}

!!arg{sky_npoly}{Sky will be estimated using polynomial fits; this is the number of coefficients. sky_npoly = 1 implies
a constant.}

!!arg{sky_reject}{The rejection threshold for sky fits in terms of a sigma}

!!arg{version}{Date of version of sreduce which has to match the date in this program for anything to work at all. !!emph{Required}.}

!!arg{spectrum_scale_individual}{yes/no to scale spectra individually or not. If yes then the y limits for the spectrum plot
are worked out per spectrum, otherwise they are set to the most extreme values of all extracted spectra}

!!arg{spectrum_scale_method}{The method to use when computing the spectrum y-limits. Values are 'direct' which just uses
user-defined fixed limits, 'automatic' which sets to the total range and 'percentile' which uses percentile ranges.}

!!arg{spectrum_ylow}{Lower limit in the case of direct scaling}

!!arg{spectrum_yhigh}{Upper limit in the case of direct scaling}

!!arg{spectrum_plow}{Lower percentile limit in the case of percentile scaling}

!!arg{spectrum_phigh}{Upper percentile limit in the case of percentile scaling}

!!arg{trail_scale_method}{The method to use when computing the spectrum y-limits. Values are 'direct' which just uses
user-defined fixed limits, 'automatic' which sets to the total range and 'percentile' which uses percentile ranges.}

!!arg{trail_ilow}{Lower limit in the case of direct scaling}

!!arg{trail_ihigh}{Upper limit in the case of direct scaling}

!!arg{trail_plow}{Lower percentile limit in the case of percentile scaling}

!!arg{trail_phigh}{Upper percentile limit in the case of percentile scaling}

!!table

!!end

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cfloat>
#include <string>
#include <fstream>
#include <map>
#include <deque>
#include <time.h>
#include "trm/subs.h"
#ifdef HAVE_TRM_COLLY_H
#include "trm/colly.h"
#endif
#include "trm/constants.h"
#include "trm/array1d.h"
#include "trm/buffer2d.h"
#include "trm/input.h"
#include "trm/header.h"
#include "trm/plot.h"
#include "trm/specap.h"
#include "trm/mccd.h"
#include "trm/frame.h"
#include "trm/ultracam.h"

// Variables that are set by reading from the input file with read_reduce_file.
// Enclosed in a namespace for safety.

namespace Sreduce {

    // General
    Ultracam::Logger logger;                  // Logger object for logging to file and standard out
    ABORT_BEHAVIOUR abort_behaviour;          // How to react when faced with a problem.
    std::vector<float> saturation;            // Saturation levels for each CCD
    TERM_OUT terminal_output;                 // Level of terminal output
    bool coerce;                              // Coerce calibration frames to match the data or not
    int naccum;                               // Number of frames to accumulate per reduced spectrum
    bool threshold;                           // yes/no for photon counting thresholding
    float photon;                             // the level for photon counting if threshold

    // Calibration
    bool bias;                                // true/false for a bias frame
    Ultracam::Frame bias_frame;               // the bias frame if bias=true
    bool dark;                                // true/false for a dark frame
    Ultracam::Frame dark_frame;               // the dark frame if dark=true
    bool flat;                                // true/false for a flat field
    Ultracam::Frame flat_frame;               // the flat field if flat=true
    bool bad_pixel;                           // true/false for a bad pixel frame
    Ultracam::Frame bad_pixel_frame;          // the bad pixel frame if bad_pixel=true
    bool gain_const;                          // true if constant gain
    float gain;                               // value of constant gain
    Ultracam::Frame gain_frame;               // gain frame if gain_const=false
    bool readout_const;                       // true if constant readout noise
    float readout;                            // value of constant readout noise
    Ultracam::Frame readout_frame;            // readout noise frame if readout_frame=false

    // Regions
    Ultracam::Mspecap region_master;          // the input extraction region file
    REGION_REPOSITION_MODE region_reposition_mode;
    float  region_fwhm;
    float  region_max_shift;
    int    region_hwidth;

    // Sky fitting
    bool  sky_fit;
    int   sky_npoly;
    float sky_reject;

    // The plot devices
    std::string spectrum_device, trail_device, hard_device;

    // Spectrum plot scaling
    bool   spectrum_scale_individual;    // whether to scale individually or over all extracted spectra
    PLOT_SCALING_METHOD spectrum_scale_method;        // method 'direct', 'automatic' or 'percentile'
    float  spectrum_ylow;                // lower limit if 'direct'
    float  spectrum_yhigh;               // upper limit if 'direct'
    float  spectrum_plow;                // lower limit if 'percentile'
    float  spectrum_phigh;               // upper limit if 'percentile'

    // Trail plot scaling
    PLOT_SCALING_METHOD trail_scale_method;        // method 'direct', 'automatic' or 'percentile'
    float  trail_ilow;                     // lower limit if 'direct'
    float  trail_ihigh;                    // upper limit if 'direct'
    float  trail_plow;                     // lower limit if 'percentile'
    float  trail_phigh;                    // upper limit if 'percentile'

    // Amount to start and extend the trail plots by
    int trail_start;

};

// ************************************************************
//
// Main program starts here!
//
// ************************************************************

int main(int argc, char* argv[]){

    const std::string nothing = "";
    const std::string blank   = " ";
    const std::string hashb   = "# ";
    const std::string newl    = "\n";

    // For listing meanings of error codes in the output log.
    std::map<Sreduce::ERROR_CODES, std::string> error_names;
    error_names[Sreduce::OK] = "All OK";
    error_names[Sreduce::SKY_OVERLAPS_EDGE_OF_WINDOW] = "Sky regions overlaps edge of data window (non-fatal)";
    error_names[Sreduce::NO_SKY] = "No valid sky at all";
    error_names[Sreduce::SATURATION] = "Counts in at least one pixel of the object region exceeds the saturation level for the CCD";
    error_names[Sreduce::OBJECT_AT_EDGE_OF_WINDOW] = "Object region overlaps edge of data window";
    error_names[Sreduce::OBJECT_OUTSIDE_WINDOW] = "Object region lies outside all data windows";
    error_names[Sreduce::OBJECT_IN_MULTIPLE_WINDOWS] = "Object region lies across more than one window";
    error_names[Sreduce::REGION_INVALID] = "The extraction regions were invalidated";

    // Buffer for formatting output with 'sprintf'
    const int NSPRINTF=1024;
    char sprint_out[NSPRINTF];

    try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in the input variables
    input.sign_in("source",   Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("rfile",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("logfile",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("url",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("file",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("first",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("trim",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ncol",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("nrow",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("twait",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("tmax",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("flist",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("splot",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("tplot",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("hplot",    Subs::Input::LOCAL,  Subs::Input::PROMPT);

    // Get input parameters
    char source;
    input.get_value("source", source, 'S', "sSlLuU", "data source: L(ocal), S(erver) or U(cm)?");
    source = toupper(source);

    std::string rfile;
    input.get_value("rfile", rfile, "reduce", "name of reduction file");
    rfile = Subs::filnam(rfile, ".sre");

    std::string logfile;
    input.get_value("logfile", logfile, "reduce", "name of log file");
    std::string save_log = logfile;
    logfile = Subs::filnam(logfile, ".log");

    // Read reduction file
    Ultracam::read_sreduce_file(rfile, logfile);

    std::string url;
    if(source == 'S'){
        input.get_value("url", url, "url", "url of file");
    }else if(source == 'L'){
        input.get_value("file", url, "file", "name of local file");
    }

    size_t first, nfile;
    bool trim;
    std::vector<std::string> file;
    Ultracam::Mwindow mwindow;
    Subs::Header header;
    Ultracam::ServerData serverdata;
    Ultracam::Frame data, dvar, bad, sky;
    double twait, tmax;
    int ncol, nrow;
    if(source == 'S' || source == 'L'){
        input.get_value("first", first, size_t(1), size_t(1), size_t(9999999), "first frame to access (starting from 1)");
        input.get_value("trim", trim, true, "trim junk lower rows from windows?");
        if(trim){
        input.get_value("ncol", ncol, 0, 0, 100, "number of columns to trim from each window");
        input.get_value("nrow", nrow, 0, 0, 100, "number of rows to trim from each window");
        }
        input.get_value("twait", twait, 1., 0., 1000., "time to wait between attempts to find a frame (seconds)");
        input.get_value("tmax", tmax, 2., 0., 100000., "maximum time to wait before giving up trying to find a frame (seconds)");

        // Add extra stuff to URL if need be.
        if(url.find("http://") == std::string::npos && source == 'S'){
        char *DEFAULT_URL = getenv(Ultracam::ULTRACAM_DEFAULT_URL);
        if(DEFAULT_URL != NULL){
            url = DEFAULT_URL + url;
        }else{
            url = Ultracam::ULTRACAM_LOCAL_URL + url;
        }
        }else if(url.find("http://") == 0 && source == 'L'){
        throw Ultracam::Ultracam_Error("Should not specify local file as a URL");
        }

        // Finally, read the XML file.
        parseXML(source, url, mwindow, header, serverdata, trim, ncol, nrow, twait, tmax);

        if(source == 'S'){
        Sreduce::logger.logit("Server file name", url);
        }else{
        Sreduce::logger.logit("Data file name", url);
        }
        Sreduce::logger.logit("Starting from frame number", first);
        if(trim){
        Sreduce::logger.logit("Junk data trimmed.");
        }else{
        Sreduce::logger.logit("Junk data not trimmed.");
        }
        Sreduce::logger.logit("");

        // Version passed as a compiler option
        Sreduce::logger.logit(std::string(" ULTRACAM pipeline software version ") + VERSION);
        Sreduce::logger.logit("");
        Sreduce::logger.ofstr() << hashb << std::string("Information extracted from the XML headers follows:") << newl;
        Sreduce::logger.logit("");
        Subs::Header::start_string = hashb;
        Sreduce::logger.ofstr()<< header;
        Sreduce::logger.logit("");

        data.format(mwindow, header);

    }else{

        std::string flist;
        input.get_value("flist", flist, "files.lis", "name of local file list");

        Sreduce::logger.logit("Name of file list", flist);

        // Read file list
        std::string name;
        std::ifstream istr(flist.c_str());
        while(istr >> name){
        file.push_back(name);
        }
        istr.close();
        if(file.size() == 0)
        throw Ultracam::Ultracam_Error("No file names loaded");
        else
        std::cout << file.size() << " file names loaded" << std::endl;
        data.read(file[0]);

        first = 0;

    }

    // Carry on getting inputs
    bool splot;
    input.get_value("splot", splot, true, "do you want to plot spectra (bar chart form)?");
    bool tplot;
    input.get_value("tplot", tplot, true, "do you want to plot spectra (trailed form)?");
    bool hplot;
    input.get_value("hplot", hplot, true, "do you want a hard-copy of the trail at the end?");

    // All inputs read, save them as defaults
    input.save();

    // Region file
    Ultracam::Mspecap region;

    float dark_expose, dark_scale = 1.;

    // Need exposure time from dark frame in order to scale it.
    if(Sreduce::dark){
        Sreduce::dark_frame["Exposure"]->get_value(dark_expose);
        if(dark_expose <= 0.f)
        throw Ultracam::Ultracam_Error("Exposure time in dark frame must be > 0.");
    }

    // Write header info into log file
    Sreduce::logger.ofstr() << hashb << newl;
    Sreduce::logger.ofstr() << hashb << std::string("For each CCD of each frame reduced, the following information is printed:") << newl;
    Sreduce::logger.ofstr() << hashb << newl;
    Sreduce::logger.ofstr() << hashb << std::string("name/number mjd flag nsat expose ccd fwhm") << newl;
    Sreduce::logger.ofstr() << hashb << newl;
    Sreduce::logger.ofstr() << hashb << std::string("where 'name/number' is either the file name for ucm file list data or the frame number for data from the .dat files,") << newl;
    Sreduce::logger.ofstr() << hashb << std::string("'mjd' is the Modified Julian Date (UTC) at the centre of the exposure. MJD = JD-2400000.5, no correction for light travel") << newl;
    Sreduce::logger.ofstr() << hashb << std::string("etc is made on the basis that the key thing is have a well-understood & correct time. 'flag' is an indication of whether") << newl;
    Sreduce::logger.ofstr() << hashb << std::string("the time is thought to be reliable or not (1=OK,0=NOK). 'nsat' is the number of satellites associated with the timestamp") << newl;
    Sreduce::logger.ofstr() << hashb << std::string("(not quite the same as the data in the case of drift mode). 'expose' is the exposure time in seconds. 'ccd' is the ccd number") << newl;
    Sreduce::logger.ofstr() << hashb << std::string("(1=red,2=green,3=uv). 'fwhm' is the fitted FWHM, =0 if no fit made.") << newl;
    Sreduce::logger.ofstr() << hashb << newl;

    for(std::map<Sreduce::ERROR_CODES,std::string>::const_iterator mi=error_names.begin();
        mi!=error_names.end(); mi++)
        Sreduce::logger.ofstr() << hashb << std::string("Error code = ") << mi->first
                    << std::string(", meaning: ") << mi->second << newl;
    Sreduce::logger.ofstr() << hashb << newl;
    Sreduce::logger.ofstr() << hashb << std::string("For the fatal codes, 0. -1 0. 0 will be printed in place"
                            " of \"counts sigma sky nrej\"") << newl;
    Sreduce::logger.ofstr() << hashb << std::string("The square bracketed section is repeated for each aperture.") << newl;
    Sreduce::logger.ofstr() << hashb << newl;

    // Now get started. Open plot devices and optionally stop so that user can
    // adjust their position.
    Subs::Plot spectrum_plot, trail_plot;
    if(splot) spectrum_plot.open(Sreduce::spectrum_device);
    if(tplot) trail_plot.open(Sreduce::trail_device);

    // Declare the objects required for the reduction
    bool reliable = false; // Is the time reliable?
    Subs::Header::Hnode* hnode; // Iterator through headers
    Subs::Time ut_date;
    const Subs::Time TEST_TIME(1,Subs::Date::Jan,1999); // the time and a check time
    float expose; // exposure time
    int nsatellite = 0; // number of satellites
    bool first_file, has_a_time; // are we on the first data file? do we have a time?

    Sreduce::ERROR_CODES ecode;

    // Buffers for storing the extracted spectra.
    // For each frame there are NCCD CCDs, each of which has a fixed number of spectra
    std::vector<std::vector<std::vector<Subs::Array1D<float> > > > spectrum_data, spectrum_errors;

    nfile      = first;
    first_file = true;

#if defined(HAVE_LIBCOLLY) && defined(HAVE_TRM_COLLY_H)

    // open molly file(s), one for each CCD/aperture combination
    std::vector<std::vector<std::ofstream* > > mstr(Sreduce::region_master.size());
    std::string mollyfile;

    for(size_t nccd=0; nccd<Sreduce::region_master.size(); nccd++){

        // Through each region of each CCD
        for(size_t nreg=0; nreg<Sreduce::region_master[nccd].size(); nreg++){
        if(data.size() > 1)
            mollyfile = save_log + "_" + Subs::str(nccd+1) + "_" + Subs::str(nreg+1) + ".mol";
        else
            mollyfile = save_log + "_" + Subs::str(nreg+1) + ".mol";

        mstr[nccd].push_back(new std::ofstream(mollyfile.c_str()));
        }
    }

#endif

    // buffers if naccum > 1
    Ultracam::Frame dbuffer, vbuffer;
    int nstack = 0;
    double ttime = 0.;
    float texpose = 0.;

    for(;;){

        // Data input section
        if(source == 'S' || source == 'L'){

        // Carry on reading until data & time are OK
        bool get_ok, reset = (nfile == first);
        for(;;){
            if(!(get_ok = Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax, reset))) break;

            reliable   = data["Frame.reliable"]->get_bool();
            ut_date    = data["UT_date"]->get_time();
            nsatellite = data["Frame.satellites"]->get_int();

            if(serverdata.is_junk(nfile)){
            std::cerr << "Skipping file " << nfile << " which has junk data" << newl;
            nfile++;
            }else if(Sreduce::abort_behaviour != Sreduce::VERY_RELAXED && ut_date < TEST_TIME){
            std::cerr << "Skipping file " << nfile << " which has junk time = " << ut_date << newl;
            nfile++;
            }else{
            break;
            }
        }
        if(!get_ok) break;

        has_a_time = true;

        }else{

        if(nfile == file.size()) break;
        do{
            data.read(file[nfile]);
            hnode = data.find("UT_date");
            if(hnode->has_data()){
            hnode->value->get_value(ut_date);
            has_a_time = true;
            }else{
            std::cout << "No header item 'UT_date' found in file " << file[nfile] << ". Will just print time = file number to the log file but continue to reduce" << std::endl;
            has_a_time = false;
            }
            if(has_a_time && Sreduce::abort_behaviour != Sreduce::VERY_RELAXED && ut_date < TEST_TIME) nfile++;
        }while(nfile < file.size() && has_a_time && Sreduce::abort_behaviour != Sreduce::VERY_RELAXED && ut_date < TEST_TIME);
        if(nfile == file.size()) break;

        hnode = data.find("Frame.reliable");
        reliable = (hnode->has_data() && hnode->value->get_bool());
        hnode = data.find("Frame.satellites");
        nsatellite = hnode->has_data() ? hnode->value->get_int() : 0;

        }

        // Data now read in
        hnode = data.find("Exposure");
        if(hnode->has_data()){
        hnode->value->get_value(expose);
        }else{
        if(Sreduce::abort_behaviour == Sreduce::FUSSY)
            throw Ultracam::Ultracam_Error("Fussy mode: failed to find header item 'Exposure' in file " + file[nfile]);
        std::cerr << "WARNING: failed to find header item 'Exposure' in file " << file[nfile] << ", will set = 0" << std::endl;
        expose = 0.;
        }

        // Check numbers of CCDs
        if(Sreduce::bias && data.size() != Sreduce::bias_frame.size())
        throw Ultracam::Ultracam_Error("Conflicting CCD numbers between data and bias files.");

        if(Sreduce::dark && data.size() != Sreduce::dark_frame.size())
        throw Ultracam::Ultracam_Error("Conflicting CCD numbers between data and dark files.");

        if(Sreduce::flat && data.size() != Sreduce::flat_frame.size())
        throw Ultracam::Ultracam_Error("Conflicting CCD numbers between data and flat files.");

        if(Sreduce::bad_pixel && data.size() != Sreduce::bad_pixel_frame.size())
        throw Ultracam::Ultracam_Error("Conflicting CCD numbers between data and bad pixel files.");

        if(!Sreduce::gain_const && data.size() != Sreduce::gain_frame.size())
        throw Ultracam::Ultracam_Error("Conflicting CCD numbers between data and gain files.");

        if(!Sreduce::readout_const && data.size() != Sreduce::readout_frame.size())
        throw Ultracam::Ultracam_Error("Conflicting CCD numbers between data and readout files.");

        if(data.size() != Sreduce::region_master.size())
        throw Ultracam::Ultracam_Error("Conflicting CCD numbers between data and aperture files.");

        if(Sreduce::dark) dark_scale = expose/dark_expose;

        // If first file, crop bias, dark, flat etc if wanted.
        if(first_file){

        int xbin_data = data[0][0].xbin();
        int ybin_data = data[0][0].ybin();

        if(Sreduce::coerce){

            if(Sreduce::bias){
            if(Sreduce::bias_frame[0][0].xbin() != xbin_data ||
               Sreduce::bias_frame[0][0].ybin() != ybin_data)
                throw Ultracam::Ultracam_Error("Binning factors of bias and data fail to match; coercion not allowed in this case.");

            Sreduce::bias_frame.crop(data);
            }

            if(Sreduce::dark) Sreduce::dark_frame.crop(data);

            if(Sreduce::flat){

            int xbin_flat = Sreduce::flat_frame[0][0].xbin();
            int ybin_flat = Sreduce::flat_frame[0][0].ybin();

            Sreduce::flat_frame.crop(data);
            if(xbin_flat*ybin_flat != xbin_data*ybin_data){
                std::cerr << "\nWarning: the data and flat-field binning factors do no match and so after it has been re-formatted," << std::endl;
                std::cerr << "Warning: the flat-field will be scaled by the ratio of pixel areas. If you do not want this, you should" << std::endl;
                std::cerr << "Warning: prepare a correctly binned version by hand.\n" << std::endl;
                float scale = float(xbin_data*ybin_data)/(xbin_flat*ybin_flat);
                Sreduce::flat_frame /= scale;
            }
            }

            if(Sreduce::bad_pixel)      Sreduce::bad_pixel_frame.crop(data);

            if(!Sreduce::gain_const){
            int xbin_gain = Sreduce::gain_frame[0][0].xbin();
            int ybin_gain = Sreduce::gain_frame[0][0].ybin();

            Sreduce::gain_frame.crop(data);

            if(xbin_gain*ybin_gain != xbin_data*ybin_data){
                std::cerr << "\nWarning: the data and gain frame binning factors do no match and so after it has been re-formatted," << std::endl;
                std::cerr << "Warning: the gain frame will be scaled by the ratio of pixel areas. If you do not want this, you should" << std::endl;
                std::cerr << "Warning: prepare a correctly binned version by hand.\n" << std::endl;
                float scale = float(xbin_data*ybin_data)/(xbin_gain*ybin_gain);
                Sreduce::gain_frame /= scale;
            }
            }

            if(!Sreduce::readout_const){
            int xbin_read = Sreduce::readout_frame[0][0].xbin();
            int ybin_read = Sreduce::readout_frame[0][0].ybin();

            Sreduce::readout_frame.crop(data);

            if(xbin_read*ybin_read != xbin_data*ybin_data){
                std::cerr << "\nWarning: the data and readout frame binning factors do no match and so after it has been re-formatted," << std::endl;
                std::cerr << "Warning: the readout noise frame will be scaled by the ratio of pixel areas. If you do not want this, you should" << std::endl;
                std::cerr << "Warning: prepare a correctly binned version by hand.\n" << std::endl;
                float scale = float(xbin_data*ybin_data)/(xbin_read*ybin_read);
                Sreduce::readout_frame /= scale;
            }
            }
        }

        if(Sreduce::gain_const){
            Sreduce::gain_frame = data;
            Sreduce::gain_frame = Sreduce::gain;
        }

        if(Sreduce::readout_const){
            Sreduce::readout_frame = data;
            Sreduce::readout_frame = Sreduce::readout*Sreduce::readout;
        }

        // initialise format of bad pixels, sky and bias frame if there is not one
        bad = data;
        sky = data;

        if(!Sreduce::bias){
            Sreduce::bias_frame = data;
            Sreduce::bias_frame = 0;
        }

        }

        // Check frame formats
        if(Sreduce::bias && data != Sreduce::bias_frame)
        throw Ultracam::Ultracam_Error("bias frame does not have same format as data frame");

        if(Sreduce::dark && data != Sreduce::dark_frame)
        throw Ultracam::Ultracam_Error("dark frame does not have same format as data frame");

        if(Sreduce::flat && data != Sreduce::flat_frame)
        throw Ultracam::Ultracam_Error("flat frame does not have same format as data frame");

        if(Sreduce::bad_pixel && data != Sreduce::bad_pixel_frame)
        throw Ultracam::Ultracam_Error("flat frame does not have same format as data frame");

        if(data != Sreduce::readout_frame)
        throw Ultracam::Ultracam_Error("readout frame does not have same format as data frame");

        if(data != Sreduce::gain_frame)
        throw Ultracam::Ultracam_Error("gain frame does not have same format as data frame");

        // Now have data read in and calibration files in correct form, so apply calibration
        if(Sreduce::bias) data -= Sreduce::bias_frame;

        // Convert to 0 or 1 photons
        if(Sreduce::threshold) data.step(Sreduce::photon);

        // Define variance frame after bias subtraction but before dark subtraction
        dvar  = data;
        dvar.max(0);
        dvar /= Sreduce::gain_frame;
        dvar += Sreduce::readout_frame;

        if(Sreduce::dark) data -= dark_scale*Sreduce::dark_frame;

        nstack++;
        if(nstack < Sreduce::naccum){
        if(nstack == 1){
            dbuffer = data;
            vbuffer = dvar;
            ttime   = 0.;
            texpose = 0.f;
            std::cout << std::endl;
        }else{
            dbuffer += data;
            vbuffer += dvar;
        }
        if(has_a_time){
            ttime   += data["UT_date"]->get_double();
            texpose += expose;
            if(Sreduce::terminal_output == Sreduce::FULL || Sreduce::terminal_output == Sreduce::MEDIUM || Sreduce::terminal_output == Sreduce::LITTLE)
            std::cout << " Frame " << nstack << " of " << Sreduce::naccum << ", time = " << data["UT_date"]->get_time()
                  << " added into data buffer." << std::endl;
        }else{
            if(Sreduce::terminal_output == Sreduce::FULL || Sreduce::terminal_output == Sreduce::MEDIUM || Sreduce::terminal_output == Sreduce::LITTLE)
            std::cout << " Frame " << nstack << " of " << Sreduce::naccum << " added into data buffer." << std::endl;
        }

        }else{

        if(Sreduce::naccum > 1){
            data += dbuffer;
            dvar += vbuffer;
            if(has_a_time){
            ttime   += data["UT_date"]->get_double();
            texpose += expose;
            if(Sreduce::terminal_output == Sreduce::FULL || Sreduce::terminal_output == Sreduce::MEDIUM || Sreduce::terminal_output == Sreduce::LITTLE)
                std::cout << " Frame " << nstack << " of " << Sreduce::naccum << ", time = " << data["UT_date"]->get_time()
                      << " added into data buffer." << std::endl;
            ttime  /= nstack;
            data.set("UT_date",  new Subs::Htime(Subs::Time(ttime), "mean UT date and time at the centre of accumulated exposure"));
            data.set("Exposure", new Subs::Hfloat(texpose, "Exposure time, seconds"));

            }else{
            if(Sreduce::terminal_output == Sreduce::FULL || Sreduce::terminal_output == Sreduce::MEDIUM || Sreduce::terminal_output == Sreduce::LITTLE)
                std::cout << " Frame " << nstack << " of " << Sreduce::naccum << " added into data buffer." << std::endl;
            }
            nstack = 0;
            std::cout << std::endl;
        }

        // Apply flat field
        if(Sreduce::flat){
            data /= Sreduce::flat_frame;
            dvar /= Sreduce::flat_frame;
            dvar /= Sreduce::flat_frame;
        }

        // Bad pixels initialised to zero or the input frame if there is one.
        if(Sreduce::bad_pixel)
            bad = Sreduce::bad_pixel_frame;
        else
            bad = 0;

        // Update the sky regions
        Ultracam::sky_move(data, dvar, Sreduce::region_master, Sreduce::region_reposition_mode, Sreduce::region_fwhm, Sreduce::region_max_shift,
                   Sreduce::region_hwidth, ecode, region);

        if(Sreduce::abort_behaviour == Sreduce::FUSSY){

            switch(ecode){

            case Sreduce::OBJECT_OUTSIDE_WINDOW:
                throw Ultracam::Ultracam_Error("Fussy mode: object outside any window!");

            case Sreduce::REGION_INVALID:
                throw Ultracam::Ultracam_Error("Fussy mode: extraction regions invalid!");

            default:
                ;
            }

        }

        // Fit the sky regions
        if(Sreduce::sky_fit)
            Ultracam::sky_fit(data, dvar, region, Sreduce::sky_npoly, Sreduce::sky_reject, sky);
        else
            sky = 0;

        std::cerr << "extracting ... " << std::endl;

        // Add more spectra, and extract normally
        spectrum_data.push_back(std::vector<std::vector<Subs::Array1D<float> > >());
        spectrum_errors.push_back(std::vector<std::vector<Subs::Array1D<float> > >());

        Ultracam::ext_nor(data, dvar, region, Sreduce::sky_npoly, sky, spectrum_data.back(), spectrum_errors.back());

        // Format for output using 'sprintf' for reliability.
        sprintf(sprint_out, "%8i %16.10f 1 %1i %9.6f %2i", int(nfile), ut_date.mjd(), int(nsatellite), expose, int(ecode));
        Sreduce::logger.ofstr() << sprint_out;

        std::cerr << "plotting ... " << std::endl;

        // Plots
        if(splot){
            spectrum_plot.focus();
            Ultracam::plot_spectrum(spectrum_data.back(), spectrum_errors.back(), Sreduce::spectrum_scale_individual,
                        Sreduce::spectrum_scale_method, Sreduce::spectrum_ylow, Sreduce::spectrum_yhigh,
                        Sreduce::spectrum_plow, Sreduce::spectrum_phigh);
        }

        if(tplot){
            trail_plot.focus();
            Ultracam::plot_trail(spectrum_data, Sreduce::trail_start, false, Sreduce::trail_scale_method,
                     Sreduce::trail_ilow, Sreduce::trail_ihigh, Sreduce::trail_plow,
                     Sreduce::trail_phigh);
        }

        // Update the file and spectrum numbers
        first_file = false;

#if defined(HAVE_LIBCOLLY) && defined(HAVE_TRM_COLLY_H)

        // Write the spectrum to disk in molly format
        std::cerr << "Writing spectrum out in molly format" << std::endl;
        Subs::Header mhead;
        mhead.set("Xtra",       new Subs::Hdirectory("Molly data"));
        mhead.set("Xtra.FCODE", new Subs::Hint(2, "Molly format code"));
        mhead.set("Xtra.UNITS", new Subs::Hstring("COUNTS          ", "Units of fluxes"));
        mhead.set("Xtra.NARC",  new Subs::Hint(0,"Number of arc coefficients"));
        hnode = data.find("User.target");
        if(hnode->has_data()){
            mhead.set("Object", new Subs::Hstring(hnode->value->get_string()));
        }else{
            mhead.set("Object", new Subs::Hstring("undefined"));
        }
        mhead.set("Record",     new Subs::Hint(nfile, "Record number"));

        std::vector<std::string> original;
        const std::vector<std::vector<Subs::Array1D<float> > >& sdata = spectrum_data.back();
        const std::vector<std::vector<Subs::Array1D<float> > >& serrs = spectrum_errors.back();

        for(size_t nccd=0; nccd<sdata.size(); nccd++){
            mhead.set("CCD", new Subs::Hint(nccd+1, "CCD number"));
            for(size_t nap=0; nap<sdata[nccd].size(); nap++){
            mhead.set("Aperture", new Subs::Hint(nap+1, "Aperture number"));
            int npix = sdata[nccd][nap].size();
            mhead.set("Xtra.NPIX",  new Subs::Hint(npix, "Number of pixels"));

            hnode = data.find("UT_date");
            if(hnode->has_data()){
                ut_date = hnode->value->get_time();
                double jd = Constants::MJD2JD + ut_date.mjd();
                mhead.set("RJD",   new Subs::Hdouble(jd, "Julian Day"));
                mhead.set("Day",   new Subs::Hint(ut_date.day(), "day of month"));
                mhead.set("Month", new Subs::Hint(ut_date.month(), "month of year"));
                mhead.set("Year",  new Subs::Hint(ut_date.year(), "year"));
                mhead.set("UTC",   new Subs::Hdouble(ut_date.hour(), "hour"));
            }

            hnode = data.find("Exposure");
            if(hnode->has_data())
                mhead.set("Dwell",  new Subs::Hfloat(hnode->value->get_float(), "Exposure time (sec)"));

            Colly::write_molly_head(*mstr[nccd][nap], mhead, original, false);

            // We must add extra bytes at the start and end of fortran records
            int nbytes = 8*npix;
            mstr[nccd][nap]->write((char*)&nbytes, 4);
            mstr[nccd][nap]->write((char*)sdata[nccd][nap].ptr(), 4*sdata[nccd][nap].size());
            mstr[nccd][nap]->write((char*)serrs[nccd][nap].ptr(), 4*sdata[nccd][nap].size());
            mstr[nccd][nap]->write((char*)&nbytes, 4);
            }
        }
#else
        std::cerr << "Writing to molly via package 'colly' is disabled" << std::endl;
#endif
        }
        nfile++;
    }

    // Make a hard copy of the trail
    if(hplot){
        Subs::Plot hard_plot(Sreduce::hard_device);
        hard_plot.focus();
        cpgscr(0,1,1,1);
        cpgscr(1,0,0,0);
        Ultracam::plot_trail(spectrum_data, spectrum_data.size(), true, Sreduce::trail_scale_method,
                 Sreduce::trail_ilow, Sreduce::trail_ihigh, Sreduce::trail_plow,
                 Sreduce::trail_phigh);
    }

#if defined(HAVE_LIBCOLLY) && defined(HAVE_TRM_COLLY_H)

    // Close molly files
    for(size_t nccd=0; nccd<mstr.size(); nccd++){
        for(size_t nreg=0; nreg<region[nccd].size(); nreg++){
        mstr[nccd][nreg]->close();
        }
    }

#endif


    }

    // Handle errors
    catch(const Subs::Subs_Error& err){
    std::cerr << "\nSubs::Subs_Error:" << std::endl;
    std::cerr << err << std::endl;
    }
    catch(const Ultracam::File_Open_Error& err){
    std::cerr << "\nUltracam::File_Open_error:" << std::endl;
    std::cerr << err << std::endl;
    }
    catch(const Ultracam::Ultracam_Error& err){
    std::cerr << "\nUltracam::Ultracam_Error:" << std::endl;
    std::cerr << err << std::endl;
    }
    catch(const std::string& err){
    std::cerr << "\n" << err << std::endl;
    }

}

