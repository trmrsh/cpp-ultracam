/*

!!begin
!!title   Plots ultracam files plus light curve
!!author  T.R. Marsh
!!created 25 Sep 2002
!!revised 15 June 2004
!!root    movie
!!index   movie
!!descr   plots an ultracam images plus light curve
!!css   style.css
!!class   Programs
!!class   Display
!!head1   movie - plots an ultracam images plus light curve

!!emph{movie} generates stills of a movie showing a CCD and a light curve together. It has lots of parameters
and is relatively fiddly to set up, so you may want to make a little script for future reference.

!!head2 Invocation

movie [device source] width aspect ((url)/(file) first trim [(ncol nrow) twait tmax])/(flist) 
nccd bias (biasframe) xleft xright ylow yhigh iset (ilow ihigh)/(plow phigh) lcurve targ comp scale x1 x2 y1 y2
skip [fraction csize lwidth cfont pause]

!!head2 Command line arguments

!!table

!!arg{device}{Display device. If identified as a gif, this will generate a separate one for each image
so that these can later be merged into a single animated gif}

!!arg{source}{Data source, either 'l' for local, 's' for server or 'u' for a list of ucm files. 
'Local' means the usual .xml and .dat files accessed directly. Do not add either .xml or .dat 
to the file name; these are assumed.}

!!arg{width}{Width of plots, in cm}

!!arg{aspect}{Aspect ratio of plots, height/width}

!!arg{url/file}{If source='S', this should be the complete URL of the
file, e.g.  'http://127.0.0.1:8007/run00000012', or just the file part
in which case the program will try to find a default part to add from
the environment variable ULTRACAM_DEFAULT_URL. Failing this it will
add http://127.0.0.1:8007/, i.e. the local host. If source='L', this
should just be a plain file name.}

!!arg{first}{If source='L' or 'S', 'first' sets which exposure to start with (starts at 1).}

!!arg{trim}{If source='L' or 'S', set trim=true to enable trimming of potential junk rows and
columns of each window}

!!arg{ncol}{If trim, then this specifies the number of columns nearest the readouts of each window to be snipped
off as these can be corrupted.}

!!arg{nrow}{If trim, then this specifies the number of rows to snip off the bottom of each window as these 
can be corrupted.}

!!arg{twait}{Time to wait between attempts to find a new exposure. (Only for data from a server)}

!!arg{tmax}{Maximum time to wait before giving up, set = 0 to give up immediately. (Only for data from
a server).}

!!arg{flist}{If source = 'U', this is the name of a list of ULTRACAM files to reduce. These should be arranged in
temporal order to help the reduction move from one exposure to the next successfully.}

!!arg{nccd}{The particular CCD to display}

!!arg{bias}{true/false according to whether you want to subtract a bias frame. You can specify a full-frame 
bias because it will be cropped to match whatever your format is. This is useful for ultracam because of
the different bias levels of the 6 readouts.}

!!arg{biasframe}{If bias, then you need to specify the name of the bias frame.}

!!arg{xleft xright}{X range to plot}

!!arg{ylow yhigh}{Y range to plot}

!!arg{iset}{'A', 'D' or 'P' according to whether you want to set the intensity limits
automatically (= min to max), directly or with percentiles.}

!!arg{ilow ihigh}{If iset='d', ilow and ihigh specify the intensity range to plot}

!!arg{plow phigh}{If iset='p', plow and phigh are percentiles to set the intensity range,
e.g. 10, 99} 

!!arg{lcurve}{Equivalent List of Ultracam files}

!!arg{targ}{Target aperture}

!!arg{comp}{Comparison star aperture}

!!arg{scale}{Scaling factor to divide into target/comparison}

!!arg{x1}{Lower X limit of light curve plot. This is measured after the subtracting the largest integer less than the 
first time encountered.}

!!arg{x2}{Upper X limit of light curve plot}

!!arg{y1}{Lower Y limit of plot}

!!arg{y2}{Upper Y limit of plot}

!!arg{skip}{Number of frames to skip between plots, to reduce overall number of images produced. 
Starts from 0}

!!arg{fraction}{Fraction of X range to devote to the images}

!!arg{csize}{Character size}

!!arg{lwidth}{Line width}

!!arg{cfont}{Character font}

!!arg{pause}{Number of seconds to pause between plots. With '/xs' this seems necessary to prevent
multiple windows being opened. 0.01 seems to work in this case.}

!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <climits>
#include <string>
#include <fstream>
#include <map>
#include "cpgplot.h"
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_plot.h"
#include "trm_header.h"
#include "trm_mccd.h"
#include "trm_frame.h"
#include "trm_window.h"
#include "trm_ultracam.h"

// Structure for light curve
struct Ldata {
  Ldata() : t(0.f), y(0.f), e(0.f) {}
  Ldata(float time, float yval, float error) : t(time), y(yval), e(error) {}
  float t;
  float y;
  float e;
};

// Main program
int main(int argc, char* argv[]){
  
  using Ultracam::File_Open_Error;
  using Ultracam::Input_Error;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables
    input.sign_in("device",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("source",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("width",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("aspect",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("url",       Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("file",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("first",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("trim",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ncol",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("nrow",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("twait",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("tmax",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("flist",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("nccd",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("bias",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("biasframe", Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("xleft",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("xright",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ylow",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("yhigh",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("iset",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ilow",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ihigh",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("plow",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("phigh",     Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("lcurve",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("targ",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("comp",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("scale",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("x1",        Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("x2",        Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("y1",        Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("y2",        Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("skip",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("fraction",  Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("csize",     Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("lwidth",    Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("cfont",     Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
    input.sign_in("pause",     Subs::Input::LOCAL,  Subs::Input::NOPROMPT);

    // Get inputs
    std::string device;
    input.get_value("device", device, "/xs", "plot device");
    char source;
    input.get_value("source", source, 'S', "uUsSlL", "data source: L(ocal), S(erver) or U(cm)?");
    source = toupper(source);

    float width;
    input.get_value("width", width, 0.f, 0.f, 100.f, "width of plots in centimetres");
    float aspect;
    input.get_value("aspect", aspect, 0.618f, 0.001f, 1000.f, "aspect ratio of plots (height/width)");

    std::string url;
    if(source == 'S'){
      input.get_value("url", url, "url", "url of file");
    }else if(source == 'L'){
      input.get_value("file", url, "file", "name of local file");
    }
    size_t first, nfile;
    bool trim;
    int ncol, nrow;
    std::vector<std::string> file;
    double twait, tmax;
    Ultracam::Mwindow mwindow;
    Subs::Header header;
    Ultracam::ServerData serverdata;
    Ultracam::Frame data;

    if(source == 'S' || source == 'L'){

      input.get_value("first", first, size_t(1), size_t(1), size_t(INT_MAX), "first file to access");
      input.get_value("trim", trim, true, "trim junk lower rows from windows?");
      if(trim){
	input.get_value("ncol", ncol, 0, 0, 100, "number of columns to trim from each window");
	input.get_value("nrow", nrow, 0, 0, 100, "number of rows to trim from each window");
      }
      input.get_value("twait", twait, 1., 0., 1000., "time to wait between attempts to find a frame (seconds)");
      input.get_value("tmax", tmax, 2., 0., 1000., "maximum time to wait before giving up trying to find a "
		      "frame (seconds)");

      // Add extra stuff to URL if need be.
      if(url.find("http://") == std::string::npos && source == 'S'){
	char *DEFAULT_URL = getenv(Ultracam::ULTRACAM_DEFAULT_URL);
	if(DEFAULT_URL != NULL){
	  url = DEFAULT_URL + url;
	}else{
	  url = Ultracam::ULTRACAM_LOCAL_URL + url;
	}
      }else if(url.find("http://") == 0 && source == 'L'){
	throw Ultracam::Input_Error("Should not specify the local file as a URL");
      }
      
      // Parse the XML file
      Ultracam::parseXML(source, url, mwindow, header, serverdata, trim, ncol, nrow, twait, tmax);
      
      // Initialise standard data frame
      data.format(mwindow, header);

      nfile = first;

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

      nfile = first = 0;

    }

    // Carry on getting inputs
    int nccd;
    input.get_value("nccd", nccd, int(1), int(1), int(data.size()), "CCD number to plot");
    nccd--;

    bool bias;
    input.get_value("bias", bias, true, "do you want to subtract a bias frame before plotting?");
    Ultracam::Frame bias_frame;
    if(bias){
      std::string sbias;
      input.get_value("biasframe", sbias, "bias", "name of bias frame");
      bias_frame.read(sbias);
      bias_frame.crop(mwindow);
    }

    float x1, x2, y1, y2;
    x2 = data[nccd].nxtot()+0.5;
    y2 = data[nccd].nytot()+0.5;

    input.get_value("xleft",  x1, 0.5f, 0.5f, x2, "left X limit of plot");
    input.get_value("xright", x2, x2,   0.5f, x2, "right X limit of plot");
    input.get_value("ylow",   y1, 0.5f, 0.5f, y2, "lower Y limit of plot");
    input.get_value("yhigh",  y2, y2, 0.5f, y2, "upper Y limit of plot");

    char iset;
    input.get_value("iset", iset, 'a', "aAdDpP", "set intensity a(utomatically), d(irectly) or with p(ercentiles)?");
    iset = toupper(iset);
    float ilow, ihigh, plow, phigh;
    if(iset == 'D'){
      input.get_value("ilow",   ilow,  0.f, -FLT_MAX, FLT_MAX, "lower intensity limit");
      input.get_value("ihigh",  ihigh, 1000.f, -FLT_MAX, FLT_MAX, "upper intensity limit");
    }else if(iset == 'P'){
      input.get_value("plow",   plow,  1.f, 0.f, 100.f,  "lower intensity limit percentile");
      input.get_value("phigh",  phigh, 99.f, 0.f, 100.f, "upper intensity limit percentile");
      plow  /= 100.;
      phigh /= 100.;
    }

    std::string lcurve;
    input.get_value("lcurve", lcurve, "light.log", "name of light curve file from reduce");
    int targ;
    input.get_value("targ",  targ, 1, 1, 1000, "target star aperture number");
    int comp;
    input.get_value("comp",  comp, 2, 1, 1000, "comparison star aperture number");
    if(targ == comp)
      throw Input_Error("Can't have target the same as the comparison aperture");
    float scale;
    input.get_value("scale",  scale, 1.f, FLT_MIN, FLT_MAX, "factor to normalise the light curve by");
    float x1_light;
    input.get_value("x1", x1_light, 0.f, -FLT_MAX, FLT_MAX, "left limit of light curve plot (from start of run in days)");
    float x2_light;
    input.get_value("x2", x2_light, 0.1f, -FLT_MAX, FLT_MAX, "right limit of light curve plot (from start of run in days)");
    if(x1_light == x2_light)
      throw Input_Error("Cannot have std::left and right plot limits the same");
    float y1_light;
    input.get_value("y1", y1_light, 0.f, -FLT_MAX, FLT_MAX, "lower limit of light curve plot");
    float y2_light;
    input.get_value("y2", y2_light, 1.f, -FLT_MAX, FLT_MAX, "upper limit of light curve plot");
    if(y1_light == y2_light)
      throw Input_Error("Cannot have upper and lower plot limits the same");

    int skip;
    input.get_value("skip", skip, 0, 0, 100000000, "number of frames to skip between plots");
    float fraction;
    input.get_value("fraction", fraction, 0.4f, 0.f, 1.f, "fraction in X to devote to the image part");
    float csize;
    input.get_value("csize", csize, 1.5f, 0.f, 100.f, "charcater size for plots");
    int lwidth;
    input.get_value("lwidth", lwidth, 2, 0, 100, "line width for plots");
    int cfont;
    input.get_value("cfont", cfont, 2, 1, 4, "character font");
    double pause;
    input.get_value("pause", pause, 0.01, 0., 100., "pause between plots (seconds)");

    // Read in light curve data, checking that some of it is visible in the plot
    bool visible = false;
    std::ifstream fin(lcurve.c_str());
    if(!fin)
      throw Input_Error("Failed to open file = " + lcurve);

    // First read the data into a map, keyed by frame number
    std::map<size_t,Ldata*> ldata;
    std::string line;
    char c;
    size_t nfmax = 0;
    double t0 = 0;
    while(fin && (c = fin.peek(), fin) && !fin.eof()){
      if(c == '#'){
	fin.ignore(10000, '\n');
      }else{

	size_t nframe;
	double time;
	int flag, nsat;
	float expose;
	int ccd;
	float fwhm, beta;
	fin >> nframe >> time >> flag >> nsat >> expose >> ccd >> fwhm >> beta;
	if(fin){
	  if(ccd == nccd+1){
	    int nape = 0, nrej, error_flag, nsky, worst;
	    float counts, sigma, sky;
	    double xf, yf, xm, ym, ex, ey;
	    bool tok = false, cok = false;
	    float ty = 0, te = 0, cy = 0, ce = 0;
	    for(int nap=0; nap<std::max(targ, comp); nap++){
	      fin >> nape >> xf >> yf >> xm >> ym >> ex >> ey >> counts >> sigma >> sky >> nsky >> nrej >> worst >> error_flag;
	      if(!fin) throw Input_Error("Error reading light curve file " + lcurve  + "\nline: " + line);
	      if(nape == targ){
		ty  = counts;
		te  = sigma;
		tok = true;
	      }else if(nape == comp){
		cy  = counts;
		ce  = sigma;
		cok = true;
	      }
	    }
	    if(tok && cok){
	      float y  = ty / cy;
	      float ye = sqrt(Subs::sqr(te) + Subs::sqr(y*ce)) /cy / scale;
	      y /= scale;
	      if(ldata.size() == 0) t0 = floor(time);
	      float x = time - t0;
	      ldata[nframe] = new Ldata(x, y, ye);
	      if(!visible && x > std::min(x1_light, x2_light) && x < std::max(x1_light, x2_light)
		 && y > std::min(y1_light, y2_light) && y < std::max(y1_light, y2_light)) visible = true;
	      nfmax = nframe > nfmax ? nframe : nfmax;
	    }
	  }
	  fin.ignore(10000, '\n');
	}
      }
    }
    fin.close();
    if(ldata.size() == 0)
      throw Input_Error("No points loaded from light curve file");
    std::cout << ldata.size() << " points loaded from light curve file." << std::endl;
    if(!visible)
      throw Input_Error("None of the loaded points will be visible in the light curve plot");

    // Compute number of digits to use in file names
    int ndigit = int(log10(float(nfmax+1))+1);

    // Save defaults now because one often wants to terminate this program early
    input.save();

    // Break down plot device specification
    std::string::size_type loc = device.find_last_of('/');
    if(loc == std::string::npos)
      throw Input_Error("Invalid device specification");

    std::string fdev;
    if(loc > 0)
      fdev = device.substr(0,loc-1);
    else
      fdev = "pgplot";
    std::string edev = device.substr(loc);

    Subs::Plot plot;

    std::cout << "\n";
    for(;;){

      if((nfile - first) % (skip + 1) == 0){

	if(source == 'S' || source == 'L'){
	  
	  if(!Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax)) break;
	  
	}else{
	  
	  if(nfile >= file.size()) break;
	  data.read(file[nfile]);
	  
	}
	
	// Subtract a bias frame
	if(bias) data -= bias_frame;
	
	// Open plot, set up style
	if(edev == "/xs" || edev == "/xw"){
	  plot.open(edev);
	}else{
	  plot.open(fdev + Subs::str(int(nfile),ndigit) + edev);
	}
	cpgpap(width/2.54, aspect);
	cpgsch(csize);
	cpgslw(lwidth);
	cpgscf(cfont);
	
	// Fix the viewport for the image to make the pixels square
	float yborder = 4.*csize/40.;   // border as fraction of Y height
	float xborder = yborder*aspect; // border as fraction of X width
	
	float xtv1=xborder, xtv2=fraction-xborder, ytv1=yborder, ytv2=1.-yborder;
	if(xtv1 >= xtv2 || ytv1 >= ytv2)
	  throw Input_Error("Invalid viewport limits (1): is character size too large?");
	cpgsvp(xtv1,xtv2,ytv1,ytv2);
	float xv1, xv2, yv1, yv2;
	cpgqvp(2,&xv1,&xv2,&yv1,&yv2);
	float pasp = (yv2-yv1)/(xv2-xv1);       // physical aspect ratio
	float rasp = (y2-y1)/(x2-x1);           // required aspect ratio
	float nasp = (ytv2-ytv1)/(xtv2-xtv1);   // normalised device coordinates aspect ratio
	if(rasp > pasp){
	  float midx   = (xtv1+xtv2)/2.;
	  float xrange = (ytv2-ytv1)*pasp/rasp/nasp; 
	  xtv1 = midx - xrange/2.;
	  xtv2 = midx + xrange/2.;
	}else{
	  float midy   = (ytv1+ytv2)/2.;
	  float yrange = (xtv2-xtv1)/pasp*rasp*nasp; 
	  ytv1 = midy - yrange/2.;
	  ytv2 = midy + yrange/2.;
	}
	if(xtv1 >= xtv2 || ytv1 >= ytv2)
	  throw Input_Error("Invalid viewport limits (2): is character size too large?");
	cpgsvp(xtv1,xtv2,ytv1,ytv2);
	cpgswin(x1,x2,y1,y2);
	
	// Turn plot region into a CCD of windows (with just 1 window)
	Ultracam::CCD<Ultracam::Window> window;
	int llx = std::max(int(1), std::min(data[0][0].nxtot(), int(std::min(x1,x2)+0.5)));
	int lly = std::max(int(1), std::min(data[0][0].nytot(), int(std::min(y1,y2)+0.5)));
	int nx  = std::min(data[0][0].nxtot()-llx+1, int(fabs(x2-x1)+0.5));
	int ny  = std::min(data[0][0].nytot()-lly+1, int(fabs(y2-y1)+0.5));
	window.push_back(Ultracam::Window(llx,lly,nx,ny,1,1,data[0][0].nxtot(),data[0][0].nytot()));
	
	// Compute intensity limits
	if(iset == 'P'){
	  data[nccd].centile(plow,phigh,ilow,ihigh,window);
	}else if(iset == 'A'){
	  ilow  = min(data[nccd],window);
	  ihigh = max(data[nccd],window);
	}
	
	// Plot
	cpgsci(Subs::WHITE);
	pggray(data[nccd],ihigh,ilow);
	cpgsci(Subs::BLUE);
	cpgbox("BCNST",0.,0,"BCNST",0.,0);
	cpgsci(Subs::WHITE);
	pgline(data[nccd]);
	pgptxt(data[nccd]);
	cpgsci(Subs::RED);
	cpglab("X pixels", "Y pixels", " ");
	std::cout << "Frame " << nfile << ", image plot range = " << ilow << " to " << ihigh << std::endl;
	
	// Light curve part
	xborder = yborder*aspect; // border as fraction of X width
	xtv1 = fraction+xborder;
	xtv2 = 1.-xborder;
	ytv1 = yborder;
	ytv2 = 1.-yborder;
	if(xtv1 >= xtv2 || ytv1 >= ytv2)
	  throw Input_Error("Invalid viewport limits (3): is character size too large?");
	cpgsvp(xtv1,xtv2,ytv1,ytv2);
	
	cpgsci(Subs::BLUE);
	cpgswin(x1_light,x2_light,y1_light,y2_light);
	cpgbox("BCNST",0.,0,"BCNST",0.,0);
	cpgsci(Subs::RED);
	std::ostringstream ostr;
	ostr << "MJD - " << t0;
	cpglab(ostr.str().c_str(), "Flux", " ");
	
	// Plot until current frame number. Assume increases sequentially.
	Ldata *ptr;
	for(std::map<size_t,Ldata*>::const_iterator cit=ldata.begin(); cit != ldata.end(); cit++){
	  if(cit->first <= nfile){
	    cpgsci(Subs::RED);
	    ptr = cit->second;
	    cpgmove(ptr->t, ptr->y - ptr->e);
	    cpgdraw(ptr->t, ptr->y + ptr->e);
	    cpgsci(Subs::WHITE);
	    cpgpt1(ptr->t, ptr->y, 1);
	  }else{
	    break;
	  }
	}
	
	plot.close();
	Subs::sleep(pause);
      }
      nfile++;
    }
  }

  // Handle errors

  catch(const Input_Error& err){
    std::cerr << "\nUltracam::Input_Error:" << std::endl;
    std::cerr << err << std::endl;
    exit(EXIT_FAILURE);
  }
  catch(const File_Open_Error& err){
    std::cerr << "\nUltracam::File_Open_error:" << std::endl;
    std::cerr << err << std::endl;
    exit(EXIT_FAILURE);
  }
  catch(const Ultracam::Ultracam_Error& err){
    std::cerr << "\nUltracam::Ultracam_Error:" << std::endl;
    std::cerr << err << std::endl;
    exit(EXIT_FAILURE);
  }
  catch(const Subs::Subs_Error& err){
    std::cerr << "\nSubs::Subs_Error:" << std::endl;
    std::cerr << err << std::endl;
    exit(EXIT_FAILURE);
  }
  catch(const std::string& err){
    std::cerr << "\n" << err << std::endl;
    exit(EXIT_FAILURE);
  }
}



