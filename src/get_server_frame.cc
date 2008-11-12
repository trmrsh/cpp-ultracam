// Make sure that we can access > 2^31 bytes

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include <fstream>
#include <bitset>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include "trm_subs.h"
#include "trm_time.h"
#include "trm_frame.h"
#include "trm_ultracam.h"

/** Gets a frame from a server data file.
 * \param source source of data: either 'S' for server or 'L' for local .xml file.
 * \param url URL of file, as in 'http://127.0.0.1:8007/run00000001', or name of file on
 * a local disk. Do not add '.xml' to it. 
 * \param data       the data file to load into
 * \param serverdata data compiled by parseXML
 * \param nfile the file number to read, starting from 1. Set = 0 to get the most recent frame, regardless of its number which will
 * be returned.
 * \param twait if you think the frame might appear while the program is running, then set twait to be the number of seconds
 * wait between successive attempts at accessing it.
 * \param tmax  this is the maximum amount of time worth waiting. Set <= 0 not to wait at all.
 * \param reset allows you to start again, as needed for twopass operation (set = true for first one of second pass)
 * \param demultiplex set this false if you are not interested in the data, but just the headers. It then avoids the demultiplexing
 * stage.
 * \return true if successful, false if not.
 */

bool Ultracam::get_server_frame(char source, const std::string& url, Frame& data, const Ultracam::ServerData& serverdata,
				size_t& nfile, double twait, double tmax, bool reset, bool demultiplex){

    static bool first = true;   // for initialisation
    static size_t headerskip;   // number of header bytes
    static std::ifstream fin;        // input stream for local file case.

    MemoryStruct buffer; // buffer for data
    CURL *curl_handle  = NULL;  
    char *error_buffer = NULL;  // errors from cURL

    // allocate the buffer
    buffer.size   = std::max(1000, serverdata.framesize);
    buffer.memory = (char *)malloc(buffer.size);
    if(!buffer.memory) throw Ultracam::Ultracam_Error("Failed to allocate cURL read buffer");
    buffer.posn = 0;

    if(source == 'S'){

	// initialise cURL
	curl_handle = curl_easy_init();
	
	// Send all data to WriteFunction
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	
	// pass the buffer to the callback function
	curl_easy_setopt(curl_handle, CURLOPT_FILE, (void *)&buffer);
	
	// Set up an error buffer
	error_buffer = new char[CURL_ERROR_SIZE];
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);

    }


    if(first || reset){

	first = false;

	if(source == 'L'){

	    if(!fin.is_open()) {
		fin.open(std::string(url + ".dat").c_str(), std::ios::binary);
		if(!fin) throw File_Open_Error(std::string("bool Ultracam::get_server_frame(char, const std::string&, Frame&, "
							   "const Ultracam::ServerData&, size_t&, double, double):\n"
							   "failed to open ") + url + std::string(".dat")); 
	    }

	}
	headerskip = serverdata.headerwords*serverdata.wordsize;
    }

    double total = 0.;

    if(nfile == 0){

	static size_t lastfile = 0;

	if(source == 'S'){
      
	    // If nfile == 0, it means we want the most recent frame, so we find out how many there are.
	    std::string URL = url + std::string("?action=get_num_frames");
	    curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
      
	    nfile = 1;
	    int success = 1;
	    while((success || nfile == lastfile) && total <= tmax){
	
		// Reset buffer
		buffer.posn  = 0;
	
		// Get the data
		success = curl_easy_perform(curl_handle);
	
		if(success != 0){
	  
		    // This error is usually temporary
		    std::cout << error_buffer << std::endl;
		    std::cout << "Will wait one second before trying again" << std::endl;
		    Subs::sleep(1.);
		    total += 1.;
	  
		}else{
	  
		    char* s = strstr(buffer.memory, "nframes=\"");
		    if(!s){
			// Failed old method, try new server method.
			s = strstr(buffer.memory, "appears to have");
			if(!s){
			    curl_easy_cleanup(curl_handle);
			    free(buffer.memory);
			    delete[] error_buffer;
			    throw Ultracam::Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
							   " const std::string&, bool, size_t&, double, double):\n"
							   " could not find the number of frames (old or new server)");
			}else{

			    // Chop off after number and translate
			    char* e = strchr(s+15, 'b');
			    *e = 0;
			    std::istringstream istr(s+15);
			    istr >> nfile;
			    if(!istr){
				curl_easy_cleanup(curl_handle);
				free(buffer.memory);
				delete[] error_buffer;
				throw Ultracam::Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
							       " const std::string&, bool, size_t&, double, double):"
							       " could not translate number of frames (new server)");
			    }
			}

		    }else{
	  
			// Chop off quotes after number and translate
			char* e = strchr(s+9, '"');
			*e = 0;
			std::istringstream istr(s+9);
			istr >> nfile;
			if(!istr){
			    curl_easy_cleanup(curl_handle);
			    free(buffer.memory);
			    delete[] error_buffer;
			    throw Ultracam::Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
							   " const std::string&, bool, size_t&, double, double):"
							   " could not translate number of frames (old server)");
			}
		    }

		    if(nfile == lastfile){
			if(tmax > 0.){
			    std::cerr << "Last file has not changed since last time\n";
			    std::cerr << "Will wait " << twait << " secs before trying again.\n";
			    Subs::sleep(twait);
			    total   += std::max(0.01,twait);
			    success = 1;
			}else{
			    std::cerr << "Last file has not changed since last time\n";
			    std::cerr << "Finishing input of server data.\n";
			    curl_easy_cleanup(curl_handle);
			    free(buffer.memory);
			    delete[] error_buffer;
			    return false;
			}
		    }
		}
	    }

	}else if(source == 'L'){

	    nfile = 1;
	    do{

		// Determine number of files first
		fin.seekg(0, std::ios::end);
		if(!fin){
		    free(buffer.memory);
		    throw Ultracam::Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
						   " const std::string&, bool, size_t&, double, double):\n"
						   " could not move to the end of the file (1).");
		}
		nfile = fin.tellg() / serverdata.framesize;

		if(!fin){
		    free(buffer.memory);
		    throw Ultracam::Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
						   " const std::string&, bool, size_t&, double, double):\n"
						   " could not work out 'get' pointer position (1).");
		}

		if(nfile == lastfile){
		    if(tmax > 0.){
			std::cerr << "Last file has not changed since last time = " << lastfile << std::endl;
			std::cerr << "Will wait " << twait << " secs before trying again." << std::endl;
			Subs::sleep(twait);
			total   += std::max(0.01,twait);
		    }else{
			if(nfile > 0){
			    std::cerr << "Last file has not changed since last time = " << lastfile << std::endl;
			    std::cerr << "Finishing input of data from local file." << std::endl;
			}
			fin.close();
			free(buffer.memory);
			return false;
		    }
		}
	    }while(nfile == lastfile && total <= tmax);

	}

	lastfile = nfile;

	if(total > tmax){
	    std::cout << "Waited longer than the maximum = " << tmax << " secs." << std::endl;
	    std::cout << "Finishing input of server data." << std::endl;
	    if(source == 'S'){
		curl_easy_cleanup(curl_handle);
		delete[] error_buffer;
	    }else{
		fin.close();
	    }
	    free(buffer.memory);
	    return false;
	}
    }


    // OK, so we want to access file number 'nfile'
    if(source == 'S'){

	// For server, files start at 0      
	std::string URL = url + "?action=get_frame&frame=" + Subs::str(nfile - 1);
    
	curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
    
	// keep trying because sometimes get errors when there is really no problem
	int success = 1;
	while(success && total <= tmax ){
      
	    buffer.posn  = 0;
	    // Get the data
	    success = curl_easy_perform(curl_handle);

	    if(success != 0){
		// This error is usually temporary
		std::cout << error_buffer << std::endl;
		std::cout << "Will wait one second before trying again" << std::endl;
		Subs::sleep(1.);
		total += 1.;
	
	    }else{
	
		// Check type
		char *content_type;
		curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &content_type);

		if(strcmp(content_type, "image/data") != 0){

		    if(strcmp(buffer.memory, "observation") == 0){
			curl_easy_cleanup(curl_handle);
			std::string err(buffer.memory);
			free(buffer.memory);
			delete[] error_buffer;
			throw Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
					     " const std::string&, bool, size_t, double, double):\n"
					     " wrong data returned = " + err); 
		    }
		    if(tmax > 0.){
			std::cout << "Suspect file number " << nfile << " is not ready yet." << std::endl;
			std::cout << "Will wait " << twait << " secs before trying again." << std::endl;
			Subs::sleep(twait);
			total   += std::max(0.01,twait);
	    
			// try again
			success  = 1;
		    }else{
			// No attempt to try again
			curl_easy_cleanup(curl_handle);
			free(buffer.memory);
			delete[] error_buffer;
			return false;
		    }

		}else{

		    // This should be good data but the fileserver of jan 2008 returns image/data
		    // even when the frame does not exist although it is only sending back a 404 error
		    if(strncmp(buffer.memory, "<h1>ERROR (404) - Not Found</h1>", 32) == 0){
			if(tmax > 0.){
			    std::cout << "Suspect file number " << nfile << " is not ready yet." << std::endl;
			    std::cout << "Will wait " << twait << " secs before trying again." << std::endl;
			    Subs::sleep(twait);
			    total   += std::max(0.01,twait);
	    
			    // try again
			    success  = 1;
			}
		    }
		}
	    }
	}

    }else if(source == 'L'){

	while(total <= tmax ){

	    // Compute number of frames to check that we are not asking for more than
	    // is available.

	    fin.seekg(0, std::ios::end);
	    if(!fin){
		free(buffer.memory);
		throw Ultracam::Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
					       " const std::string&, bool, size_t&, double, double):\n"
					       " could not move to the end of the file (2).");
	    }

	    size_t nfile_tot = fin.tellg() / serverdata.framesize;
	    if(!fin){
		free(buffer.memory);
		throw Ultracam::Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
					       " const std::string&, bool, size_t&, double, double):\n"
					       " could not work out 'get' pointer position (2).");
	    }

	    if(nfile <= nfile_tot){

		// OK get some data
		fin.seekg(off_t(serverdata.framesize)*off_t(nfile-1), std::ios::beg);
		if(!fin){
		    free(buffer.memory);
		    throw Ultracam::Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
						   " const std::string&, bool, size_t&, double, double):\n"
						   " failed to move into position for reading data.");
		}
		fin.read(buffer.memory,serverdata.framesize);
		if(!fin){
		    free(buffer.memory);
		    throw Ultracam::Ultracam_Error("bool Ultracam::get_server_frame(Frame&, const Ultracam::ServerData&,"
						   " const std::string&, bool, size_t&, double, double):\n"
						   " failed to read data from local disk file.");
		}

		// everything OK
		break;

	    }else{

		if(tmax > 0.){
		    std::cout << "Suspect file number " << nfile << " is not ready yet." << std::endl;
		    std::cout << "Will wait " << twait << " secs before trying again." << std::endl;
		    Subs::sleep(twait);
		    total   += std::max(0.01,twait);
	  
		}else{
		    // No attempt to try again
		    fin.close();
		    free(buffer.memory);
		    return false;
		}
	    }
	}
    }

    if(total > tmax){
	std::cout << "Waited longer than the maximum = " << tmax << " secs." << std::endl;
	std::cout << "Finishing input of server data." << std::endl;

	if(source == 'S'){
	    delete[] error_buffer;
	    curl_easy_cleanup(curl_handle);
	}else{
	    fin.close();
	}
	free(buffer.memory);
	return false;
    }

    // added to get around problem of large files
    if(source == 'S'){
	delete[] error_buffer;
	curl_easy_cleanup(curl_handle);
    }

    // Now have data loaded into the memory buffer, the same for either source.

    // Work out time and frame number

    TimingInfo timing;
    Ultracam::read_header(buffer.memory, serverdata, timing);

    if(timing.frame_number != int(nfile)) 
	std::cerr << "WARNING: conflicting frame numbers in bool Ultracam::get_next_server_frame(Frame&, const Ultracam::ServerData&, const std::string&, size_t&: " 
		  << timing.frame_number << " vs " << nfile << std::endl;


    if(serverdata.nblue > 1){
	data.set("UT_date_blue",   new Subs::Htime(timing.ut_date_blue, "UT at the centre of the u-band exposure"));
	data.set("Exposure_blue",  new Subs::Hfloat(timing.exposure_time_blue, "u-band exposure time, seconds"));
	data.move_to_top("Exposure_blue");
	data.move_to_top("UT_date_blue");
    }

    data.set("UT_date",            new Subs::Htime(timing.ut_date, "UT at the centre of the exposure"));
    data.set("Exposure",           new Subs::Hfloat(timing.exposure_time, "Exposure time, seconds"));
    data.move_to_top("Exposure");
    data.move_to_top("UT_date");

    data.set("Frame",              new Subs::Hdirectory("Other frame specific information"));
    data.set("Frame.reliable",     new Subs::Hbool(timing.reliable,              "UT_date reliable?"));
    data.set("Frame.GPS_time",     new Subs::Htime(timing.gps_time,              "Raw GPS time stamp associated with this frame"));
    data.set("Frame.frame_number", new Subs::Hint(timing.frame_number,           "Frame number"));
    data.set("Frame.satellites",   new Subs::Hint(timing.nsatellite,             "Number of satellites used for GPS time stamp"));
    data.set("Frame.vclock_frame", new Subs::Hfloat(timing.vclock_frame,         "The row transfer time used, seconds"));
    data.set("Frame.as_documented",new Subs::Hbool(timing.fix_as_documented,     "Timestamps as documented (else Dec 2004 bug fix)?"));
    data.set("Frame.bad_blue",     new Subs::Hbool(timing.blue_is_bad,           "Blue-side data is junk for this frame"));
    if(serverdata.nblue > 1)
	data.set("Frame.reliable_blue", new Subs::Hbool(timing.reliable_blue, "UT_date_blue reliable?"));

    // Store status bit
    data.set("Frame.last", new Subs::Hbool((buffer.memory[0] & 1<<0) == 1<<0, "Last frame?"));

    // Like to know if this ever occurs
    if((buffer.memory[0] & 1<<2) == 1<<2)
	std::cerr << "WARNING: second status bit representing a 'pon error' was set. Let Tom Marsh know if you ever see this." << std::endl;
      
    if(demultiplex){
	if(serverdata.instrument == "ULTRACAM"){
	    Ultracam::de_multiplex_ultracam(buffer.memory+headerskip, data);
	}else{
	    Ultracam::de_multiplex_ultraspec(buffer.memory+headerskip, data);
	}
    }

    free(buffer.memory);
    return true;
}

