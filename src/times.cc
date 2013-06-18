/*

!!begin
!!title    times
!!author   T.R. Marsh
!!created  02 Dec 2004
!!revised  16 Mar 2006
!!root     times
!!index    times
!!descr    prints out timing information of runs
!!class    Testing
!!class    Information
!!css      style.css
!!head1    prints out timing information of runs

!!emph{times} is a routine to print out full timing information on a run. This
is designed to be of use to tracking down timing problems. The information
reported includes the raw GPS timestamp, the derived time and the exposure time.
Note that for drift mode the derived times will be shifted relative to the raw
stamps because the raw stamps apply to later frames. This routine is essentially
!!ref{grab.html}{grab} with the data writing removed. The times are UTC reported
in the form of MJD. The information reported comes in the following order: frame
number, raw GPS time, derived time of mid-exposure, exposure time. If the derived times
(3rd column) cannot be estimated as is the case for the first few frames of drift
mode and the first one or two in windows mode, 0 will be printed.
Some intitial header information is also reported, commented out with hashes.

!!head2 Invocation

times [source] (url)/(file) first last [clock twait tmax]

!!head2 Arguments

!!table

!!arg{source}{Data source, either 'l' for local or 's' for server. 'Local' means the usual .xml and .dat
files accessed directly. Do not add either .xml or .dat to the file name; these are assumed.}

!!arg{url/file}{If source='S', this should be the complete URL of the file, e.g. 'http://127.0.0.1:8007/run00000012',
or just the file part in which case the program will try to find a default part to
add from the environment variable ULTRACAM_DEFAULT_URL. Failing this it will add
http://127.0.0.1:8007/, i.e. the local host. If source='L', this should just be a plain file name.}

!!arg{first}{The first file, starting from 1.}

!!arg{clock}{The type of fix to apply to obtain the times. This is to correct for changes that occurred in the timestamping
owing to an upgrade in July 2003. This introduced a problem that was only spotted in Dec 2004. The way the times are
corrected had therefore to change. This parameter allows one either to use the default correction determined by the time. This
should always be the choice if one wants the correct times. The alternative is to use the reverse of the default correction
which is implemented so that one can judge the extent of the problem for a given run. For safety this parameter always reverts
to 'true' unless explicitly set false}

!!arg{last}{The last file, 0  means all of them}

!!arg{twait}{Time to wait between attempts to find a new exposure (seconds).}

!!arg{tmax}{Maximum time to wait before giving up (seconds). Set = 0 to quit as soon as a frame is
not found.}

!!table

!!end

*/

#include <cstdlib>
#include <climits>
#include <string>
#include <fstream>
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_header.h"
#include "trm_format.h"
#include "trm_mccd.h"
#include "trm_frame.h"
#include "trm_window.h"
#include "trm_ultracam.h"

// Main program

int main(int argc, char* argv[]){
  
  using Ultracam::File_Open_Error;
  using Ultracam::Ultracam_Error;
  using Ultracam::Input_Error;

  // Print the date/time only
  Subs::Hitem::pmode = 1;

  try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in input variables
    input.sign_in("source", Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("url",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("file",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("first",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("last",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("clock",  Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("twait",  Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("tmax",   Subs::Input::GLOBAL, Subs::Input::NOPROMPT);

    // Get inputs
    char source;
    input.get_value("source", source, 'S', "sSlL", "data source: L(ocal) or S(erver)?");
    source = toupper(source);
    std::string url;
    if(source == 'S'){
      input.get_value("url", url, "url", "url of file");
    }else{
      input.get_value("file", url, "file", "name of local file");
    }
    size_t first;
    input.get_value("first", first, size_t(1), size_t(1), size_t(INT_MAX), "first file to access (starting from 1)");
    size_t last;
    input.get_value("last",  last,  size_t(0), size_t(0), size_t(INT_MAX), "last file to access (0 to go to the end)");
    if(last !=0 && last < first)
      throw Ultracam::Ultracam_Error("last must either = 0 or be >= first");
    bool clock;
    input.set_default("clock", true);
    input.get_value("clock", clock, true, "use the default fix for the timestamps?");
    double twait;
    input.get_value("twait", twait, 1., 0., 1000., "time to wait between attempts to find a frame (seconds)");
    double tmax;
    input.get_value("tmax", tmax, 2., 0., 100000., "maximum time to wait before giving up trying to find a frame (seconds)");
    input.save();

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

    Subs::Format dform(16);
    dform.left();
    dform.width(18);

    // Parse the XML file      
    Ultracam::Mwindow mwindow;
    Subs::Header header;
    Ultracam::ServerData serverdata;
    serverdata.timestamp_default = clock;
    parseXML(source, url, mwindow, header, serverdata, false, 0, 0, twait, tmax);

    Ultracam::Frame data(mwindow, header);

    // Print some header info
    std::cout << "# " << std::endl;
    std::cout << "#                   File: " << url << std::endl;
    std::cout << "# " << std::endl;
    bool drift = false;

    int nwins, rflag = serverdata.readout_mode;
    switch(rflag){
    case Ultracam::ServerData::FULLFRAME_CLEAR:
      std::cout << "#           Readout mode: full frame with clear" << std::endl;
      break;
    case Ultracam::ServerData::FULLFRAME_NOCLEAR:
      std::cout << "#           Readout mode: full frame with no clear" << std::endl;
      break;
    case Ultracam::ServerData::FULLFRAME_OVERSCAN:
      std::cout << "#           Readout mode: full frame with overscan" << std::endl;
      break;
    case Ultracam::ServerData::WINDOWS:
      std::cout << "#           Readout mode: multiple window pairs with no clear" << std::endl;
      break;
    case Ultracam::ServerData::DRIFT:
      std::cout << "#           Readout mode: drift" << std::endl;
      nwins = int((1033./data[0][0].ny()/data[0][0].ybin()+1)/2.);
      std::cout << "#                  nwins: " << nwins << std::endl;
      drift = true;
      break;
    case Ultracam::ServerData::WINDOWS_CLEAR:
      std::cout << "#           Readout mode: multiple windows with clear" << std::endl;
      break;
    default:
      std::cout << "# Readout mode unrecognised" << std::endl;
    }

    std::cout << "# " << std::endl;
    std::cout << "#        Binning factors: " << data[0][0].xbin() << " " << data[0][0].ybin() << std::endl;
    std::cout << "#      Number of windows: " << data[0].size() << std::endl;
    std::cout << "#         Window formats: ";
    for(size_t io=0; io<data[0].size(); io++)
      std::cout << " [" << data[0][io].llx() << "," << data[0][io].lly() << "," << data[0][io].nx() << "," << data[0][io].ny() << "]";
    std::cout << "\n# " << std::endl;
    if(clock)
      std::cout << "# The default timestamp handler was used." << std::endl;
    else
      std::cout << "# The default timestamp handler was overridden." << std::endl;
    std::cout << "# " << std::endl;
    std::cout << "# Columns are: frame number, raw GPS time (MJD), deduced mid-exposure (MJD), deduction thought reliable or not," << std::endl;
    std::cout << "# exposure time, date/time of raw GPS time. | use as a separator." << std::endl;
    std::cout << "# " << std::endl;

    // Now onto frame by frame
    std::string::size_type n = url.find_last_of('/');
    std::string server_file, out_file;
    if(n != std::string::npos){
      server_file = url.substr(n+1);
    }else{
      server_file = url;
    }
    size_t nfile = first;
    int count = 0;
    for(;;){

      // Carry on reading until data are OK
      if(!Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax, false, false)) break;

      if(nfile == first){
	if(data["Frame.as_documented"]->get_bool())
	  std::cout << "# The timestamps were assumed to be standard.\n#" << std::endl;
	else
	  std::cout << "# The timestamps were assumed to be non-standard\n#" << std::endl;

	std::cout << "# " << std::endl;
      }

      // Report information
      double derived = data["UT_date"]->get_double();

      std::cout << std::setw(7) << data["Frame.frame_number"]->get_int() << " | " << dform(data["Frame.GPS_time"]->get_double()) 
	   << " | " << dform(derived) << " | " << data["Frame.reliable"] << " | " << data["Exposure"]->get_float() 
	   << " | " << data["Frame.GPS_time"] << "\n";
      count++;
      if(count % 10 == 0) std::cout << std::flush;
      if(last != 0 && nfile == last) break;
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
  catch(const Ultracam_Error& err){
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



