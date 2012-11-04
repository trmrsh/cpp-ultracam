#include <string>
#include <vector>

// for cURL http software
#include <string.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "cpgplot.h"
#include "trm_subs.h"
#include "trm_ultracam.h"

//! Little structure for setup windows
struct Swin {
    Swin() : xstart(1), ystart(1), nx(120), ny(120) {}
    int xstart;    /**< left-hand X limit of the window */ 
    int ystart;    /**< lower y value of the window */ 
    int nx;        /**< number of unbinned pixels in X */ 
    int ny;        /**< number of unbinned pixels in Y */
};

/**
 * Plots setup windows in rtplot
 * \param setwin the name of the file containing the setup windows or the http URL of the Java server
 * that generates rtplot setup files This should of the form 'http://135.205.45.7'. The port number will
 * be added. The file format is ASCII starting with a line of the binning factors, NX NY, e.g. '1 2', 
 * then followed by definitions of the window pairs in the form XSTART YSTART NX NY. NX, NY should be in 
 * unbinned pixels.
 * \param numccd the total number of CCDs 
 * \param x1 left plot limit
 * \param x2 right plot limit
 * \param y1 lower plot limit
 * \param y2 upper plot limit
 * \param all true to plot all CCDs
 * \param stackdirn  stacking direction for multi-ccd plots: either 'X' or 'Y'
 * \param nccd the CCD number to plot if not all.
 * \param ultraspec true for ULTRASPEC, otherwise ULTRACAM. For ULTRACAM there must be 2, 4 or 6 windows,
 * and they must come in the order left-hand window, right-hand window. For ULTRASPEC there must be 1 or 2 windows.
 */

void Ultracam::plot_setupwins(const std::string& setwin, int numccd, float x1, float x2, float y1, float y2, 
			      bool all, char stackdirn, int nccd, bool ultraspec){

    const char BEEP = '\a';

    try{

	int xbin=0, ybin=0;
	std::vector<Swin> wins;
	static std::vector<Swin> old_wins;
	Swin win;
    
	// Start by loading in the data
	if(setwin.find("http:") == 0){
      
	    // 'http:' at the start means that we want to get the data from a server
	    // Buffer to receive the rtplot setup file. Use malloc rather than new for the curl stuff
	    // 1000 characters should be more than enough to avoid the need for any reallocation
	    // by WriteMemoryCallback
	    MemoryStruct buffer;
	    buffer.size   = 1000;
	    buffer.memory = (char *)malloc(buffer.size);
	    if(!buffer.memory) throw Ultracam::Ultracam_Error("failed to allocate cURL read buffer");
	    buffer.posn = 0;
      
	    CURL* curl_handle = curl_easy_init();
      
	    // Send all data to WriteFunction
	    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
      
	    // Pass the buffer to the callback function
	    curl_easy_setopt(curl_handle, CURLOPT_FILE, (void *)&buffer);
      
	    // Set up an error buffer
	    char* error_buffer = new char[CURL_ERROR_SIZE];
	    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);
      
	    // Add port. this port number must match that used by the server of course
	    std::string URL = setwin + std::string(":5100");
	    curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
      
	    // Get the data
	    int success = curl_easy_perform(curl_handle);

	    // Wind down curl
	    curl_easy_cleanup(curl_handle);
	
	    if(success != 0){
		std::cout << error_buffer << std::endl;
		delete[] error_buffer;
		free(buffer.memory);
		throw Ultracam_Error("failed to read windows data from server");	
	    }
	    delete[] error_buffer;      

	    // Now have window file in first buffer.posn characters. Interpret them.
	    std::string sbuffer(buffer.memory, buffer.posn);
	    free(buffer.memory);
	    if(sbuffer.find("No valid data") != std::string::npos)
		throw Ultracam_Error("no valid windows were available from the server.");	

	    // Read from an istringstream
	    std::istringstream istr(sbuffer);
	    if(istr){
		int nwin;
		istr >> xbin >> ybin >> nwin;
		if(!istr)
		    throw  Ultracam_Error("failed to read binning factors and number of windows from server data");
	      
		if(xbin < 1 || ybin < 1)
		    throw Ultracam_Error("invalid binning factors = " + Subs::str(xbin) + ", " + Subs::str(ybin) + " from server.");

		if((!ultraspec && nwin != 2 && nwin != 4 && nwin != 6)  || (ultraspec && (nwin < 1 || nwin > 4)))
		    throw Ultracam_Error("invalid number of windows = " + Subs::str(nwin) + " from server.");
	      
		for(int nw=0; nw<nwin; nw++){
		    istr >> win.xstart >> win.ystart >> win.nx >> win.ny;
		    if(!istr){
			throw Ultracam_Error("could not interpret windows line from server");
		    }else if(win.nx < 1 || win.ny < 1){
			throw Ultracam_Error("window with NX and/or NY < 1 from server");
		    }else{
			wins.push_back(win);
		    }
		}
	    }else{
		throw Ultracam_Error("failed to create std::string buffer from server data");
	    } 

	}else{
      
	    // Normal disc file
	    std::ifstream fswin(setwin.c_str());
	    if(fswin){
		char c;
		while(fswin && (c = fswin.peek(), fswin) && !fswin.eof()){
		    if(c != ' ' && c != '\t' && c != '#' && c != '\n'){
	    
			if(xbin == 0){
			    fswin >> xbin >> ybin;
			    if(!fswin)
				throw  Ultracam_Error("failed to read binning factors from setup file = " + setwin);
	      
			    if(xbin < 1 || ybin < 1)
				throw Ultracam_Error("invalid binning factors = " + Subs::str(xbin) + ", " + Subs::str(ybin) 
						     + " in setup file = " + setwin);
			}else{
	      
			    fswin >> win.xstart >> win.ystart >> win.nx >> win.ny;
			    if(!fswin){
				throw Ultracam_Error("could not interpret windows line in setup file = " + setwin);
			    }else if(win.nx < 1 || win.ny < 1){
				throw Ultracam_Error("window std::pair with NX and/or NY < 1 in setup file = " + setwin);
			    }else{
				wins.push_back(win);
			    }
			}
		    }else{
	    
			// Skip the line
			fswin.ignore(10000, '\n');
		    }
		}
		fswin.close();
	    }else{
		throw Ultracam_Error("failed to open setup file = " + setwin);
	    } 
	}

	// Now check windows read in.
	if(xbin < 1 || ybin < 1)
	    throw Ultracam_Error("no valid binning factors found");

	if(wins.size() == 0){
	    throw Ultracam_Error("no window definitions found");

	}else if((!ultraspec && wins.size() != 2 && wins.size() != 4 && wins.size() != 6)  || 
		 (ultraspec && wins.size() != 1 && wins.size() != 2)){
	    throw Ultracam_Error("invalid number of windows = " + Subs::str(wins.size()));

	}else{
      
	    // Checks on the windows. Do not abort, just plot in red and make
	    // a beep if there are problems
	    bool error = false;
	    for(size_t iw=0; iw<wins.size(); iw++){
	
		if(wins[iw].nx % xbin != 0){
		    error = true;
		    std::cerr << "plot_setupwins: NX not commensurate with X binning factor in setup window number " << iw + 1 << std::endl;
		}
	
		if(wins[iw].ny % ybin != 0){
		    error = true;
		    std::cerr << "plot_setupwins: NY not commensurate with Y binning factor in setup window number " << iw + 1 << std::endl;
		}
		
		if(ultraspec && (wins[iw].ystart < 1 || wins[iw].ystart > 1072)){
		    error = true;
		    std::cerr << "plot_setupwins: YSTART out of range 1 to 1072 in setup window number " << iw + 1 << std::endl;

		}else if(!ultraspec && (wins[iw].ystart < 1 || wins[iw].ystart > 1024)){
		    error = true;
		    std::cerr << "plot_setupwins: YSTART out of range 1 to 1024 in setup window number " << iw + 1 << std::endl;
		}
		
		if(!ultraspec && iw % 2 == 0){
		    if(wins[iw].xstart < 1 || wins[iw].xstart > 512){
			error = true;
			std::cerr << "plot_setupwins: XSTART out of range 1 to 512 in setup window number " << iw + 1 << std::endl;
		    }else if(wins[iw].xstart + wins[iw].nx > 513){
			error = true;
			std::cerr << "plot_setupwins: NX = " << wins[iw].nx 
				  << " too large given XSTART = " << wins[iw].xstart
				  << " in setup window number " << iw + 1 << std::endl;
		    }
		}else if(!ultraspec && iw % 2 == 1){
		    if(wins[iw].xstart < 513 || wins[iw].xstart > 1024){
			error = true;
			std::cerr << "plot_setupwins: XSTART out of range 513 to 1024 in setup window number " << iw + 1 << std::endl;
		    }else if(wins[iw].xstart + wins[iw].nx > 1025){
			error = true;
			std::cerr << "plot_setupwins: NX = " << wins[iw].nx 
				  << " too large given XSTART = " << wins[iw].xstart
				  << " in setup window std::pair number " << iw + 1 << std::endl;
		    }
		}else if(ultraspec){
		    if(wins[iw].xstart < 1 || wins[iw].xstart > 1072){
			error = true;
			std::cerr << "plot_setupwins: XSTART out of range 1 to 1072 in setup window number " << iw + 1 << std::endl;
		    }else if(wins[iw].xstart + wins[iw].nx > 1073){
			error = true;
			std::cerr << "plot_setupwins: NX = " << wins[iw].nx 
				  << " too large given XSTART = " << wins[iw].xstart
				  << " in setup window std::pair number " << iw + 1 << std::endl;
		    }
		}
	
		if(!ultraspec && wins[iw].ystart + wins[iw].ny > 1025){
		    error = true;
		    std::cerr << "plot_setupwins: NY = " << wins[iw].ny 
			      << " too large given YSTART = " << wins[iw].ystart 
			      << " in setup window std::pair number " << iw + 1 << std::endl;
		}else if(ultraspec && wins[iw].ystart + wins[iw].ny > 1073){
		    error = true;
		    std::cerr << "plot_setupwins: NY = " << wins[iw].ny 
			      << " too large given YSTART = " << wins[iw].ystart 
			      << " in setup window std::pair number " << iw + 1 << std::endl;
		}
		
		// final checks for overlap between windows	
		for(size_t iww=0; iww<iw; iww++){
		    if((wins[iw].xstart < wins[iww].xstart+wins[iww].nx && wins[iw].xstart+wins[iw].nx > wins[iww].xstart) &&
		       (wins[iw].ystart < wins[iww].ystart+wins[iww].ny && wins[iw].ystart+wins[iw].ny > wins[iww].ystart)){
			error = true;
			std::cerr << "plot_setupwins: setup window " << iw + 1 << " overlaps window " << iww + 1 << std::endl;
		    }
		}

	    }
      
	    if(error) std::cerr << BEEP << std::flush;
      
	    // Finally plot
	    for(int ip=0; ip<2; ip++){

		const std::vector<Swin> &twins = ip == 0 ? old_wins : wins;

		cpgsls(2);
		cpgsfs(2);		    
		if(ip == 0){
		    cpgsci(Subs::BLACK);
		}else{
		    if(error){
			cpgsci(Subs::RED);
		    }else{
			cpgsci(Subs::YELLOW);
		    }
		}
		
		if(all){
		    if(stackdirn == 'X'){
			cpgsubp(numccd,1);
		    }else if(stackdirn == 'Y'){
			cpgsubp(1,numccd);
		    }else{
			throw Ultracam_Error(std::string("invalid stacking option = ") + stackdirn);
		    }
		    
		    for(int ic=0; ic<numccd; ic++){
			if(stackdirn == 'X'){
			    cpgpanl(ic+1,1);
			}else{
			    cpgpanl(1,ic+1);
			}
			cpgwnad(x1,x2,y1,y2);
			for(size_t iw=0; iw<twins.size(); iw++)
			    cpgrect(twins[iw].xstart-0.5,twins[iw].xstart+twins[iw].nx-0.5, twins[iw].ystart-0.5,twins[iw].ystart+twins[iw].ny-0.5);
		    }
		}else{
		    for(size_t iw=0; iw<twins.size(); iw++)
			cpgrect(twins[iw].xstart-0.5,twins[iw].xstart+twins[iw].nx-0.5, twins[iw].ystart-0.5,twins[iw].ystart+twins[iw].ny-0.5);
		}
		cpgsls(1);
	    }

	    old_wins = wins;
	}
    }
    catch(const Ultracam_Error& e){
	std::cerr << "Error in plot_setupwins: " << e << "\n" << BEEP << std::flush;
    }
}
