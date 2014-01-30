/*

!!begin
!!title    folder
!!author   T.R. Marsh
!!created  20 Aug 2002
!!revised  06 Jul 2007
!!root     folder
!!index    folder
!!descr    phase folds server data on a specified period
!!class    Reduction
!!class    Observing
!!css      style.css
!!head1    phase folds server data on a specified period.

!!emph{folder} reads frames from the server and allocates them into a
regular series of bins folded on a specified period. Optionally it can
shift the frame to align reference targets before adding the frames
in.  It also multiplies the data by the sine and cosine of the phase
and produces summed versions of these which can be used to see what is
varying strongly on the given period. At the end of the program, the
data files will be dumped to disk in the form of 'root_01', 'root_02',
'root_03', ... 'root_nbin', 'root_constant', 'root_cosine',
'root_sine', 'root_amplitude' and 'root_phase'. These can be plotted
with !!ref{plot.html}{plot}. The first set are the phase binned files
(zero-padded to make it easier to display them in order). Then come
the sum of the files, the sum of the files times cosine, the sum
times sine, and the equivalent amplitudes and phases. The phase, phi0,
is defined in terms of cos(2*Pi*(phi-phi0)) and ranges from -0.5 to
0.5. The amplitude file is probably most useful if you just want to
see if you are detecting something.

It is possible to add the results of multiple runs together by loading in
old frames. The only condition is that you must use the same ephemeris (which
will be checked). As a by-product of this program you get a shifted & summed
file, but no cosmic ray checking is done I'm afraid.

If you try shifting, you have to specify an aperture file with reference stars marked.
Any CCD without a reference star will be added with 0 shift. If a CCD has reference stars
but they are lost because of porr conditions, all the CCDs for that exposure will be skipped.

!!head2 Invocation

folder [source] ((url)/(file) first trim [(ncol nrow) twait tmax])/(flist) fussy
nsave bias (biasframe) flat (flatframe) tzero period etype position (telescope)
nbins root new shift (aperture [xshift yshift] smethod [fwhm1d hwidth1d])

!!head2 Arguments

!!table

!!arg{source}{Data source, either 'l' for local, 's' for server or 'u' for ucm files. 'Local' means the
usual .xml and .dat files accessed directly. Do not add either .xml or .dat to the file name; these are assumed.
'u' means you will need to specify a list of files which should all be .ucm files (either with or without
the extension)}

!!arg{url/file}{If source='S', 'url' is the complete URL of the file, e.g. 'http://127.0.0.1:8007/run00000012',
or just the file part in which case the program will try to find a default part to
add from the environment variable ULTRACAM_DEFAULT_URL. Failing this it will add
http://127.0.0.1:8007/, i.e. the local host. If source='L' then this should be plain file name
without .xml or .dat}

!!arg{first}{If source = 'S' or 'L', this is the number of the first file, starting from 1.}

!!arg{trim}{If source = 'S' or 'L', set trim=true to enable trimming of potential junk rows and
columns of each window}

!!arg{ncol}{If trim, then this specifies the number of columns nearest the readouts of each window to be snipped
off as these can be corrupted.}

!!arg{nrow}{If trim, then this specifies the number of rows to snip off the bottom of each window as these
can be corrupted.}

!!arg{twait}{If source = 'S' or 'L', time to wait between attempts to find a new exposure (seconds).}

!!arg{tmax}{If source == 'S' or 'L', maximum time to wait before giving up (seconds).
Set = 0 to quit as soon as a frame is not found.}

!!arg{flist}{If source = 'U', this is the name of a list of .ucm files to read.}

!!arg{fussy}{If set true, frames with times flagged 'unreliable' (which may not be that bad) are booted out}

!!arg{nsave}{Intermediate results can be saved every nsave frames. These are sent to files of the form
"temp_root_" etc, following the convention above. Do not set this too small (e.g. 1) or you will slow the program
down considerably, in addition to possibly encountering a singular matrix warning early on (although this is not
serious). Enter 0 to not bother saving at all until the program terminates.}

!!arg{bias}{true/false according to whether you want to subtract a bias frame. You can specify a full-frame
bias because it will be cropped to match whatever your format is. This is useful for ultracam because of
the different bias levels of the 6 readouts.}

!!arg{biasframe}{If bias, then you need to specify the name of the bias frame}

!!arg{flat}{true/false according to whether you want to flat field the data. You can specify a full-frame
flat because it will be cropped to match whatever your format is. The flat will be !!emph{divided} into
your data.}

!!arg{flatframe}{If flat, then you need to specify the name of the flatfield}

!!arg{tzero}{Zero point of ephemeris (days). NB It should be a Modified Julian Day, i.e. JD - 2400000.5.
There are two choices of timescale; see 'etype'}

!!arg{period}{Period of ephemeris (days)}

!!arg{etype}{Type of ephemeris: 'BJD' for TDB corrected to the barycentre in terms of JD, 'BMJD' for the same
in terms of MJD, 'HJD' and 'HMJD' the equivalents in terms of UTC corrected to the heliocentre,}

!!arg{position}{The celestial position of the target, as required for light-travel time
corrections. Should be in the form RA Dec Epoch. FK5 coordinates are assumed. The sign of
the declination, whether positive or negative, !!emph{must} be given.}

!!arg{telescope}{Telescope name (only if barycentric ephemeris because then it is assumed that
greater accuracy is wanted). If you enter an invalid name, you will get a list of possibilities.}

!!arg{nbins}{Number of phase bins. Can be zero in which case only
the sine/cosine stuff will be carried out. The main consideration here is
that for speed, all frames are kept in memory. This implies the raw
data, bias and flat, the 5 frames associated with the sine/cosine
stuff, and then the 'nbins' phase bins. Internally the pipeline stores
data as floats, so a full frame requires ~ 12 Mbytes. Thus in full
frame mode, more than 50 bins might be pushing it, depending upon the available memory.}

!!arg{root}{Root name for storage of data files at end. See above for the names that they will have.}

!!arg{new}{True/false according to whether the data files are new or to be read from some previously stored on disk. This
option allows you to add multiple runs-worth of data. Be careful not to corrupt the files by Ctrl-C-ing during the
final save. If you have accumulated a huge store of files, you may want to save a copy of them for safety every
so often to avoid this possibility.}

!!arg{shift}{True/false according to whether you want to try shifting the frame before ading them. This option requires
the specification of an aperture file that must have been set up from data equivalent to that which you want to shift.
The frames will all be shifted to match whichever file you set the apertures up on in the first place.}

!!arg{aperture}{The file of apertures if shift = true. You must mark the stars you want to use for determination of
the shifts as 'reference' stars using the 'S' option in !!ref{setaper.html}{setaper}. If any CCD has no reference stars marked,
no shift will be made. If you define reference stars but they get lost, a warning will be issued and the entire set of CCDs
will be skipped. If you are combining data from a number of runs, this file must be the same every
time. To minimise edge effects, you should try to define the aperture from a frame that represents the median position.}

!!arg{xshift}{Initial shift in X. This is useful if the targets have shifted grossly with respect to their position when
you set up the aperture file.}

!!arg{yshift}{Initial shift in Y. This is useful if the targets have shifted grossly with respect to their position when
you set up the aperture file.}

!!arg{smethod}{Shift method. 'N' for nearest pixel, 'L' for linear interpolation in both X and Y from 4 nearest pixels.
NB only simple translations are supported, no rotations or other distortions. 'N' is faster than, but not as good as
'L'. Linear interpolation is recommended unless speed is critical.}

!!arg{fwhm1d}{FWHM for 1D search used to re-position apertures. Unbinned pixels}

!!arg{hwidth1d}{Half-width in unbinned pixels for 1D search used to re-position apertures.}
!!table

!!end

*/

#include <cstdlib>
#include <climits>
#include <string>
#include <fstream>
#include "trm/subs.h"
#include "trm/buffer2d.h"
#include "trm/ephem.h"
#include "trm/position.h"
#include "trm/input.h"
#include "trm/header.h"
#include "trm/frame.h"
#include "trm/aperture.h"
#include "trm/mccd.h"
#include "trm/ultracam.h"

// Main program

int main(int argc, char* argv[]){

  using Ultracam::File_Open_Error;
  using Ultracam::Ultracam_Error;
  using Ultracam::Input_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in input variables
    input.sign_in("source",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("url",       Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("file",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("first",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("trim",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ncol",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("nrow",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("twait",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("tmax",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("flist",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("fussy",     Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("nsave",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("bias",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("biasframe", Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("flat",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("flatframe", Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("tzero",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("period",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("etype",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("position",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("telescope", Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nbins",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("root",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("new",       Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("shift",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("aperture",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("xshift",    Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("yshift",    Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("smethod",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("fwhm1d",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("hwidth1d",  Subs::Input::GLOBAL, Subs::Input::NOPROMPT);

    // Get inputs

    char source;
    input.get_value("source", source, 'S', "sSlLuU", "data source: L(ocal), S(erver) or U(cm)?");
    source = toupper(source);
    std::string url;
    if(source == 'S'){
      input.get_value("url", url, "url", "url of file");
    }else if(source == 'L'){
      input.get_value("file", url, "file", "name of local file");
    }
    size_t first;
    bool trim, fussy;
    std::vector<std::string> file;
    Ultracam::Mwindow mwindow;
    Subs::Header header;
    Ultracam::ServerData serverdata;
    Ultracam::Frame data, dvar;
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
    throw Ultracam::Input_Error("Should not specify local file as a URL");
      }

      parseXML(source, url, mwindow, header, serverdata, trim, ncol, nrow, twait, tmax);
      data.format(mwindow, header);

    }else{

      std::string flist;
      input.get_value("flist", flist, "files.lis", "name of local file list");

      // Read file list
      std::string name;
      std::ifstream istr(flist.c_str());
      while(istr >> name){
    file.push_back(name);
      }
      istr.close();
      if(file.size() == 0)
    throw Input_Error("No file names loaded");

      data.read(file[0]);

    }

    input.get_value("fussy", fussy, true, "do you want to ignore frames with times flagged as unreliable?");

    // Carry on getting inputs
    size_t nsave;
    input.get_value("nsave", nsave, size_t(10), size_t(0), size_t(INT_MAX), "number of frames between intermediate result saves");

    bool bias;
    input.get_value("bias", bias, true, "do you want to subtract a bias frame?");
    Ultracam::Frame bias_frame;
    if(bias){
      std::string sbias;
      input.get_value("biasframe", sbias, "bias", "name of bias frame");
      bias_frame.read(sbias);
      bias_frame.crop(data);
    }

    // variance frame
    float gain = 1.1, read = 4.;
    dvar = data;
    dvar.max(0.);
    dvar /= gain;
    dvar += read*read;

    bool flat;
    input.get_value("flat", flat, true, "do you want to apply a flat field?");
    Ultracam::Frame flat_frame;
    if(flat){
      std::string sflat;
      input.get_value("flatframe", sflat, "flat", "name of flatfield frame");
      flat_frame.read(sflat);
      flat_frame.crop(data);
    }

    double tzero;
    input.get_value("tzero", tzero, 50000., 0., 1.e8, "zero-point of periodic signal (days)");
    double period;
    input.get_value("period", period, 0.0001, 1.e-10, 1.e3, "period of periodic signal (days)");
    std::string etype;
    input.get_value("etype", etype, "HMJD", "what type of ephemeris is this?");
    etype = Subs::toupper(etype);
    Subs::Ephem ephem;
    if(etype == "BJD"){
      ephem = Subs::Ephem(tzero, period, Subs::Ephem::BJD);
    }else if(etype == "BMJD"){
      ephem = Subs::Ephem(tzero, period, Subs::Ephem::BMJD);
    }else if(etype == "HJD"){
      ephem = Subs::Ephem(tzero, period, Subs::Ephem::HJD);
    }else if(etype == "HMJD"){
      ephem = Subs::Ephem(tzero, period, Subs::Ephem::HMJD);
    }else{
      throw Ultracam_Error("Could not recognise ephemeris type = " + etype + "\n Possible types are: BJD, BMJD, HJD, HMJD");
    }
    std::string spos;
    input.get_value("position", spos, "15:09:32.2 +34:02:56.7 2000", "position of target (RA, Dec, Epoch)");
    Subs::Position position(spos);
    std::string stel;
    Subs::Telescope tel;
    if(etype == "BJD" || etype == "BMJD"){
      input.get_value("telescope", stel, "WHT", "telescope name");
      tel = Subs::Telescope(stel);
    }
    size_t nbins;
    input.get_value("nbins", nbins, size_t(10), size_t(0), size_t(1000), "number of phase bins");
    int ndigit = int(log(nbins+.5)/log(10.)+1.);
    std::string root;
    input.get_value("root", root, "fold", "root name for saved files");
    bool newfile;
    input.get_value("new", newfile, false, "do you want to create new files?");
    bool shift;
    input.get_value("shift", shift, false, "do you want to shift images to account for image motion?");
    Ultracam::Maperture master_aperture;
    char smethod;
    float fwhm1d, xshift, yshift;
    int   hwidth1d;
    Ultracam::SHIFT_METHOD shift_method = Ultracam::NEAREST_PIXEL;
    if(shift){
      std::string saper;
      input.get_value("aperture", saper, "aper", "enter aperture file with reference stars");
      master_aperture = Ultracam::Maperture(saper);
      if(master_aperture.size() != data.size())
    throw Ultracam::Input_Error("Number of CCDs in aperture file does not match number in data file");

      input.get_value("xshift",  xshift,  0.f, -1000.f, 1000.f, "initial shift in X to help acquire reference stars");
      input.get_value("yshift",  yshift,  0.f, -1000.f, 1000.f, "initial shift in Y to help acquire reference stars");
      input.get_value("smethod", smethod, 'L', "nNlL", "method to use for shifting");
      if(toupper(smethod) == 'N'){
    shift_method = Ultracam::NEAREST_PIXEL;
      }else if(toupper(smethod) == 'L'){
    shift_method = Ultracam::LINEAR_INTERPOLATION;
      }else{
    throw Ultracam::Input_Error("Shift method unrecognised");
      }
      input.get_value("fwhm1d",   fwhm1d,  10.f, 2.f, 1000.f, "FWHM for 1D search for aperture re-positioning");
      input.get_value("hwidth1d", hwidth1d, int(2.*fwhm1d)+1, int(fwhm1d+1), INT_MAX, "half-width of 1D search region");
    }

    // Save inputs
    input.save();

    // Now create other frames
    Ultracam::Frame constant, sine, cosine, amp, phs;
    std::vector<Ultracam::Frame> bin(nbins);
    double sum, sum_c, sum_s, sum_cc, sum_cs, sum_ss;

    amp.format(data);
    phs.format(data);

    if(newfile){

      sum = sum_c = sum_s = sum_cc = sum_cs = sum_ss = 0.;

      constant.format(data);
      constant = 0.f;
      constant.set("UT_date",       new Subs::Htime(Subs::Time(1,1,2002,0,0,0.), "UTC date and time"));
      constant.set("Exposure",      new Subs::Hfloat(0.5f, "Exposure time"));
      constant.set("folder",        new Subs::Hdirectory("Phase folding program info"));
      constant.set("folder.sum",    new Subs::Hdouble(sum,    "Sum of 1"));
      constant.set("folder.sum_c",  new Subs::Hdouble(sum_c,  "Sum of cosine"));
      constant.set("folder.sum_s",  new Subs::Hdouble(sum_s,  "Sum of sine"));
      constant.set("folder.sum_cc", new Subs::Hdouble(sum_cc, "Sum of cosine*cosine"));
      constant.set("folder.sum_cs", new Subs::Hdouble(sum_cs, "Sum of cosine*sine"));
      constant.set("folder.sum_ss", new Subs::Hdouble(sum_ss, "Sum of sine*sine"));
      constant.set("folder.tzero",  new Subs::Hdouble(tzero,  "Ephemeris zero-point"));
      constant.set("folder.period", new Subs::Hdouble(period, "Ephemeris period"));

      cosine.format(data);
      cosine = 0.f;
      cosine.set("folder",        new Subs::Hdirectory("Phase folding program info"));
      cosine.set("folder.sum",    new Subs::Hdouble(sum,    "Sum of 1"));
      cosine.set("folder.sum_c",  new Subs::Hdouble(sum_c,  "Sum of cosine"));
      cosine.set("folder.sum_s",  new Subs::Hdouble(sum_s,  "Sum of sine"));
      cosine.set("folder.sum_cc", new Subs::Hdouble(sum_cc, "Sum of cosine*cosine"));
      cosine.set("folder.sum_cs", new Subs::Hdouble(sum_cs, "Sum of cosine*sine"));
      cosine.set("folder.sum_ss", new Subs::Hdouble(sum_ss, "Sum of sine*sine"));
      cosine.set("folder.tzero",  new Subs::Hdouble(tzero,  "Ephemeris zero-point"));
      cosine.set("folder.period", new Subs::Hdouble(period, "Ephemeris period"));

      sine.format(data);
      sine = 0.f;
      sine.set("folder",        new Subs::Hdirectory("Phase folding program info"));
      sine.set("folder.sum",    new Subs::Hdouble(sum,    "Sum of 1"));
      sine.set("folder.sum_c",  new Subs::Hdouble(sum_c,  "Sum of cosine"));
      sine.set("folder.sum_s",  new Subs::Hdouble(sum_s,  "Sum of sine"));
      sine.set("folder.sum_cc", new Subs::Hdouble(sum_cc, "Sum of cosine*cosine"));
      sine.set("folder.sum_cs", new Subs::Hdouble(sum_cs, "Sum of cosine*sine"));
      sine.set("folder.sum_ss", new Subs::Hdouble(sum_ss, "Sum of sine*sine"));
      sine.set("folder.tzero",  new Subs::Hdouble(tzero,  "Ephemeris zero-point"));
      sine.set("folder.period", new Subs::Hdouble(period, "Ephemeris period"));

      for(size_t i=0; i<nbins; i++){
    bin[i].format(data);
    bin[i] = 0.f;
    bin[i].set("folder",        new Subs::Hdirectory("Phase folding program info"));
    bin[i].set("folder.tzero",  new Subs::Hdouble(tzero,  "Ephemeris zero-point"));
    bin[i].set("folder.period", new Subs::Hdouble(period, "Ephemeris period"));
      }
      std::cout << "Frames created & initialised." << std::endl;

    }else{

      constant.read(root + "_constant");
      if(constant != data)
    throw Ultracam::Input_Error(std::string("File = ") + root + std::string("_constant has incompatible format"));

      cosine.read(root + "_cosine");
      if(cosine != data)
    throw Ultracam::Input_Error(std::string("File = ") + root + std::string("_cosine has incompatible format"));

      sine.read(root + "_sine");
      if(sine != data)
    throw Ultracam::Input_Error(std::string("File = ") + root + std::string("_sine has incompatible format"));

      // Retrieve old sums
      constant["folder.sum"]->get_value(sum);
      constant["folder.sum_c"]->get_value(sum_c);
      constant["folder.sum_s"]->get_value(sum_s);
      constant["folder.sum_cc"]->get_value(sum_cc);
      constant["folder.sum_cs"]->get_value(sum_cs);
      constant["folder.sum_ss"]->get_value(sum_cs);

      // Cursory checks
      if(sum != cosine["folder.sum"]->get_double())
    throw Ultracam::Input_Error("sum in cosine file does not that in constant file");

      if(sum != sine["folder.sum"]->get_double())
    throw Ultracam::Input_Error("sum in sine file does not that in constant file");

      if(tzero  != constant["folder.tzero"]->get_double())
    throw Ultracam::Input_Error("tzero in constant file does not match the one you have specified");

      if(period != constant["folder.period"]->get_double())
    throw Ultracam::Input_Error("period in constant file does not match the one you have specified");

      for(size_t i=0; i<nbins; i++){
    bin[i].read(root + "_" + Subs::str(int(i+1),ndigit));
    if(bin[i] != data)
      throw Ultracam::Input_Error("File = " + root + "_" + Subs::str(int(i+1),ndigit) + " has incompatible format");
    if(tzero  != bin[i]["folder.tzero"]->get_double())
      throw Ultracam::Input_Error("tzero in phase bin file does not match the one you have specified");

    if(period != bin[i]["folder.period"]->get_double())
      throw Ultracam::Input_Error("period in phase bin file does not match the one you have specified");
      }
      std::cout << "Files loaded." << std::endl;
    }

    bool initialised = false;
    size_t nfile, nsofar=0;
    if(source == 'S' || source == 'L'){
      nfile = first;
    }else{
      nfile = first = 0;
    }

    Subs::Time ut_date, ttime(1,Subs::Date::May,2002);
    Ultracam::Maperture last_aperture;
    bool reliable = false;
    Subs::Header::Hnode* hnode;

    for(;;){

      // Get data
      if(source == 'S' || source == 'L'){

    // Carry on reading until data & time are OK
    bool get_ok;
    for(;;){
      if(!(get_ok = Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax))) break;
      ut_date  = data["UT_date"]->get_time();
      reliable = data["Frame.reliable"]->get_bool();
      if(serverdata.is_junk(nfile)){
        std::cerr << "Skipping file number " << nfile << " which has junk data" << std::endl;
        nfile++;
      }else if(ut_date < ttime){
        std::cerr << "Skipping file number " << nfile << " which has junk time = " << ut_date << std::endl;
        nfile++;
      }else if(fussy && !reliable){
        std::cerr << "Skipping file number " << nfile << " which has an unreliable time = " << ut_date << std::endl;
        nfile++;
      }else{
        break;
      }
    }
    if(!get_ok) break;

    std::cout << "Processing frame number " << nfile << ", time = " << data["UT_date"]->get_time() << std::endl;


      }else{

    while(nfile < file.size()){
      data.read(file[nfile]);
      ut_date = data["UT_date"]->get_time();
      hnode = data.find("Frame.reliable");
      reliable = (hnode->has_data() && hnode->value->get_bool());
      if(reliable)
        std::cerr << "time reliable" << std::endl;
      else
        std::cerr << "time unreliable" << std::endl;

      if(ut_date < ttime){
        std::cerr << "Skipping file " << file[nfile] << " which has junk time = " << ut_date << std::endl;
        nfile++;
      }else if(fussy && !reliable){
        std::cerr << "Skipping file " << file[nfile] << " which has an unreliable time = " << ut_date << std::endl;
        nfile++;
      }else{
        break;
      }
    }
    if(nfile == file.size()) break;

    std::cout << "Processing file = " << file[nfile] << ", time = " << ut_date << std::endl;

      }

      // Apply calibrations
      if(bias) data -= bias_frame;
      dvar = data;
      dvar.max(0.);
      dvar /= gain;
      dvar += read*read;
      if(flat) data /= flat_frame;

      // Compute shifts
      std::vector<Ultracam::Shift_info> shift_info(data.size());
      bool addin = true;
      std::vector<bool> has_a_ref(data.size(), false);
      if(shift){
    Ultracam::Maperture aperture;

    if(!initialised) {
      aperture = master_aperture;

      // Set flags to indicate whether we skip a frame if no aperture were located
      for(size_t nccd=0; nccd<data.size() && addin; nccd++){
        for(size_t naper=0; naper<aperture[nccd].size(); naper++){
          Ultracam::Aperture* app  = &aperture[nccd][naper];
          if(app->valid() && app->ref()){
        has_a_ref[nccd] = true;
        break;
          }
        }
      }

      // Apply initial shift
      for(size_t nccd=0; nccd<data.size(); nccd++){
        for(size_t naper=0; naper<aperture[nccd].size(); naper++){
          Ultracam::Aperture* app = &aperture[nccd][naper];
          app->set_xref(app->xref() + xshift);
          app->set_yref(app->yref() + yshift);
        }
      }
      initialised = true;

    }else{
      aperture = last_aperture;
    }

    for(size_t nccd=0; nccd<data.size() && addin; nccd++){

      // Loop over apertures
      float sx = 0., sy = 0.;
      int nap = 0;

      for(size_t naper=0; naper<aperture[nccd].size(); naper++){

        Ultracam::Aperture* app  = &aperture[nccd][naper];
        Ultracam::Aperture* rapp = &master_aperture[nccd][naper];

        // Only consider valid reference apertures
        if(app->valid() && app->ref()){

          try{

        const Ultracam::Windata &dwin = data[nccd].enclose(app->xref(), app->yref());
        const Ultracam::Windata &vwin = dvar[nccd].enclose(app->xref(), app->yref());

        // window found for this aperture, but need to check that
        // star aperture is fully enclosed by it
        if(
           dwin.left()   < app->xref()-app->rstar() &&
           dwin.bottom() < app->yref()-app->rstar() &&
           dwin.right()  > app->xref()+app->rstar() &&
           dwin.top()    > app->yref()+app->rstar()){

          float xstart   = dwin.xcomp(app->xref());
          float ystart   = dwin.ycomp(app->yref());
          float xref     = dwin.xcomp(rapp->xref());
          float yref     = dwin.ycomp(rapp->yref());

          float fwhm_x   = fwhm1d/dwin.xbin();
          fwhm_x = fwhm_x > 2.f ? fwhm_x : 2.f;
          float fwhm_y   = fwhm1d/dwin.ybin();
          fwhm_y = fwhm_y > 2.f ? fwhm_y : 2.f;

          int   hwidth_x = hwidth1d/dwin.xbin();
          hwidth_x = hwidth_x > int(fwhm_x+1.) ? hwidth_x : int(fwhm_x+1.);
          int   hwidth_y = hwidth1d/dwin.ybin();
          hwidth_y = hwidth_y > int(fwhm_y+1.) ? hwidth_y : int(fwhm_y+1.);
          double xpos, ypos;
          float xe, ye;

          // Remeasure the position
          Ultracam::findpos(dwin, vwin, dwin.nx(), dwin.ny(), fwhm_x, fwhm_y, hwidth_x, hwidth_y, xstart, ystart, true,
                    xpos, ypos, xe, ye);
          sx += dwin.xbin()*(xpos-xref);
          sy += dwin.ybin()*(ypos-yref);
          nap++;
        }
          }
          catch(const Ultracam_Error& err){
        throw Ultracam_Error("failed to enclose target start position");
          }
        }
      }

      // Update aperture file
      if(nap){
        sx /= nap;
        sy /= nap;
        // -ve shifts to compensate
        shift_info[nccd].dx = -sx;
        shift_info[nccd].dy = -sy;
        for(size_t naper=0; naper<aperture[nccd].size(); naper++){
          Ultracam::Aperture* app  = &aperture[nccd][naper];
          Ultracam::Aperture* rapp = &master_aperture[nccd][naper];
          app->set_xref(rapp->xref() + sx);
          app->set_yref(rapp->yref() + sy);
        }
        std::cout << "Will apply a shift of (" << shift_info[nccd].dx << "," << shift_info[nccd].dy << ") to CCD " << nccd + 1 << std::endl;
        shift_info[nccd].ok = true;

      }else{

        shift_info[nccd].dx = 0.f;
        shift_info[nccd].dy = 0.f;
        if(has_a_ref[nccd]){
          std::cerr << "No valid reference apertures located for CCD number " << nccd + 1 << std::endl;
          std::cerr << "This and the other CCDs will not be added in, a somewhat crude " << std::endl;
          std::cerr << "but correct course of action." << std::endl;
          shift_info[nccd].ok = false;
          addin               = false;
        }else{
          std::cout << "Will apply a shift of (" << shift_info[nccd].dx << "," << shift_info[nccd].dy << ") to CCD " << nccd + 1 << std::endl;
          shift_info[nccd].ok = false;
        }
      }
    }

    last_aperture = aperture;

      }else{

    for(size_t nccd=0; nccd<data.size(); nccd++){
      shift_info[nccd].dx = 0.f;
      shift_info[nccd].dy = 0.f;
      shift_info[nccd].ok = true;
    }

      }

      // update file number
      nfile++;

      if(addin){

    // Compute phase info, applying corrections for timescales and light-travel
    double tcorr;
    if(ephem.get_tscale() == Subs::Ephem::BJD || ephem.get_tscale() == Subs::Ephem::BMJD){
      tcorr  = ut_date.tt()  + position.tcorr_bar(ut_date, tel)/Constants::DAY;
    }else if(ephem.get_tscale() == Subs::Ephem::BJD || ephem.get_tscale() == Subs::Ephem::HMJD){
      tcorr  = ut_date.mjd() + position.tcorr_hel(ut_date, tel);
    }else{
      throw Ultracam::Input_Error("Unrecognised ephemeris type");
    }

    const double MJD2JD = 2400000.5;
    if(ephem.get_tscale() == Subs::Ephem::BJD || ephem.get_tscale() == Subs::Ephem::HJD)
      tcorr += MJD2JD;

    double phase  = ephem.phase(tcorr);
    phase = phase - floor(phase);
    int nadd    = int(nbins*phase);
    double cosp = cos(Constants::TWOPI*phase);
    double sinp = sin(Constants::TWOPI*phase);

    // Add into sums
    sum    += 1.;
    sum_c  += cosp;
    sum_s  += sinp;
    sum_cc += cosp*cosp;
    sum_cs += cosp*sinp;
    sum_ss += sinp*sinp;

    // Add to appropriate frame.
    Ultracam::shift_and_add(constant,  data, shift_info, Ultracam::internal_data(1.),      shift_method);
    Ultracam::shift_and_add(cosine,    data, shift_info, Ultracam::internal_data(cosp),    shift_method);
    Ultracam::shift_and_add(sine,      data, shift_info, Ultracam::internal_data(sinp),    shift_method);
    if(nbins > 0)
      Ultracam::shift_and_add(bin[nadd], data, shift_info, Ultracam::internal_data(1.),      shift_method);

    // Update headers
    constant["folder.sum"]->set_value(sum);
    constant["folder.sum_c"]->set_value(sum_c);
    constant["folder.sum_s"]->set_value(sum_s);
    constant["folder.sum_cc"]->set_value(sum_cc);
    constant["folder.sum_cs"]->set_value(sum_cs);
    constant["folder.sum_ss"]->set_value(sum_ss);

    cosine["folder.sum"]->set_value(sum);
    cosine["folder.sum_c"]->set_value(sum_c);
    cosine["folder.sum_s"]->set_value(sum_s);
    cosine["folder.sum_cc"]->set_value(sum_cc);
    cosine["folder.sum_cs"]->set_value(sum_cs);
    cosine["folder.sum_ss"]->set_value(sum_ss);

    sine["folder.sum"]->set_value(sum);
    sine["folder.sum_c"]->set_value(sum_c);
    sine["folder.sum_s"]->set_value(sum_s);
    sine["folder.sum_cc"]->set_value(sum_cc);
    sine["folder.sum_cs"]->set_value(sum_cs);
    sine["folder.sum_ss"]->set_value(sum_ss);

    nsofar++;

    // Optional save
    if(nsave && (((source == 'S' || source == 'L') && nsofar % nsave == 0) ||
             (source == 'U' && (nfile+1) % nsave == 0 && file.size() > nfile + nsave))){

      // Save standard files
      constant.write("temp_"   + root + "_constant");
      cosine.write("temp_"  + root + "_cosine");
      sine.write("temp_"  + root + "_sine");
      for(size_t i=0; i<nbins; i++)
        bin[i].write(root + "_" + Subs::str(int(i+1),ndigit));

      // Get ready for LU decomposition
      Subs::Buffer2D<double> a(3,3);
      Subs::Buffer1D<size_t> indx(3);
      double d;
      a[0][0] = sum;
      a[0][1] = a[1][0] = sum_c;
      a[0][2] = a[2][0] = sum_s;
      a[1][1] = sum_cc;
      a[1][2] = a[2][1] = sum_cs;
      a[2][2] = sum_cc;

      try{
        Subs::ludcmp(a,indx,d);

        Subs::Buffer1D<double> b(3);
        for(size_t nccd=0; nccd<data.size(); nccd++){
          for(size_t nobj=0; nobj<data[nccd].size(); nobj++){
        const Ultracam::Windata& wcon = constant[nccd][nobj];
        const Ultracam::Windata& wcos = cosine[nccd][nobj];
        const Ultracam::Windata& wsin = sine[nccd][nobj];
        Ultracam::Windata& wamp = amp[nccd][nobj];
        Ultracam::Windata& wphs = phs[nccd][nobj];
        for(int iy=0; iy<data[nccd][nobj].ny(); iy++){
          for(int ix=0; ix<data[nccd][nobj].nx(); ix++){
            b[0] = wcon[iy][ix];
            b[1] = wcos[iy][ix];
            b[2] = wsin[iy][ix];
            Subs::lubksb(a,indx,b);
            wamp[iy][ix] = sqrt(Subs::sqr(b[1])+Subs::sqr(b[2]));
            wphs[iy][ix] = atan2(b[2],b[1])/Constants::TWOPI;
          }
        }
          }
        }

        amp.write("temp_"   + root + "_amplitude");
        phs.write("temp_"   + root + "_phase");

      }
      catch(const Subs::Subs_Error& err){
        std::cerr << err << std::endl;
      }
      std::cout << "Temporary files just saved." << std::endl;
    }
      }
    }

    // Save files to disk
    std::cout << "Saving " << nbins+5 << " files to disk" << std::endl;
    constant.write(root + "_constant");
    cosine.write(root + "_cosine");
    sine.write(root + "_sine");
    for(size_t i=0; i<nbins; i++)
      bin[i].write(root + "_" + Subs::str(int(i+1),ndigit));

    // Get ready for LU decomposition
    Subs::Buffer2D<double> a(3,3);
    Subs::Buffer1D<size_t> indx(3);
    double d;
    a[0][0] = sum;
    a[0][1] = a[1][0] = sum_c;
    a[0][2] = a[2][0] = sum_s;
    a[1][1] = sum_cc;
    a[1][2] = a[2][1] = sum_cs;
    a[2][2] = sum_cc;

    try{
      Subs::ludcmp(a,indx,d);

      Subs::Buffer1D<double> b(3);
      for(size_t nccd=0; nccd<data.size(); nccd++){
    for(size_t nobj=0; nobj<data[nccd].size(); nobj++){
      const Ultracam::Windata& wcon = constant[nccd][nobj];
      const Ultracam::Windata& wcos = cosine[nccd][nobj];
      const Ultracam::Windata& wsin = sine[nccd][nobj];
      Ultracam::Windata& wamp = amp[nccd][nobj];
      Ultracam::Windata& wphs = phs[nccd][nobj];
      for(int iy=0; iy<data[nccd][nobj].ny(); iy++){
        for(int ix=0; ix<data[nccd][nobj].nx(); ix++){
          b[0] = wcon[iy][ix];
          b[1] = wcos[iy][ix];
          b[2] = wsin[iy][ix];
          Subs::lubksb(a,indx,b);
          wamp[iy][ix] = sqrt(Subs::sqr(b[1])+Subs::sqr(b[2]));
          wphs[iy][ix] = atan2(b[2],b[1])/Constants::TWOPI;
        }
      }
    }
      }

      amp.write(root + "_amplitude");
      phs.write(root + "_phase");

    }
    catch(const Subs::Subs_Error& err){
      std::cerr << err << std::endl;
    }
  }

  // Handle errors
  catch(const Input_Error& err){
    std::cerr << "\nUltracam::Input_Error:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const File_Open_Error& err){
    std::cerr << "\nUltracam::File_Open_error:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const Ultracam_Error& err){
    std::cerr << "\nUltracam::Ultracam_Error:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const Subs::Subs_Error& err){
    std::cerr << "\nSubs::Subs_Error:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const std::string& err){
    std::cerr << "\n" << err << std::endl;
  }
}



