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
To this end it generates a large number of statistics for each input frame. These consist of things
like means, medians, RMS values and various percentiles. For ULTRACAM data it applies an offset to 
the windows from the right-hand side of the frames to make them match the clipped mean of the left-hand
side windows. The offset used is reported; for ULTRASPEC the offset is returned as 0.

!!head2 Invocation

diags [source] (url)/(file) first (last) trim [(ncol nrow) twait tmax] skip 

!!head2 Arguments

!!table

!!arg{source}{Data source, either 'l' for local or 's' for server. 'Local' means the usual .xml and .dat
files accessed directly. Do not add either .xml or .dat to the file name; these are assumed.}

!!arg{url/file}{If source='S', this should be the complete URL of the file, e.g. 'http://127.0.0.1:8007/run00000012',
or just the file part in which case the program will try to find a default part to
add from the environment variable ULTRACAM_DEFAULT_URL. Failing this it will add
http://127.0.0.1:8007/, i.e. the local host. If source='L', this should just be a plain file name.}

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

	std::cout << "# Attempting to access " << url << "\n" << std::endl;

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

	Subs::Header::Hnode *hnode = data.find("Instrument.instrument");
	bool ultraspec = (hnode->has_data() && hnode->value->get_string() == "ULTRASPEC"); 

	input.save();

	std::string::size_type n = url.find_last_of('/');
	std::string server_file, out_file;
	if(n != std::string::npos){
	    server_file = url.substr(n+1);
	}else{
	    server_file = url;
	}
	size_t nfile = first < 0 ? -first : first;

	// info line
	std::cout << "#" << std::endl; 
	std::cout << "# file frame NCCD*(nccd offset rawmean rawrms mean rms nrej min 0.0001 0.001 0.01 0.1 0.3 0.5 0.7 0.9 0.99 0.999 0.9999 max)" << std::endl; 

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

	    // Compute buffer size
	    int ntot = 0;
	    for(size_t iw=0; iw<data[0].size(); iw++)
		ntot += data[0][iw].ntot();

	    buffer.resize(ntot);

	    // Start print out
	    std::cout << short_url << " " << nfile;
	    for(size_t ic=0; ic<data.size(); ic++){

		// Get all data for the CCD out.
		if(ultraspec){
		    for(size_t iw=0, ip=0; iw<data[ic].size(); iw++)
			for(int iy=0; iy<data[ic][iw].ny(); iy++)
			    for(int ix=0; ix<data[ic][iw].nx(); ix++, ip++)
				buffer[ip] = data[ic][iw][iy][ix];
		    std::cout << " " << ic+1 << " " << 0.;
		}else{

		    // ULTRACAM uses different outputs for left and right-hand windows.
		    // Here we compute an offset to add to the right-hand side data to make them
		    // match otherwise many of the stats are of little use, especially the RMS
		    // values.

		    // left-hand windows 
		    size_t ip=0;
		    for(size_t iw=0; iw<data[ic].size(); iw+=2)
			for(int iy=0; iy<data[ic][iw].ny(); iy++)
			    for(int ix=0; ix<data[ic][iw].nx(); ix++, ip++)
				buffer[ip] = data[ic][iw][iy][ix];
		    // right-hand windows
		    for(size_t iw=1; iw<data[ic].size(); iw+=2)
			for(int iy=0; iy<data[ic][iw].ny(); iy++)
			    for(int ix=0; ix<data[ic][iw].nx(); ix++, ip++)
				buffer[ip] = data[ic][iw][iy][ix];

		    double rawmean, rawrms, rmean, lmean, rms;
		    int nrej;
		    Subs::sigma_reject(buffer, buffer.size()/2, 3.0, false, rawmean, rawrms, lmean, rms, nrej);
		    Subs::sigma_reject(buffer+buffer.size()/2, buffer.size()/2, 3.0, false, rawmean, rawrms, rmean, rms, nrej);
		    
		    double offset = lmean - rmean;
		    std::cout << " " << ic+1 << " " << offset;
		    for(int i=buffer.size()/2; i<buffer.size(); i++)
			buffer[i] += offset;
		}
		
		// Sort into ascending order (discarding key)
		buffer.sort();

		double rawmean, rawrms, mean, rms;
		int nrej;
		Subs::sigma_reject(buffer, buffer.size(), 3.0, false, rawmean, rawrms, mean, rms, nrej);

		std::cout << " " << std::setprecision(6) << rawmean << " " << rawrms << " " << mean << " " << rms << " " 
			  << std::setw(6) << nrej << " " << std::setw(5) << int(buffer[0]) << " " << int(buffer[int(0.0001*(ntot-1)+0.5)]) << " " 
			  << int(buffer[int(0.001*(ntot-1)+0.5)]) << " " << int(buffer[int(0.01*(ntot-1)+0.5)]) << " " 
			  << int(buffer[int(0.1*(ntot-1)+0.5)]) << " " << int(buffer[int(0.3*(ntot-1)+0.5)]) << " " 
			  << int(buffer[int(0.5*(ntot-1)+0.5)]) << " " << int(buffer[int(0.7*(ntot-1)+0.5)]) << " " 
			  << int(buffer[int(0.9*(ntot-1)+0.5)]) << " " << int(buffer[int(0.99*(ntot-1)+0.5)]) << " " 
			  << int(buffer[int(0.999*(ntot-1)+0.5)]) << " " << int(buffer[int(0.9999*(ntot-1)+0.5)]) << " " 
			  << int(buffer[ntot-1]);
		    

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



