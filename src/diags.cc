/*

!!begin
!!title    diags
!!author   T.R. Marsh
!!created  25 June 2009
!!root     diags
!!index    diags
!!descr    prints out diagnostic stats from raw data files or ucm lists, one line per frame
!!class    Observing
!!class    IO
!!css      style.css
!!head1    prints out diagnostic stats from raw data files or ucm lists, one line per frame

!!emph{diags} is to help with working out what a given run contains in terms of the type of data.
To this end it generates a large number of statistics for each input frame. The stats it generates are
as follows: minimum pixel value, percentiles (as fractions): 0.001, 0.01, 0.1, 0.5, 0.9, 0.99, 0.999, 
maximum, mean, rms. 

!!head2 Invocation

diag [source] (url)/(file) ndigit first (last) trim [(ncol nrow) twait tmax] skip
bias (biasframe) (threshold (photon) naccum) 

!!head2 Arguments

!!table

!!arg{source}{Data source, either 'l' for local or 's' for server. 'Local' means the usual .xml and .dat
files accessed directly. Do not add either .xml or .dat to the file name; these are assumed.}

!!arg{url/file}{If source='S', this should be the complete URL of the file, e.g. 'http://127.0.0.1:8007/run00000012',
or just the file part in which case the program will try to find a default part to
add from the environment variable ULTRACAM_DEFAULT_URL. Failing this it will add
http://127.0.0.1:8007/, i.e. the local host. If source='L', this should just be a plain file name.}

!!arg{ndigit}{The minimum number of digits to use on output to tack on the frame number. If ndigit=0, then 
just enough will be used for each frame, however specifying a number > 0 has the advantage that it is 
then much easier to get a listing of the files in temporal order since this is also alphabetical.
e.g. you will end up with files of the form run0012_001, run0012_002, etc if you set ndigit=3.}

!!arg{first}{The first file, starting from 1. Negative numbers just grab one specific file, e.g. -10
grabs the 10th. 0 grabs the last.}

!!arg{last}{The last file, if first > 0. last=0 will just try to grab as many as possible, otherwise it
should be >= first}

!!arg{trim}{Set trim=true to enable trimming of potential junk rows and columns of each window}

!!arg{ncol}{If trim, then this specifies the number of columns nearest the readouts of each window to be snipped
off as these can be corrupted.}

!!arg{nrow}{If trim, then this specifies the number of rows to snip off the bottom of each window as these 
can be corrupted.}

!!arg{twait}{Time to wait between attempts to find a new exposure (seconds).}

!!arg{tmax}{Maximum time to wait before giving up (seconds). Set = 0 to quit as soon as a frame is
not found.}

!!arg{skip}{true to skip junk data at start of drift mode runs}

!!arg{bias}{true/false according to whether you want to subtract a bias frame. You can specify a full-frame 
bias because it will be cropped to match whatever your format is. This is useful for ultracam because of
the different bias levels of the 6 readouts. The exposure time of the bias will be inserted into the headers
of the output frames as it is needed for later dark subtraction.}

!!arg{biasframe}{If bias, then you need to specify the name of the bias frame.}

!!table

!!end

*/

#include <cstdlib>
#include <climits>
#include <cfloat>
#include <string>
#include <fstream>
#include "trm_subs.h"
#include "trm_format.h"
#include "trm_input.h"
#include "trm_header.h"
#include "trm_frame.h"
#include "trm_mccd.h"
#include "trm_ultracam.h"

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
	input.sign_in("last",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("trim",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("ncol",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("nrow",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("twait",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("tmax",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
	input.sign_in("skip",      Subs::Input::LOCAL,  Subs::Input::NOPROMPT);
	input.sign_in("bias",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("biasframe", Subs::Input::GLOBAL, Subs::Input::PROMPT);


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
	std::string short_url = url;
	int first;
	input.get_value("first", first, 1, INT_MIN, INT_MAX, "first file to access");
	int last;
	if(first > 0){
	    input.get_value("last", last, 0, 0, INT_MAX, "last file to access (0 for all)");
	    if(last != 0 && last < first)
		throw Ultracam_Error("Last file must either be 0 or >= first");
	}
	bool trim;
	input.get_value("trim", trim, true, "trim junk lower rows from windows?");
	int ncol, nrow;
	if(trim){
	    input.get_value("ncol", ncol, 0, 0, 100, "number of columns to trim from each window");
	    input.get_value("nrow", nrow, 0, 0, 100, "number of rows to trim from each window");
	}
	double twait;
	input.get_value("twait", twait, 1., 0., 1000., "time to wait between attempts to find a frame (seconds)");
	double tmax;
	input.get_value("tmax", tmax, 2., 0., 100000., "maximum time to wait before giving up trying to find a frame (seconds)");
	bool skip;
	input.get_value("skip", skip, true, "skip junk data at start of drift mode runs?");

	std::cout << "Attempting to access " << url << "\n" << std::endl;

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
	parseXML(source, url, mwindow, header, serverdata, trim, ncol, nrow, twait, tmax);

	Ultracam::Frame data(mwindow, header);

	bool bias;
	input.get_value("bias", bias, true, "do you want to subtract a bias frame from the grabbed data?");
	Ultracam::Frame bias_frame;
	if(bias){
	    std::string sbias;
	    input.get_value("biasframe", sbias, "bias", "name of bias frame");
	    bias_frame.read(sbias);
	    bias_frame.crop(mwindow);
	
	    // We need to record this in the frame for potential dark subtraction
	    float bias_expose = bias_frame["Exposure"]->get_float();
	    data.set("Bias_exposure", new Subs::Hfloat(bias_expose, "Exposure time of bias subtracted from this frame"));
	
	}
    
	input.save();

	std::string::size_type n = url.find_last_of('/');
	std::string server_file, out_file;
	if(n != std::string::npos){
	    server_file = url.substr(n+1);
	}else{
	    server_file = url;
	}
	size_t nfile = first < 0 ? -first : first;
	Subs::Format form(6);

	// info line
	std::cout << "#" << std::endl; 
	std::cout << "# file frame rawmean rawrms mean rms nrej nccd*(min 0.0001 0.001 0.01 0.1 0.3 0.5 0.7 0.9 0.99 0.999 0.9999 max)" << std::endl; 

	Subs::Array1D<float> buffer;
	for(;;){

	    // Carry on reading until data are OK
	    bool get_ok;
	    for(;;){
		if(!(get_ok = Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax))) break;
		if(serverdata.is_junk(nfile)){
		    if(skip){
			std::cerr << "Skipping file " << nfile << " which has junk data" << std::endl;
			nfile++;
		    }else{
			std::cerr << "File " << nfile << " has junk data but will still be written to disk" << std::endl;
			break;
		    }
		}else{
		    break;
		}
	    }
	    if(!get_ok) break;

	    // Subtract a bias frame
	    if(bias) data -= bias_frame;
      
	    // Compute buffer size
	    int ntot = 0;
	    for(size_t iw=0; iw<data[0].size(); iw++)
		ntot += data[0][iw].ntot();

	    buffer.resize(ntot);

	    // Start print out
	    std::cout << short_url << " " << nfile;
	    for(size_t ic=0; ic<data.size(); ic++){
		// Get all data for the CCD out.
		for(size_t iw=0, ip=0; iw<data[ic].size(); iw++)
		    for(int iy=0; iy<data[ic][iw].ny(); iy++)
			for(int ix=0; ix<data[ic][iw].nx(); ix++, ip++)
			    buffer[ip] = data[ic][iw][iy][ix];
		
		// Sort into ascending order (discarding key)
		buffer.sort();

		double rawmean, rawrms, mean, rms;
		int nrej;
		Subs::sigma_reject(buffer, buffer.size(), 3.0, true, 
				   rawmean, rawrms, mean, rms, nrej);
		std::cout << " " << ic+1 << " " << rawmean << " " << rawrms << " " << mean << " " << rms << " " 
			  << nrej << " " << buffer[0] << " " << buffer[int(0.0001*(ntot-1)+0.5)] << " " 
			  << buffer[int(0.001*(ntot-1)+0.5)] << " " << buffer[int(0.01*(ntot-1)+0.5)] << " " 
			  << buffer[int(0.1*(ntot-1)+0.5)] << " " << buffer[int(0.3*(ntot-1)+0.5)] << " " 
			  << buffer[int(0.5*(ntot-1)+0.5)] << " " << buffer[int(0.7*(ntot-1)+0.5)] << " " 
			  << buffer[int(0.9*(ntot-1)+0.5)] << " " << buffer[int(0.99*(ntot-1)+0.5)] << " " 
			  << buffer[int(0.999*(ntot-1)+0.5)] << " " << buffer[int(0.9999*(ntot-1)+0.5)] << " " 
			  << buffer[ntot-1] << " " << buffer[int(0.9999*(ntot-1)+0.5)];
		    

	    }
	    std::cout << std::endl;

	    if(first < 0 || (last > 0 && int(nfile) >= last)) break;
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



