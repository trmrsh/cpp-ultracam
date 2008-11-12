/*

!!begin
!!title    grab
!!author   T.R. Marsh
!!created  06 May 2002
!!revised  13 Nov 2007
!!root     grab
!!index    grab
!!descr    grabs server or local .dat files, writing them as .ucm files
!!class    Reduction
!!class    Observing
!!class    IO
!!css      style.css
!!head1    grabs server or local .dat files, writing them as .ucm files

!!emph{grab} grabs an ultracam run from the server or local disk and splits it up
into ".ucm" files, as needed for flat field preparation etc.
The files will be dumped in whichever disk this is run. One '.dat' file
can generate many .ucm files. These will have names consisting of the
server file name + "_001" etc. The server must be running for this program
to get data from the server of course. You may experience problems defining the directory path.
It is helpful to look at the messages the server produces to see where it
is trying to find the files in this case.  Note that !!emph{grab} will skip
over junk data in the case of drift mode, so your first file might be number 
5 say even though you asked for number 1.

Very often one ctrl-C's !!emph{grab} to exit it. This may well lead to the last file being
corrupted. This is especially the case if it is not listed as having been written
to disk and yet exists. You need to make sure that such files do not creep into file lists
for using !!ref{combin.html}{combine} or whatever.

Note that you should use !!emph{grab} for bias subtraction when making darks because it stores the 
exposure time of the bias frame in the result which is needed for bias subtraction.

!!head2 Invocation

grab [source] (url)/(file) ndigit first (last) trim [(ncol nrow) twait tmax] skip
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

!!arg{threshold}{If you are applying a bias to ULTRASPEC L3CCD data, you have the option of 
converting to photon counts (0 or 1 whether above or below a certain threshold).}

!!arg{photon}{The threshold level if threshold = true}

!!arg{naccum}{Accumulate data into the sum of naccum frames before writing to disk. If some are still
accumulating at the end, they will be written out even if they have not reach 'naccum'.}

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
	input.sign_in("ndigit",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
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
	input.sign_in("threshold", Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("photon",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("naccum",    Subs::Input::GLOBAL, Subs::Input::PROMPT);

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
	int ndigit;
	input.get_value("ndigit", ndigit, 0, 0, 20, "number of digits in file numbers");
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
	parseXML(source, url, mwindow, header, serverdata, trim, ncol, nrow);

	Ultracam::Frame data(mwindow, header);

	Subs::Header::Hnode *hnode = data.find("Instrument.instrument");
	bool ultraspec = (hnode->has_data() && hnode->value->get_string() == "ULTRASPEC"); 

	bool bias, thresh = false;
	float photon;
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
	
	    if(ultraspec){ 
		input.get_value("threshold", thresh, true, "do you want to threshold to get 0 or 1 photons/pix?");
		if(thresh)
		    input.get_value("photon", photon, 50.f, FLT_MIN, FLT_MAX, "threshold level to count as 1 photon");
	    }
	}
    
	int naccum = 1;
	if(ultraspec) 
	    input.get_value("naccum", naccum, 1, 1, 10000, "number of frames to accumulate before writing");

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

	// Data buffer if naccum > 1 
	Ultracam::Frame dbuffer;
	int nstack = 0;
	double ttime = 0.;

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
      
	    // Apply threshold
	    if(thresh) data.step(photon);

	    nstack++;
	    if(nstack < naccum){
		if(nstack == 1){
		    dbuffer = data;
		    ttime   = 0.;
		    std::cout << std::endl;
		}else{
		    dbuffer += data;
		}
		ttime   += data["UT_date"]->get_double();
		std::cout << " Frame " << nstack << " of " << naccum << ", time = " << data["UT_date"]->get_time()  
			  << " added into data buffer." << std::endl;

	    }else{

		// Retrieve from the data buffer if necessary
		if(naccum > 1) {
		    ttime  += data["UT_date"]->get_double();
		    data   += dbuffer;
		    std::cout << " Frame " << nstack << " of " << naccum << ", time = " << data["UT_date"]->get_time()  
			      << " added into data buffer." << std::endl;
		    ttime  /= nstack;
		    data.set("UT_date", new Subs::Htime(Subs::Time(ttime), "mean UT date and time at the centre of accumulated exposure"));
		    nstack  = 0;
		    std::cout << std::endl;
		}

		if(bias || naccum > 1){
		    data.write(server_file + "_" + Subs::str(int(nfile), ndigit));
		}else{
		    // Write it out in RAW format to save space.
		    data.write(server_file + "_" + Subs::str(int(nfile), ndigit), Ultracam::Windata::RAW);
		}
	  
		if(naccum > 1){
		    std::cout << "Written " << server_file + "_" + Subs::str(int(nfile), ndigit) << ", mean time = " 
			      << data["UT_date"]->get_time() << ", exposure time = "  << form(data["Exposure"]->get_float()) 
			      << " secs to disk." << std::endl;
		}else{
		    std::cout << "Written " << server_file + "_" + Subs::str(int(nfile), ndigit) << ", time = " 
			      << data["UT_date"]->get_time() << ", exposure time = "  << form(data["Exposure"]->get_float()) 
			      << " secs to disk." << std::endl;
		}

	    }

	    if(first < 0 || (last > 0 && int(nfile) >= last)) break;
	    nfile++;
	}

	// Write last one out    
	if(naccum > 1 && nstack > 0) {
	    std::cout << "Writing sum of final " << nstack << " frames" << std::endl;
	    ttime  /= nstack;
	    dbuffer.set("UT_date", new Subs::Htime(Subs::Time(ttime), "mean UT date and time at the centre of accumulated exposure"));
	    dbuffer.write(server_file + "_" + Subs::str(int(nfile), ndigit));
	  
	    if(naccum > 1){
		std::cout << "Written " << server_file + "_" + Subs::str(int(nfile), ndigit) << ", mean time = " 
			  << dbuffer["UT_date"]->get_time() << ", exposure time = "  << form(dbuffer["Exposure"]->get_float()) 
			  << " secs to disk." << std::endl;
	    }else{
		std::cout << "Written " << server_file + "_" + Subs::str(int(nfile), ndigit) << ", time = " 
			  << dbuffer["UT_date"]->get_time() << ", exposure time = "  << form(dbuffer["Exposure"]->get_float()) 
			  << " secs to disk." << std::endl;
	    }
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



