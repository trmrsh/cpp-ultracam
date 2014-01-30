/*

!!begin
!!title    oneline
!!author   T.R. Marsh
!!created  23 Sep 2002
!!root     oneline
!!index    oneline
!!descr    prints one line of useful info from server or local .dat and .xml files
!!class    Reduction
!!class    Observing
!!class    Information
!!css      style.css
!!head1    oneline - prints one line of useful info from server or local .dat and .xml files

!!emph{oneline} provides some basic header info based upon the .xml and .dat files. It prints
out the following information: the name of the file, the number of frames, the start and end times,
the exposure time (there is bug here for drift mode), the binning factors and window formats.


!!head2 Invocation

oneline [source] (url)/(file) [twait tmax]

!!head2 Arguments

!!table

!!arg{source}{Data source, either 'l' for local or 's' for server. 'Local' means the usual .xml and .dat
files accessed directly. Do not add either .xml or .dat to the file name; these are assumed.}

!!arg{url/file}{If source='S', this should be the complete URL of the file, e.g. 'http://127.0.0.1:8007/run00000012',
or just the file part in which case the program will try to find a default part to
add from the environment variable ULTRACAM_DEFAULT_URL. Failing this it will add
http://127.0.0.1:8007/, i.e. the local host. If source='L', this should just be a plain file name.}

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
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/header.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/window.h"
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
    input.sign_in("source", Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
    input.sign_in("url",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("file",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
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
    double twait;
    input.get_value("twait", twait, 1., 0., 1000., "time to wait between attempts to find a frame (seconds)");
    double tmax;
    input.get_value("tmax", tmax, 2., 0., 100000., "maximum time to wait before giving up trying to find a frame (seconds)");
    input.save();

    std::string name = url;
    std::cout << name << "                 ";

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

    Ultracam::Mwindow mwindow;
    Subs::Header header;
    Ultracam::ServerData serverdata;
    parseXML(source, url, mwindow, header, serverdata, false, 0, 0, twait, tmax);

    Ultracam::Frame data(mwindow, header);

    std::string::size_type n = url.find_last_of('/');
    std::string server_file, out_file;
    if(n != std::string::npos)
      server_file = url.substr(n+1);
    else
      server_file = url;

    // Determine total number of frames so far
    size_t nfile = 0;

    if(!Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax) && nfile > 0)
      throw Ultracam_Error("failed to determine the number of frames.");

    if(nfile == 0)
      throw Ultracam_Error("no complete frames were found.");

    size_t numfiles = nfile;

    Subs::Time first_time, last_time;
    float exposure;
    bool single = true;

    if(serverdata.readout_mode == Ultracam::ServerData::DRIFT){

      // Read enough frames to get a good time at start
      size_t nwins = int((1033./serverdata.window[0].ny/serverdata.ybin+1.)/2.);
      if(nwins >= numfiles)
    throw Ultracam_Error(name + std::string(": drift mode with no good data!"));

      for(size_t nf=1; nf<=nwins; nf++)
    if(!Ultracam::get_server_frame(source, url, data, serverdata, nf, twait, tmax))
      throw Ultracam_Error(name + std::string(": failed to read first good frame of drift mode."));

      first_time  = data["UT_date"]->get_time();
      exposure    = data["Exposure"]->get_float();

      // Read enough frames to get a good time at end
      for(size_t nf=nfile-nwins; nf<=numfiles; nf++)
    if(!Ultracam::get_server_frame(source, url, data, serverdata, nf, twait, tmax))
      throw Ultracam_Error(name + std::string(": failed to read last file (1)."));

      last_time  = data["UT_date"]->get_time();
      numfiles -= nwins;
      single = false;

    }else{

      // Read first frame time
      if(nfile > 1){
    nfile = 1;
    if(!Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax))
      throw Ultracam_Error(name + std::string(": no OK data found (1)"));
      }

      first_time  = data["UT_date"]->get_time();

      if(numfiles > 2){

    // Read frames 2 and 3 to get a good exposure
    for(size_t nf=2; nf<=3; nf++)
      if(!Ultracam::get_server_frame(source, url, data, serverdata, nf, twait, tmax))
        throw Ultracam_Error(name + std::string(": no OK data found (2)"));

    exposure    = data["Exposure"]->get_float();

    // Read last two frames to get a good time at end
    for(size_t nf=numfiles-1; nf<=numfiles; nf++)
      if(!Ultracam::get_server_frame(source, url, data, serverdata, nf, twait, tmax))
        throw Ultracam_Error(name + std::string(": failed to read last file (2)."));

    last_time  = data["UT_date"]->get_time();

    single = false;

      }else if(numfiles == 2){

    exposure    = data["Exposure"]->get_float();

    // Read last two frames to get a good time at end
    for(size_t nf=numfiles-1; nf<=numfiles; nf++)
      if(!Ultracam::get_server_frame(source, url, data, serverdata, nf, twait, tmax))
        throw Ultracam_Error(name + std::string(": failed to read last file (3)."));

    last_time  = data["UT_date"]->get_time();

    single = false;

      }else{

    exposure    = data["Exposure"]->get_float();

      }
    }

    std::cout << std::setw(6) << numfiles;
    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    if(single){
      std::cout << "  " << first_time << "  " << first_time << "  " << std::setfill(' ') << std::setprecision(3) << std::setw(6) << exposure;
    }else{
      std::cout << "  " << first_time << "  " << last_time  << "  " << std::setfill(' ') << std::setprecision(3) << std::setw(6) << exposure;
    }
    std::cout << "  " << data[0][0].xbin() << " " << data[0][0].ybin();
    std::cout << "  " << data[0].size() << " ";
    for(size_t io=0; io<data[0].size(); io++)
      std::cout << " ["
       << data[0][io].llx() << "," << data[0][io].lly() << ","
       << data[0][io].nx() << "," << data[0][io].ny() << "]";
    std::cout << std::endl;
  }

  // Handle errors

  catch(const std::string& err){
    std::cerr << err << std::endl;
    exit(EXIT_FAILURE);
  }
}



