/** parseXML reads an XML file from the ATC server or a local disk and extracts
 * format information from it. The ATC data format consists of one XML file and one 
 * data ('.dat') file per ULTRACAM run. The XML file defines the windows etc, and needs to be
 * parsed prior to accessing the data. The files have names of the form 'abcd.xml' and 'abcd.dat'. 
 * parseXML accesses the XML file across a network, for which the ATC server must be running,
 * or directly from local disk.
 * \param source source of data: either 'S' for server or 'L' for local .xml file.
 * \param XML_URL URL of file, as in 'http://127.0.0.1:8007/run013', or name of file on
 * a local disk. Do not add '.xml' to it. 
 * rather than through a server.
 * \param mwindow Returned multi-window object which contains the format.
 * \param header Returned header object
 * \param serverdata structure of returned information.
 * \param trim  true/false according to whether you want the returned mwindow to be re-jigged to eliminate known
 * problem parts. (on the lower rows and the columns closest to the readouts). You must use the same value of 
 * 'trim' and the next two arguments when de-multiplexing the data.
 * \param ncol the number of columns to trim. These are removed on the side next to the readouts.
 * \param nrow the number of rows to trim. These are removed from the lower edges of the windows.
 * \param twait number of seconds to wait between requests to find the xml file (only appears after first file)
 * \param tmax  maximum number of seconds to wait in total.
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>

// for cURL http software
#include <string.h>
#include <curl/curl.h>
#include <curl/easy.h>

// xerces stuff
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/MemBufInputSource.hpp> 
#include <xercesc/parsers/XercesDOMParser.hpp> 
#include <xercesc/util/XMLString.hpp>  

// my stuff
#include "trm_subs.h"
#include "trm_header.h"
#include "trm_mccd.h"
#include "trm_ultracam.h"
#include "trm_signal.h"

// namespace 
using namespace xercesc;

// Structures for storing the data of interest

struct Uinfo{
    std::string observatory;              // observatory name
    std::string telescope;                // telescope name
    int expose_time;                      // exposure time
    float time_units;                     // seconds per expose_time
    std::string gain_speed;               // gain speed setting
    int number_of_exposures;              // number of exposures
    int nccd;                             // number of CCDs
    int xbin, ybin;                       // binning factors (same for all CCDs)
    int revision;                         // ATC revision number
    std::vector<Ultracam::Wind> wind;     // windows (same for all CCDs)
    bool user_info;                       // There is user XML info available (May 2005 and onwards)
    std::string target;                   // Target name, if user_info
    std::string grating;                  // Grating, if ultraspec
    std::string filters;                  // filters, if user_info
    std::string slit_angle;               // Slit angle, if ultraspec
    std::string id;                       // ID, if user_info
    std::string pi;                       // PI, if user_info
    std::string observers;                // Observers, if user_info
};


// local declarations; see end for definitions
void parse_filesave_status(const DOMNode* const node, Uinfo& uinfo, Ultracam::ServerData& serverdata);
void parse_instrument_status(const DOMNode* const node, Uinfo& uinfo, Ultracam::ServerData& serverdata);
void parse_data_status(const DOMNode* const node, Ultracam::ServerData& serverdata);
void parse_user(const DOMNode* const node, Uinfo& uinfo, Ultracam::ServerData& serverdata);
std::string getTextValue(const DOMNode* const node);
bool same(const XMLCh* const native, const char* const local);
const std::string XtoString(const XMLCh* const native);
std::string AttToString(const DOMElement* elem, const char* const name);

void Ultracam::parseXML(char source, const std::string& XML_URL, Ultracam::Mwindow& mwindow, Subs::Header& header, 
			Ultracam::ServerData& serverdata, bool trim, int ncol, int nrow, double twait, double tmax){

    using Ultracam::Input_Error;

    MemoryStruct chunk;
  
    if(source == 'S'){
    
	CURL *curl_handle;
    
	// We really have no idea of the potential size of the stuff sent
	// back by the server in this case, so just start at 0
	chunk.memory = NULL; 
	chunk.size   = 0;    
	chunk.posn   = 0;    
    
	// initialise cURL
	curl_handle = curl_easy_init();
    
	// Get URL, action=get_xml will make ATC server send back everything
	std::string URL = XML_URL + "?action=get_xml";
    
	curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
    
	// Send all data to WriteMemoryCallback
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	// Pass chunk to the callback function
	curl_easy_setopt(curl_handle, CURLOPT_FILE, (void *)&chunk);

	double total = 0.;
	bool try_again = true;
	while(try_again && total <= tmax){

	    // reset buffer
	    chunk.posn = 0;

	    // Get the data. If the xml is not there, a 404 error is returned with a strng that
	    // is searched for. Don't ever put this string as a comment to an ultracam run ...
	    int success = curl_easy_perform(curl_handle);
	    if(success != 0)
		throw Input_Error("parseXML error: failed to access URL = " + URL + ".\nServer down or wrong URL?");

	    if(strstr(chunk.memory, "Exception: no_xml: file does not exist")){
		if(tmax > 0.){
		    std::cerr << "Request to server: " << URL << " failed.\nPerhaps the first exposure is not finished or the run number is wrong" << std::endl;
		    std::cerr << "Will wait " << twait << " secs before trying again." << std::endl;
		    Subs::sleep(twait);
		    total   += std::max(0.01,twait);
		}else{
		    std::cerr << "Request to server: " << URL << " failed.\nPerhaps the first exposure is not finished or the run number is wrong" << std::endl;
		    std::cerr << "tmax <=0  so finishing attempted input of server data." << std::endl;
		    curl_easy_cleanup(curl_handle);
		    free(chunk.memory);
		    throw Input_Error("parseXML error: gave up while trying to access URL = " + URL);
		}	  
	    }else{
		try_again = false;
	    }
	    if(global_ctrlc_set) break;
	}
    
	if(total > tmax || global_ctrlc_set){
	    if(total > tmax){
		std::cerr << "Waited longer than the maximum = " << tmax << " secs." << std::endl;
	    }else{
		std::cerr << "ctrl-C trapped inside parseXML" << std::endl;
	    }
	    std::cerr << "Finishing attempted input of server data." << std::endl;
	    throw Input_Error("parseXML error: gave up while trying to access URL = " + URL);
	}

	// Wind down curl
	curl_easy_cleanup(curl_handle);
   
    }else if(source == 'L'){

	// Read from a local file
	loadXML(XML_URL + ".xml", chunk);

    }else{
	throw Input_Error(std::string("parseXML error: data source = ") + source + std::string(" not recognised."));
    }

    // At this point chunk.memory points to a memory block that is chunk.size
    // bytes big and contains the entire XML file in the first chunk.posn characters.
    // We must remember to free it if any errors occur.

    // Now onto the Xerces XML DOM stuff. Initialise or not ...

    try{
	XMLPlatformUtils::Initialize();
    }
    catch(const XMLException& toCatch){
	free(chunk.memory);
	throw Input_Error(std::string("parseXML error: XML Exception: ") + XtoString(toCatch.getMessage()));
    }

    // Structure containing data that we want to extract
    Uinfo uinfo;
    uinfo.user_info = false;

    // It is important that any xerces resources are deallocated before calling the termination routine.
    // Otherwise segmentation faults result. To ensure this is the case, the resources are allocated dynamically
    // so that they can be explicitly deallocated when the need arises.

    // Construct a memory buffer for parsing. We do not copy it (to avoid wasting time), thus
    // the buffer must be maintained until we have finished with it.
    MemBufInputSource *membuf = new MemBufInputSource((const XMLByte*)chunk.memory, chunk.posn, "cURL input buffer", false);
    
    // Create the parser with no validation
    XercesDOMParser *parser = new XercesDOMParser;
    if(parser){
	parser->setValidationScheme(XercesDOMParser::Val_Never);
      
	try{
	    parser->parse(*membuf);
	    if(parser->getErrorCount() > 0)
		throw Input_Error("parseXML error: " + Subs::str(parser->getErrorCount()) + " XML parser errors occurred");
	
	    // Get root node
	    DOMNode *doc = parser->getDocument();
	
	    // Finally start to do some real work and see what we have got
	    DOMNode *child = doc->getFirstChild();
	
	    if(same(child->getNodeName(), "datainfo")){
	  
		bool found_data_status = false;
		DOMNodeList *grandchild = child->getChildNodes();
		bool found_user = false;
		for(unsigned int i=0; i<grandchild->getLength(); i++){
		    if(same(grandchild->item(i)->getNodeName(), "data_status")){
			parse_data_status(grandchild->item(i), serverdata);
			found_data_status = true;
		    }else if(same(grandchild->item(i)->getNodeName(), "user")){
			parse_user(grandchild->item(i), uinfo, serverdata);
			found_user = true;
		    }
		}
	  
		if(!found_data_status)
		    throw Input_Error("parseXML error: Could not find data_status node!");
	  
		// Must look at filesave status after data to get readout_mode mode status
		bool found_filesave_status = false;
		for(unsigned int i=0; i<grandchild->getLength(); i++){
		    if(same(grandchild->item(i)->getNodeName(),"filesave_status")){
			parse_filesave_status(grandchild->item(i), uinfo, serverdata);
			found_filesave_status = true;
		    }
		}

		if(serverdata.instrument == "ULTRACAM"){
		    if(found_user){
			std::cerr << "parseXML warning: found 'user' XML element; will assume 0.1 millisecond time exposure delay steps, valid as of January 2005" << std::endl;
			uinfo.time_units = 0.0001;
		    }else{ 
			std::cerr << "# parseXML warning: did NOT find 'user' XML element; will assume 1.0 millisecond time exposure delay steps, as valid before January 2005" << std::endl;
			uinfo.time_units = 0.001;
		    }
		    std::cerr << "parseXML warning: ULTRACAM file" << std::endl;
		}else{
                    // August 2012 Dave changed the time units to 0.1 millisec (had been 1) for Thai/ULTRASPEC
		    if(found_user && serverdata.headerwords == 16 && uinfo.revision >= 120813){
			std::cerr << "parseXML warning: version >= 120813; will assume 0.1 millisecond time exposure delay steps, valid as of August 2012" << std::endl;
                        uinfo.time_units = 0.0001;
                    }else{
			std::cerr << "parseXML warning: version < 120813; will assume 1 millisecond time exposure delay steps, valid prior to August 2012" << std::endl;
                        uinfo.time_units = 0.001;
                    }
		    std::cerr << "parseXML warning: ULTRASPEC file" << std::endl;
		}
	  
		if(!found_filesave_status)
		    throw Input_Error("parseXML error: Could not find filesave_status node!");
	  
	    }else if(same(child->getNodeName(),"error")){
		throw 
		    Input_Error("parseXML error: No data returned. Is the file name correct?");
	    }else{
		throw 
		    Input_Error("parseXML error: First node of XML file not = datainfo!");
	    }
	}
      
	catch(const XMLException& err){
	    delete parser;
	    delete membuf;
	    XMLPlatformUtils::Terminate();
	    free(chunk.memory);
	    throw Input_Error("XML Exception: " + XtoString(err.getMessage()));
	}
      
	catch (const DOMException& edom){
	    delete parser;
	    delete membuf;
	    xercesc::XMLPlatformUtils::Terminate();
	    free(chunk.memory);
	    throw Input_Error("DOM Exception, code = " + Subs::str(edom.code));
	}

	catch (...){
	    delete parser;
	    delete membuf;
	    xercesc::XMLPlatformUtils::Terminate();
	    free(chunk.memory);
	    throw;
	}
      
    }else{
	free(chunk.memory);
	throw Input_Error("parseXML error: Failed to get point to XML parser");
    }
    
    // All OK, then mop up
    delete parser;
    delete membuf;
    XMLPlatformUtils::Terminate();
    free(chunk.memory);
  
    // Now format the Mwindow and Header objects for return back to the calling program.
    mwindow.clear();
    mwindow.resize(uinfo.nccd);

    // Following parameters are not fundamental but allow frames to be compared on the same basis
    const int NXTOT = serverdata.instrument == "ULTRACAM" ? 1080 : 1056; // largest ever X dimension
    const int NYTOT = serverdata.instrument == "ULTRACAM" ? 1032 : 1072; // largest ever Y dimension

    // Trim all but overscan mode
    bool ok_to_trim = (trim && serverdata.readout_mode != Ultracam::ServerData::FULLFRAME_OVERSCAN);
    if(!ok_to_trim){
	nrow = 0;
	ncol = 0;
    }

    if(serverdata.headerwords == 16){

        const int NRECOG = 5;
        const int RECOG[NRECOG] = {100222, 111205, 120716, 120813, 130307};
        bool ok = false;
        int vfound = uinfo.user_info ? uinfo.revision : serverdata.version;
        for(int i=0; i<NRECOG; i++){
            if( vfound == RECOG[i]){
                ok = true;
                break;
            }
        }

	if(ok){
	    serverdata.version = vfound;
	}else{
            std::cerr << "parseXML warning: 16 header words found, but version number = " << vfound << " was not recognised out of 100222, 111205, 120716 or 120813" << std::endl;
            if(vfound > 120813){
                std::cerr << "parseXML warning: 120813 will be used, but this could indicate a programming error so watch out for timing issues." << std::endl;
                serverdata.version = 120813;
            }else{
                std::cerr << "parseXML warning: 100222 will be used, but this is probably a programming error so watch out for timing issues." << std::endl;
                serverdata.version = 100222;
            }
	}

	// Since March 2010, in 6-windows mode the V_FT_CLK parameter has had to go, so it is 
	// now hard-wired into the code. In DSP this is set to 0x8C0000, but I store simply as 
	// an unsigned char with value 140
	if(serverdata.application == "appl7_window3pair_cfg"){
	    std::cerr << "parseXML warning: 6-windows mode post-Mar 2010 identified; v_ft_clk byte (needed for precise times) will be set = 140" << std::endl;
	    serverdata.v_ft_clk  = 140;
	    serverdata.which_run = Ultracam::ServerData::OTHERS;
	}

    }else if(uinfo.user_info){

	if(serverdata.version != -1 && uinfo.revision != serverdata.version){
	    std::cerr << "parseXML warning: user revision number = " << uinfo.revision << " does not match preset revision = " << serverdata.version << std::endl;
	    std::cerr << "parseXML warning: the user revision number will be preferred but this could indicate a problem" << std::endl;
	}
	serverdata.version = uinfo.revision;
    }

    if(serverdata.instrument == "ULTRACAM" && serverdata.version < 0){
	if(serverdata.readout_mode != Ultracam::ServerData::FULLFRAME_OVERSCAN){
	    std::cerr << "parseXML warning: outermost pixels will be removed to account for pixel shift bug (should only happen before 2007)" << std::endl;
	}else{
	    std::cerr << "parseXML warning: the outermost pixels should be removed to account for pixel shift bug," << std::endl;
	    std::cerr << "parseXML warning: but this has not been worked out for overscan mode and nothing will be done." << std::endl;
	    std::cerr << "parseXML warning: If this is important, contact Tom Marsh." << std::endl;
	}
    }

    std::cerr << "parseXML warning: version number = " << serverdata.version << std::endl;

    // Reversed readout in the X-direction for the avalanche readout of the L3CCD requires
    // a correction to the llx value (as well as a re-ordering handled by de_multiplex)
    bool reverse = serverdata.instrument != "ULTRACAM" && serverdata.l3data.output == 1;
 
    for(size_t nccd=0; nccd<mwindow.size(); nccd++){
	for(size_t nwin=0; nwin<uinfo.wind.size(); nwin++){
      
	    const Wind& wind = uinfo.wind[nwin];

	    // Pixel shift bug: until the VLT run of June 2007, ULTRACAM suffered a problem which meant that the 
	    // first pixel read out was junk, regardless of the binning. thus if you specified a window from
	    // x=11 to x=22 you actually got J,11,12,13 ... 21. If you specified x=11 to 22, xbin=4 you got
	    // J,[11-14],[15-18]. If these two were compared on the basis that the unbinned format was correct, 
	    // it then seemed that the second started xbin-1 to the left of the first.  This is how I used to correct
	    // this problem, until the true problem was discovered by DA in May 2007.
	    //
	    // I now instead always remove the outermost pixels which are always junk (in data taken before May 2007)
	    // This is seen below in the removal of a pixel in the X-direction.
	    if(nwin % 2 == 0){
		if(serverdata.instrument == "ULTRACAM" && serverdata.version < 0){
		    mwindow[nccd].push_back(Window(wind.llx+ncol*uinfo.xbin, wind.lly+nrow*uinfo.ybin, wind.nx-ncol-1, wind.ny-nrow, uinfo.xbin, uinfo.ybin, NXTOT, NYTOT));
		}else{
		    if(reverse){
			mwindow[nccd].push_back(Window(wind.llx, wind.lly+nrow*uinfo.ybin, wind.nx-ncol, wind.ny-nrow, uinfo.xbin, uinfo.ybin, NXTOT, NYTOT));
		    }else{
			mwindow[nccd].push_back(Window(wind.llx+ncol*uinfo.xbin, wind.lly+nrow*uinfo.ybin, wind.nx-ncol, wind.ny-nrow, uinfo.xbin, uinfo.ybin, NXTOT, NYTOT));
		    }
		}
	    }else{
		if(serverdata.instrument == "ULTRACAM" && serverdata.version < 0){
		    mwindow[nccd].push_back(Window(wind.llx+uinfo.xbin, wind.lly+nrow*uinfo.ybin, wind.nx-ncol-1, wind.ny-nrow, uinfo.xbin, uinfo.ybin, NXTOT, NYTOT));
		}else{
		    if(reverse){
			mwindow[nccd].push_back(Window(wind.llx, wind.lly+nrow*uinfo.ybin, wind.nx-ncol, wind.ny-nrow, uinfo.xbin, uinfo.ybin, NXTOT, NYTOT));
		    }else{
			mwindow[nccd].push_back(Window(wind.llx, wind.lly+nrow*uinfo.ybin, wind.nx-ncol, wind.ny-nrow, uinfo.xbin, uinfo.ybin, NXTOT, NYTOT));
		    }
		}
	    }
	}
    }

    // Check the framesize and wordsize which sometimes seem to be in error
    const int WORDSIZE = 2;
    if(serverdata.wordsize != WORDSIZE)
	throw Input_Error("parseXML error: wordsize expected = " + Subs::str(WORDSIZE) + 
			  " but found = " + Subs::str(serverdata.wordsize) );
    int framesize = 0;
    if(serverdata.instrument == "ULTRACAM"){
	for(size_t nwin=0; nwin<uinfo.wind.size(); nwin++)
	    framesize += 2*uinfo.wind[nwin].nx*uinfo.wind[nwin].ny;
    }else{
	for(size_t nwin=0; nwin<uinfo.wind.size(); nwin++)
	    framesize += 2*(uinfo.wind[nwin].nx+serverdata.l3data.nchop[nwin])*uinfo.wind[nwin].ny;
    }
    framesize *= mwindow.size();
    framesize += WORDSIZE*serverdata.headerwords;
    if(serverdata.framesize != framesize)
	throw Input_Error("parseXML error: framesize expected = " + Subs::str(framesize) + " but found = " + Subs::str(serverdata.framesize) );

    // Set header items
    header.clear();

    // User information
    if(uinfo.user_info){
	header.set("User",         new Subs::Hdirectory("Data entered by the user at the telescope"));
	header.set("User", new Subs::Hdirectory("Data entered by the user at the telescope"));
	header.set("User.target",    new Subs::Hstring(uinfo.target, "Target name"));
	header.set("User.filters",   new Subs::Hstring(uinfo.filters, "Filters used"));
	header.set("User.id",        new Subs::Hstring(uinfo.id, "Program ID"));
	header.set("User.pi",        new Subs::Hstring(uinfo.pi, "Program PI"));
	header.set("User.observers", new Subs::Hstring(uinfo.observers, "Observers"));
	if (serverdata.instrument == "ULTRASPEC") {
	    header.set("User.grating",   new Subs::Hstring(uinfo.grating, "Grating"));
	    header.set("User.angle",     new Subs::Hstring(uinfo.slit_angle, "Slit angle"));
	}
    }

    // Add a little bit of history
    header.set("History",          new Subs::Hdirectory("History of this file"));
    header.set("History.Comment1", new Subs::Hstring("Orginally generated from run: " + XML_URL));

    // Instrument information
    header.set("Instrument",       new Subs::Hdirectory("Instrument information"));
    header.set("Instrument.instrument", new Subs::Hstring(serverdata.instrument, "Instrument"));
    header.set("Instrument.version",    new Subs::Hint(serverdata.version, "Software version; -1 = undefined"));
    if(serverdata.instrument == "ULTRACAM"){
	header.set("Instrument.Gain_Speed", new Subs::Hstring(uinfo.gain_speed, "Gain speed setting"));
	header.set("Instrument.v_ft_clk",   new Subs::Huchar(serverdata.v_ft_clk, "Vertical clocking time parameter."));
	header.set("Instrument.nblue",      new Subs::Hint(serverdata.nblue, "Number of u-band co-adds."));
    }else{
	if(serverdata.l3data.gain >= 0)
	    header.set("Instrument.Gain",     new Subs::Hint(serverdata.l3data.gain,       "L3CCD gain parameter"));
	header.set("Instrument.Output",   new Subs::Hint(serverdata.l3data.output,     "L3CCD output"));
	header.set("Instrument.HV_Gain",  new Subs::Hint(serverdata.l3data.hv_gain,    "L3CCD HV gain parameter"));
	header.set("Instrument.Clear",    new Subs::Hint(serverdata.l3data.en_clr,     "L3CCD clear enabled or not"));
	if(serverdata.l3data.speed >= 0)
	    header.set("Instrument.Speed",    new Subs::Hint(serverdata.l3data.speed,      "L3CCD speed setting; 0=slow,1=medium,2=fast"));
	if(serverdata.l3data.led_flsh >= 0)
	    header.set("Instrument.Led_Flsh", new Subs::Hint(serverdata.l3data.led_flsh,   "L3CCD led flash setting"));
	/*
       	if(serverdata.l3data.rd_time >= 0)
	    header.set("Instrument.Rd_Time",  new Subs::Hint(serverdata.l3data.rd_time,    "L3CCD parameter"));
	if(serverdata.l3data.rs_time >= 0)
	    header.set("Instrument.Rs_Time",  new Subs::Hint(serverdata.l3data.rs_time,    "L3CCD parameter"));
	*/
    }
    header.set("Instrument.exp_delay", new Subs::Hfloat(uinfo.expose_time*uinfo.time_units, "Exposure delay (seconds)"));

    if(serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_CLEAR){
	header.set("Instrument.Readout_Mode_Flag", new Subs::Hint(serverdata.readout_mode, "Full frame with a clear each exposure"));

    }else if(serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_NOCLEAR){
	header.set("Instrument.Readout_Mode_Flag", new Subs::Hint(serverdata.readout_mode, "Full frame with only a clear at the start"));

    }else if(serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_OVERSCAN){
	header.set("Instrument.Readout_Mode_Flag", new Subs::Hint(serverdata.readout_mode, "Full frame with a clear each exposure and an overscan"));

    }else if(serverdata.readout_mode == Ultracam::ServerData::WINDOWS){
	header.set("Instrument.Readout_Mode_Flag", new Subs::Hint(serverdata.readout_mode, "Standard windowed mode"));

    }else if(serverdata.readout_mode == Ultracam::ServerData::DRIFT){
	header.set("Instrument.Readout_Mode_Flag", new Subs::Hint(serverdata.readout_mode, "Drift mode"));

    }else if(serverdata.readout_mode == Ultracam::ServerData::WINDOWS_CLEAR){
	header.set("Instrument.Readout_Mode_Flag", new Subs::Hint(serverdata.readout_mode, "Two-windows-with-clear mode"));

    }else if(serverdata.readout_mode == Ultracam::ServerData::L3CCD_WINDOWS){
	header.set("Instrument.Readout_Mode_Flag", new Subs::Hint(serverdata.readout_mode, "L3CCD standard mode"));

    }else if(serverdata.readout_mode == Ultracam::ServerData::L3CCD_DRIFT){
	header.set("Instrument.Readout_Mode_Flag", new Subs::Hint(serverdata.readout_mode, "L3CCD drift mode"));

    }else{
	throw Input_Error("parseXML error: no readout mode identified.");
    }


    // Trimming information
    header.set("Trimming",         new Subs::Hdirectory("Trimming information"));
    header.set("Trimming.applied", new Subs::Hbool(ok_to_trim, "Was trimming carried out or not?"));
    if(ok_to_trim){
	header.set("Trimming.ncols", new Subs::Hint(ncol, "Number of columns near readouts removed"));
	header.set("Trimming.nrows", new Subs::Hint(nrow, "Number of rows at bottom of windows removed"));
    }

    // Site information
    header.set("Site",             new Subs::Hdirectory("Observing site information"));
    header.set("Site.Observatory", new Subs::Hstring(uinfo.observatory, "Name of the observing site"));
    header.set("Site.Telescope",   new Subs::Hstring(uinfo.telescope,   "Name of the telescope"));

    
    // Set the exposure time info.
    serverdata.time_units  = uinfo.time_units;
    serverdata.expose_time = uinfo.expose_time*uinfo.time_units;
    serverdata.ybin        = uinfo.ybin;
    serverdata.xbin        = uinfo.xbin;
    serverdata.window      = uinfo.wind;
    serverdata.gain_speed  = uinfo.gain_speed;

    return;
}


// interpret filesave status info
void parse_filesave_status(const DOMNode* const node, Uinfo& uinfo, Ultracam::ServerData& serverdata){

    using Ultracam::Input_Error;

    DOMNodeList *child = node->getChildNodes();
    DOMNodeList *grandchild;
    int observatoryCounter = 0;
    int instrumentCounter = 0;
    for(unsigned int i=0; i<child->getLength(); i++){

	if(same(child->item(i)->getNodeName(), "observatory_status")){

	    // observatory status should have observatory and telescope name
	    grandchild = child->item(i)->getChildNodes();

	    for(unsigned int j=0; j<grandchild->getLength(); j++){

		if(same(grandchild->item(j)->getNodeName(), "name")){
		    uinfo.observatory = getTextValue(grandchild->item(j));
		}else if(same(grandchild->item(j)->getNodeName(), "telescope")){
		    uinfo.telescope = getTextValue(grandchild->item(j));
		}

	    }
	    observatoryCounter++;

	}else if(same(child->item(i)->getNodeName(), "instrument_status")){

	    parse_instrument_status(child->item(i), uinfo, serverdata);
	    instrumentCounter++;

	}
    }
  
    bool error = false;
    std::string message;
    if(observatoryCounter == 0){
	message += "Error. XML document has no observatory_status element.\n";
	error = true;
    }else if(observatoryCounter != 1){
	message += 
	    "Warning. XML document has multiple observatory_status element.\n" 
	    "Only the last one will be counted.\n";
	error = true;
    }
    if(instrumentCounter == 0){
	message += "Error. XML document has no instrument_status element.\n";
	error = true;
    }else if(instrumentCounter != 1){
	message += 
	    "Warning. XML document has multiple instrument_status elements.\n" 
	    "Only the last one will be counted.\n";
	error = true;
    }
    if(error) throw Input_Error(message);

}

/* This is now pretty horrific as it has to cope with standard ultracam xml data which comes in several forms
 * and also the more recent L3CCD stuff */

void parse_instrument_status(const DOMNode* const node, Uinfo& uinfo, Ultracam::ServerData& serverdata){

    using Ultracam::Input_Error;

    DOMNodeList *child = node->getChildNodes();

    // For translating values
    std::string buff;
    std::istringstream istr(buff);
    std::string xl_start, xr_start, x_start, y_start, x_size, y_size;
    int xlStart_, xrStart_, xStart_, yStart_, xSize_, ySize_;
    std::map<int,int> xlStart, xrStart, xStart, yStart, xSize, ySize;

    bool found_instrument = false;

    // First of all, see if we can identify the instrument, ULTRACAM or ULTRASPEC ...
    for(unsigned int j=0; j<child->getLength(); j++){

	if(same(child->item(j)->getNodeName(), "name")){
      
	    std::string instrument = getTextValue(child->item(j));

	    if(Subs::toupper(instrument) == "ULTRACAM"){
		serverdata.instrument = "ULTRACAM";
	    }else if(Subs::toupper(instrument) != "ULTRASPEC" || Subs::toupper(instrument) != "CCD201"){
		serverdata.instrument = "ULTRASPEC";
	    }else{
		throw std::string("Instrument = " + instrument + " not recognised!");
	    }
	    found_instrument = 1;
	}
    }
    if(!found_instrument) throw Input_Error("parseXML error: could not find the instrument.");

    bool found_readout_mode = false;

    // Next look through for the application to define the readout mode.
    // This may not be the best way, but it is the only one at the moment.
    // This includes the applications from both May and September 2002 
    for(unsigned int j=0; j<child->getLength(); j++){
	if(same(child->item(j)->getNodeName(), "application_status")){  
	    if(child->item(j)->getNodeType() == DOMNode::ELEMENT_NODE){	
		if(AttToString((DOMElement*)child->item(j), "id") == "SDSU Exec"){
		    std::string name = AttToString((DOMElement*)child->item(j), "name");
		    serverdata.application = name;
		    
		    // 250 settings from updates of July 2003
		    // ap5b is a two window mode with a clear that allows faster accurately exposed
		    // frames. Change made 19/08/2004
		    if(serverdata.instrument == "ULTRACAM" && 
		       (name.find("ap9_fullframe_mindead") != std::string::npos || 
			name.find("ap9_250_fullframe_mindead") != std::string::npos ||
			name.find("appl9_fullframe_mindead_cfg") != std::string::npos)){
			serverdata.readout_mode = Ultracam::ServerData::FULLFRAME_NOCLEAR;

		    }else if(serverdata.instrument == "ULTRACAM" &&
			     (name.find("ap3_fullframe") != std::string::npos || 
			      name.find("ap3_250_fullframe") != std::string::npos ||
			      name.find("appl3_fullframe_cfg") != std::string::npos)){
			serverdata.readout_mode = Ultracam::ServerData::FULLFRAME_CLEAR;

		    }else if(serverdata.instrument == "ULTRACAM" && 
			     (name.find("ap5b_250_window1pair")   != std::string::npos ||
			      name.find("appl5b_window1pair_cfg") != std::string::npos)){
			serverdata.readout_mode = Ultracam::ServerData::WINDOWS_CLEAR;

		    }else if(serverdata.instrument == "ULTRACAM" && 
			     (name.find("ap5_250_window1pair")   != std::string::npos ||
			      name.find("ap6_250_window2pair")   != std::string::npos ||
			      name.find("ap7_250_window3pair")   != std::string::npos ||
			      name.find("ap5_window1pair")       != std::string::npos ||
			      name.find("ap6_window2pair")       != std::string::npos ||
			      name.find("ap7_window3pair")       != std::string::npos ||
			      name.find("ap_win4_bin8")          != std::string::npos || 
			      name.find("ap_win4_bin1")          != std::string::npos || 
			      name.find("ap_win2_bin2")          != std::string::npos || 
			      name.find("appl5_window1pair_cfg") != std::string::npos ||
			      name.find("appl6_window2pair_cfg") != std::string::npos ||
			      name.find("appl7_window3pair_cfg") != std::string::npos)){

			serverdata.readout_mode = Ultracam::ServerData::WINDOWS;

		    }else if(serverdata.instrument == "ULTRACAM" && 
			     (name.find("drift") != std::string::npos ||
			      name.find("appl8_driftscan_cfg") != std::string::npos)){
			serverdata.readout_mode = Ultracam::ServerData::DRIFT;

		    }else if(serverdata.instrument == "ULTRACAM" && 
			     (name.find("frameover") != std::string::npos ||
			      name.find("appl4_frameover_cfg") != std::string::npos)){
			serverdata.readout_mode = Ultracam::ServerData::FULLFRAME_OVERSCAN;

		    }else if(serverdata.instrument == "ULTRASPEC" && 
			     (name.find("ccd201_winbin_con") != std::string::npos ||
			      name.find("ccd201_winbin_cfg") != std::string::npos)){
			serverdata.readout_mode = Ultracam::ServerData::L3CCD_WINDOWS;

		    }else if(serverdata.instrument == "ULTRASPEC" && 
			     (name.find("ccd201_driftscan_cfg") != std::string::npos)){
			serverdata.readout_mode = Ultracam::ServerData::L3CCD_DRIFT;

		    }else{
			throw Input_Error("parseXML error: unrecognised application & readout mode = [" + name + "]");
		    }
		    found_readout_mode = true;
		}
	    }else{
		throw Input_Error("parseXML error: application_status node was not an element node");
	    }
	}
    }
    if(!found_readout_mode) throw Input_Error("parseXML error: could not find readout mode.");

    /* Now we grind through all the possible parameters */

    bool found_exposure = false, found_gain_speed = false, found_number_of_exposures=false;
    bool found_xbin = false, found_ybin = false, found_v_ft_clk = false, found_gain = false; 
    bool found_hv_gain = false, found_en_clr = false, found_output = false, found_version = false;
    //    bool found_rd_time = false, found_rs_time = false, found_led_flsh = false, found_speed = false;
    bool found_led_flsh = false, found_speed = false;
    bool found_nblue = false;
    uinfo.nccd = 0;

    // maximum number of windows we search for
    const int NMAX = 10;

    for(unsigned int j=0; j<child->getLength(); j++){

	if(same(child->item(j)->getNodeName(), "detector_status")){
      
	    if(child->item(j)->getNodeType() == DOMNode::ELEMENT_NODE){
		uinfo.nccd++;
	    }else{
		throw Input_Error("parseXML error: detector_status was not an element node!");
	    }
      
	}else if(same(child->item(j)->getNodeName(), "parameter_status")){
      
	    if(child->item(j)->getNodeType() == DOMNode::ELEMENT_NODE){

		if(AttToString((DOMElement*)child->item(j), "name") == "EXPOSE_TIME" && serverdata.instrument == "ULTRACAM"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> uinfo.expose_time;
		    if(!istr) throw Input_Error("parseXML error: Could not translate exposure time");
		    istr.clear();
		    found_exposure = true;
	    
		}else if(AttToString((DOMElement*)child->item(j), "name") == "DWELL"  && serverdata.instrument == "ULTRASPEC"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> uinfo.expose_time;
		    if(!istr) throw Input_Error("parseXML error: Could not translate exposure time");
		    istr.clear();
		    found_exposure = true;
	    
		}else if(AttToString((DOMElement*)child->item(j), "name") == "GAIN_SPEED" && serverdata.instrument == "ULTRACAM"){
		    uinfo.gain_speed = AttToString((DOMElement*)child->item(j), "value");
		    found_gain_speed = true;

		}else if((AttToString((DOMElement*)child->item(j), "name") == "NO_EXPOSURES" && serverdata.instrument == "ULTRACAM")||
			 (AttToString((DOMElement*)child->item(j), "name") == "NUM_EXPS"  && serverdata.instrument == "ULTRASPEC")){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> uinfo.number_of_exposures;
		    if(!istr) throw Input_Error("parseXML error: Could not translate number of exposures");
		    istr.clear();
		    found_number_of_exposures = true;

		}else if((AttToString((DOMElement*)child->item(j), "name") == "X_BIN_FAC" && serverdata.instrument == "ULTRACAM") ||
			 (AttToString((DOMElement*)child->item(j), "name") == "X_BIN"     && serverdata.instrument == "ULTRASPEC")){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> uinfo.xbin;
		    if(!istr) throw Input_Error("parseXML error: Could not translate X binning factor");
		    istr.clear();
		    found_xbin = true;

		}else if((AttToString((DOMElement*)child->item(j), "name") == "Y_BIN_FAC" && serverdata.instrument == "ULTRACAM") ||
			 (AttToString((DOMElement*)child->item(j), "name") == "Y_BIN"     && serverdata.instrument == "ULTRASPEC")){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> uinfo.ybin;
		    if(!istr) throw Input_Error("parseXML error: Could not translate Y binning factor");
		    istr.clear();
		    found_ybin = true;

		}else if(AttToString((DOMElement*)child->item(j), "name") == "NBLUE"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.nblue;
		    if(!istr) throw Input_Error("parseXML error: Could not translate NBLUE u-band skip factor");
		    istr.clear();
		    found_nblue = true;

		}else if((AttToString((DOMElement*)child->item(j), "name") == "CLR_EN" || 
			  AttToString((DOMElement*)child->item(j), "name") == "EN_CLR") && serverdata.instrument == "ULTRASPEC"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.l3data.en_clr;
		    if(!istr) throw Input_Error("parseXML error: Could not translate L3CCD EN_CLR parameter");
		    istr.clear();
		    found_en_clr = true;

		}else if(AttToString((DOMElement*)child->item(j), "name") == "GAIN" && serverdata.instrument == "ULTRASPEC"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.l3data.gain;
		    if(!istr) throw Input_Error("parseXML error: Could not translate L3CCD GAIN parameter");
		    istr.clear();
		    found_gain = true;

		}else if(AttToString((DOMElement*)child->item(j), "name") == "HV_GAIN" && serverdata.instrument == "ULTRASPEC"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.l3data.hv_gain;
		    if(!istr) throw Input_Error("parseXML error: Could not translate L3CCD HV_GAIN parameter");
		    istr.clear();
		    found_hv_gain = true;

		}else if(AttToString((DOMElement*)child->item(j), "name") == "OUTPUT" && serverdata.instrument == "ULTRASPEC"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.l3data.output;
		    if(!istr) throw Input_Error("parseXML error: Could not translate L3CCD OUTPUT parameter");
		    istr.clear();
		    found_output = true;

		    /*
		}else if(AttToString((DOMElement*)child->item(j), "name") == "RD_TIME" && serverdata.instrument == "ULTRASPEC"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.l3data.rd_time;
		    if(!istr) throw Input_Error("parseXML error: Could not translate L3CCD RD_TIME parameter");
		    istr.clear();
		    found_rd_time = true;

		}else if(AttToString((DOMElement*)child->item(j), "name") == "RS_TIME" && serverdata.instrument == "ULTRASPEC"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.l3data.rs_time;
		    if(!istr) throw Input_Error("parseXML error: Could not translate L3CCD RS_TIME parameter");
		    istr.clear();
		    found_rs_time = true;
		    */

		}else if(AttToString((DOMElement*)child->item(j), "name") == "SPEED" && serverdata.instrument == "ULTRASPEC"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.l3data.speed;
		    if(!istr) throw Input_Error("parseXML error: Could not translate L3CCD SPEED parameter");
		    istr.clear();
		    found_speed = true;

		}else if(AttToString((DOMElement*)child->item(j), "name") == "LED_FLSH" && serverdata.instrument == "ULTRASPEC"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.l3data.led_flsh;
		    if(!istr) throw Input_Error("parseXML error: Could not translate L3CCD LED_FLSH parameter");
		    istr.clear();
		    found_led_flsh = true;

		}else if(AttToString((DOMElement*)child->item(j), "name") == "VERSION" || AttToString((DOMElement*)child->item(j), "name") == "REVISION"){
		    if(found_version)
			throw Input_Error("parseXML error: two or more of VERSION and REVISION found.");
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
		    istr >> serverdata.version;
		    if(!istr)
			std::cerr << "parseXML warning: Could not translate VERSION/REVISION parameter; will look for user-defined version" << std::endl;
		    else
			found_version = true;
		    istr.clear();

		}else if(AttToString((DOMElement*)child->item(j), "name") == "V_FT_CLK" && serverdata.instrument == "ULTRACAM"){
		    istr.str(AttToString((DOMElement*)child->item(j), "value"));
	    
		    // Try to interpret the vclock string.
		    // When read as a 4-byte int on little-endian (big-endian) machines, only the 
		    // third (second) byte is significant. There are two possibilities determined 
		    // by the most significant bit with the delay given by 80+(fine/course adjustment)*multiplier 
		    // in nanosecs and the number we want being 6 times this. The fine/course adjustment is
		    // 20ns if MSB = 0, 160ns if MSB = 1. The multiplier is given by the remaining
		    // bits obtained here by subtraction of 128 if MSB=1.
	    
		    int iread;
		    istr >> iread;
		    if(!istr) throw Input_Error("parseXML error: Could not translate V_FT_CLK");
		    istr.clear();
		    if(Subs::is_little_endian())
			serverdata.v_ft_clk = *((unsigned char*)(&iread)+2);
		    else
			serverdata.v_ft_clk = *((unsigned char*)(&iread)+1);
		    found_v_ft_clk = true;
	    
		}else if(serverdata.readout_mode == Ultracam::ServerData::WINDOWS || 
			 serverdata.readout_mode == Ultracam::ServerData::DRIFT   ||
			 serverdata.readout_mode == Ultracam::ServerData::WINDOWS_CLEAR){
	    
		    // wind through windows.	  
		    for(int n=0; n<NMAX; n++){
		
			// define the variables we are looking for
			xl_start = "X" + Subs::str(n+1) + "L_START";
			xr_start = "X" + Subs::str(n+1) + "R_START";
			y_start  = "Y" + Subs::str(n+1) + "_START";
			x_size   = "X" + Subs::str(n+1) + "_SIZE";
			y_size   = "Y" + Subs::str(n+1) + "_SIZE";
		
			// look for them. the number found is recorded as the length of the respective
			// vectors.
		
			if(AttToString((DOMElement*)child->item(j), "name") == xl_start){
			    istr.str(AttToString((DOMElement*)child->item(j), "value"));
			    istr >> xlStart_;
			    if(!istr) throw Input_Error("parseXML error: Could not translate lower std::left start X pixel of std::left window");
			    istr.clear();
			    xlStart[n] = xlStart_;
		    
			}else if(AttToString((DOMElement*)child->item(j), "name") == xr_start){
			    istr.str(AttToString((DOMElement*)child->item(j), "value"));
			    istr >> xrStart_;
			    if(!istr) throw Input_Error("parseXML error: Could not translate lower std::left start X pixel of right window");
			    istr.clear();
			    xrStart[n] = xrStart_;
		    
			}else if(AttToString((DOMElement*)child->item(j), "name") == y_start){
			    istr.str(AttToString((DOMElement*)child->item(j), "value"));
			    istr >> yStart_;
			    if(!istr) throw Input_Error("parseXML error: Could not translate lower std::left start Y pixel of left/right windows");
			    istr.clear();
			    yStart[n] = yStart_;
		    
			}else if(AttToString((DOMElement*)child->item(j), "name") == x_size){
			    istr.str(AttToString((DOMElement*)child->item(j), "value"));
			    istr >> xSize_;
			    if(!istr) throw Input_Error("parseXML error: Could not translate X size of left/right windows");
			    istr.clear();
			    xSize[n] = xSize_;
		    
			}else if(AttToString((DOMElement*)child->item(j), "name") == y_size){
			    istr.str(AttToString((DOMElement*)child->item(j), "value"));
			    istr >> ySize_;
			    if(!istr) throw Input_Error("parseXML error: Could not translate Y size of left/right windows");
			    istr.clear();
			    ySize[n] = ySize_;
			}
		    }

		}else if(serverdata.readout_mode == Ultracam::ServerData::L3CCD_WINDOWS ||
			 serverdata.readout_mode == Ultracam::ServerData::L3CCD_DRIFT){

		    // wind through windows
		    for(int n=0; n<NMAX; n++){
		
			// define the variables we are looking for
			x_start = "X" + Subs::str(n+1) + "_START";
			y_start = "Y" + Subs::str(n+1) + "_START";
			x_size  = "X" + Subs::str(n+1) + "_SIZE";
			y_size  = "Y" + Subs::str(n+1) + "_SIZE";
		
			// look for them. the number found is recorded as the length of the respective
			// vectors.
		
			if(AttToString((DOMElement*)child->item(j), "name") == x_start){
			    istr.str(AttToString((DOMElement*)child->item(j), "value"));
			    istr >> xStart_;
			    if(!istr) throw Input_Error("parseXML error: Could not translate lower left start X pixel");
			    istr.clear();
			    xStart[n] = xStart_;
		    
			}else if(AttToString((DOMElement*)child->item(j), "name") == y_start){
			    istr.str(AttToString((DOMElement*)child->item(j), "value"));
			    istr >> yStart_;
			    if(!istr) throw Input_Error("parseXML error: Could not translate lower left start Y pixel");
			    istr.clear();
			    yStart[n] = yStart_;
		    
			}else if(AttToString((DOMElement*)child->item(j), "name") == x_size){
			    istr.str(AttToString((DOMElement*)child->item(j), "value"));
			    istr >> xSize_;
			    if(!istr) throw Input_Error("parseXML error: Could not translate X size");
			    istr.clear();
			    xSize[n] = xSize_;

			}else if(AttToString((DOMElement*)child->item(j), "name") == y_size){
			    istr.str(AttToString((DOMElement*)child->item(j), "value"));
			    istr >> ySize_;
			    if(!istr) throw Input_Error("parseXML error: Could not translate Y size");
			    istr.clear();
			    ySize[n] = ySize_;

			}
		    }
		    if(serverdata.readout_mode == Ultracam::ServerData::L3CCD_DRIFT){
			// just 2 windows with the same ystart and size
			yStart[1] = yStart[0];
			ySize[1]  = ySize[0];
		    }
		}

	    }else{
		throw Input_Error("parseXML error: parameter_status node was not an element node");
	    }
	}
    }
  
    // Check that we have found what we expected to find
 
    if(!found_exposure)   throw Input_Error("parseXML error: could not find exposure time.");
    if(!found_number_of_exposures) throw Input_Error("parseXML error: could not find number of exposures.");
    if(!found_xbin) throw Input_Error("parseXML error: could not find X bin factor.");
    if(!found_ybin) throw Input_Error("parseXML error: could not find Y bin factor.");
    if(!found_version) serverdata.version = -1;
    if(serverdata.instrument == "ULTRACAM"){
	if(!found_gain_speed) throw Input_Error("parseXML error: could not find gain speed.");
	if(!found_v_ft_clk && !found_version){
	    serverdata.which_run    = Ultracam::ServerData::MAY_2002;
	    serverdata.v_ft_clk = 0;
	}else{
	    serverdata.which_run    = Ultracam::ServerData::OTHERS;
	}   
	if(!found_nblue) serverdata.nblue = 0;

    }else if(serverdata.instrument == "ULTRASPEC"){
	if(!found_en_clr){
	    if(serverdata.readout_mode == Ultracam::ServerData::L3CCD_WINDOWS)
		throw Input_Error("parseXML error: could not find L3CCD parameter EN_CLR.");
	    else
		serverdata.l3data.en_clr = false;
	}		
	if(!found_gain)    serverdata.l3data.gain = -1;
	if(!found_hv_gain) throw Input_Error("parseXML error: could not find L3CCD parameter HV_GAIN.");
	if(!found_output)  throw Input_Error("parseXML error: could not find L3CCD parameter OUTPUT.");
	//	if(!found_rs_time) serverdata.l3data.rs_time   = -1;
	//	if(!found_rd_time) serverdata.l3data.rd_time   = -1;
	if(!found_speed)   serverdata.l3data.speed     = -1;
	if(!found_led_flsh) serverdata.l3data.led_flsh = -1;
	serverdata.nblue = 0;
	serverdata.which_run    = Ultracam::ServerData::OTHERS;

    }else{
	throw Input_Error("parseXML error: expecting ULTRACAM or ULTRASPEC!!");
    }

    int last = NMAX;

    // Now check on the windows we have found.
    Ultracam::Wind wind;
    if(serverdata.readout_mode == Ultracam::ServerData::WINDOWS || 
       serverdata.readout_mode == Ultracam::ServerData::DRIFT   ||
       serverdata.readout_mode == Ultracam::ServerData::WINDOWS_CLEAR){

	if(xlStart.size() != xrStart.size() || xrStart.size() != yStart.size() ||
	   yStart.size() != xSize.size() || xSize.size() != ySize.size())
	    throw Input_Error("parseXML error: differing numbers of window parameters found.");
    
	if(xlStart.size() <= 0) throw Input_Error("parseXML error: no window parameters found");
    
	typedef std::map<int, int>::const_iterator CI;
	CI xlStart_it, xrStart_it, yStart_it, xSize_it, ySize_it;
	for(int n=0; n<NMAX; n++){
	    if((xlStart_it = xlStart.find(n)) != xlStart.end() &&
	       (xrStart_it = xrStart.find(n)) != xrStart.end() &&
	       (yStart_it  = yStart.find(n))  != yStart.end()  &&    
	       (xSize_it   = xSize.find(n))   != xSize.end()   &&    
	       (ySize_it   = ySize.find(n))   != ySize.end()){
	
		// set up left window
		wind.llx = xlStart_it->second;
		wind.lly = yStart_it->second;
		wind.nx  = xSize_it->second;
		if(wind.nx % uinfo.xbin != 0)
		    throw Input_Error("parseXML error: X binning factor does not divide into X size");
		wind.nx /= uinfo.xbin;
	
		wind.ny  = ySize_it->second;
		if(wind.ny % uinfo.ybin != 0)
		    throw Input_Error("parseXML error: Y binning factor does not divide into Y size");
		wind.ny /= uinfo.ybin;
	
		// store left window
		uinfo.wind.push_back(wind);
	
		// set up right window. only change is in x start
		wind.llx = xrStart_it->second;
	
		// store right window
		uinfo.wind.push_back(wind);

	    }else{
		last = n;
		break;
	    }
	}
	if(xlStart.size() != size_t(last))
	    throw Input_Error("parseXML error: number of windows differs from numbers of window parameters found.");

    }else if(serverdata.readout_mode == Ultracam::ServerData::L3CCD_WINDOWS ||
	     serverdata.readout_mode == Ultracam::ServerData::L3CCD_DRIFT){

	typedef std::map<int, int>::const_iterator CI;
	CI xStart_it, yStart_it, xSize_it, ySize_it;
	for(int n=0; n<NMAX; n++){
	    if((xStart_it = xStart.find(n)) != xStart.end() &&
	       (yStart_it = yStart.find(n)) != yStart.end() &&    
	       (xSize_it  = xSize.find(n))  != xSize.end()  &&    
	       (ySize_it  = ySize.find(n))  != ySize.end()  &&
	       xSize_it->second && ySize_it->second){

		// Compute number of binned pixels that must be chopped from the start to eliminate overscan.
		int nchop = std::max(0,17-xStart_it->second);
		if(nchop % uinfo.xbin == 0){
		    nchop /= uinfo.xbin;
		}else{
		    nchop = nchop / uinfo.xbin + 1;
		}
		wind.nx  = xSize_it->second - nchop;

		// Compute the new start pixels which depends upon the output port as well 
		// as the number of pixels chopped
		if(serverdata.l3data.output == 0){
		    wind.llx = std::max(1, xStart_it->second + nchop*uinfo.xbin - 16);
		}else{
		    wind.llx = std::max(1, 1074 - xStart_it->second - xSize_it->second*uinfo.xbin);
		}
		serverdata.l3data.nchop.push_back(nchop);

		wind.lly = yStart_it->second;
		wind.ny  = ySize_it->second;

		// store window
		uinfo.wind.push_back(wind);

	    }else{
		last = n;
		break;
	    }
	}

    }else{

	// full frame case there are just two windows spanning half the chip each. Dimensions 512 by 1024
	// divided by the binning factors (always possible), or 540 by 1032 in the overscan case. 
	// However, we actually turn this into 6 windows, allocating 4 separate windows to the
	// overscan and keeping the physical windows as 512 by 1024

	// left physical window first
	wind.llx = 1;
	wind.lly = 1;
	wind.nx  = 512/uinfo.xbin;
	wind.ny  = 1024/uinfo.ybin;
	uinfo.wind.push_back(wind);

	// right physical window
	wind.llx = 513;
	wind.lly = 1;
	wind.nx  = 512/uinfo.xbin;
	wind.ny  = 1024/uinfo.ybin;
	uinfo.wind.push_back(wind);

	if(serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_OVERSCAN){

	    // Left overscan, moved over to the right
	    wind.llx = 1025;
	    wind.lly = 1;
	    wind.nx  = 28/uinfo.xbin;
	    wind.ny  = 1032/uinfo.ybin;
	    uinfo.wind.push_back(wind);

	    // Right overscan
	    wind.llx = 1053;
	    wind.lly = 1;
	    wind.nx  = 28/uinfo.xbin;
	    wind.ny  = 1032/uinfo.ybin;
	    uinfo.wind.push_back(wind);

	    // Top-left overscan
	    wind.llx = 1;
	    wind.lly = 1025;
	    wind.nx  = 512/uinfo.xbin;
	    wind.ny  = 8/uinfo.ybin;
	    uinfo.wind.push_back(wind);

	    // Top-right overscan
	    wind.llx = 513;
	    wind.lly = 1025;
	    wind.nx  = 512/uinfo.xbin;
	    wind.ny  = 8/uinfo.ybin;
	    uinfo.wind.push_back(wind);

	}
    }
}


void parse_data_status(const DOMNode* node, Ultracam::ServerData& serverdata){

    using Ultracam::Input_Error;
  
    std::string buff;
    std::istringstream istr(buff);

    if(node->getNodeType() == DOMNode::ELEMENT_NODE){

	// Read values from data_status. Meaning of WARNING not known but allowed after request by Vik 11/08/03
	if(AttToString((DOMElement*)node, "status") != "OK" &&
	   AttToString((DOMElement*)node, "status") != "WARNING")
	    throw Input_Error("parseXML error: data status is set neither to OK nor to WARNING");

	if(AttToString((DOMElement*)node, "status") == "WARNING")
	    std::cerr << "parseXML warning: data status = WARNING" << std::endl;

	istr.str(AttToString((DOMElement*)node, "framesize"));
	istr >> serverdata.framesize;
	if(!istr) throw Input_Error("parseXML error: could not translate framesize");
	istr.clear();

	istr.str(AttToString((DOMElement*)node, "wordsize"));
	istr >> serverdata.wordsize;
	if(!istr) throw Input_Error("parseXML error: could not translate wordsize");
	istr.clear();

	// Now look at its children
	DOMNodeList *child = node->getChildNodes();

	for(unsigned int j=0; j<child->getLength(); j++){

	    if(same(child->item(j)->getNodeName(), "header_status")){
      
		if(AttToString((DOMElement*)child->item(j), "status") != "OK")
		    throw Input_Error("parseXML error: header status is not OK");

		istr.str(AttToString((DOMElement*)child->item(j), "headerwords"));
		istr >> serverdata.headerwords;
		if(!istr) throw Input_Error("parseXML error: could not translate headerwords");

	    }
	}      

    }else{
	throw Input_Error("data status node is not an XML element node");
    }
}

// interpret user information
void parse_user(const DOMNode* const node, Uinfo& uinfo, Ultracam::ServerData& serverdata){

    using Ultracam::Input_Error;

    DOMNodeList *child = node->getChildNodes();

    std::string buff;
    std::istringstream istr(buff);

    uinfo.user_info = true;
    uinfo.revision = -1;
    for(unsigned int i=0; i<child->getLength(); i++){
	if(same(child->item(i)->getNodeName(), "target")){
	    uinfo.target = getTextValue(child->item(i));
	}else if(same(child->item(i)->getNodeName(), "filters")){
	    uinfo.filters = getTextValue(child->item(i));
	}else if(same(child->item(i)->getNodeName(), "ID")){
	    uinfo.id = getTextValue(child->item(i));
	}else if(same(child->item(i)->getNodeName(), "PI")){
	    uinfo.pi = getTextValue(child->item(i));
	}else if(same(child->item(i)->getNodeName(), "Observers")){
	    uinfo.observers = getTextValue(child->item(i));
	}else if(same(child->item(i)->getNodeName(), "grating")){
	    uinfo.grating = getTextValue(child->item(i));
	}else if(same(child->item(i)->getNodeName(), "slit_angle")){
	    uinfo.slit_angle = getTextValue(child->item(i));
	}else if(same(child->item(i)->getNodeName(), "revision")){
	    istr.str(getTextValue(child->item(i)));
	    istr >> uinfo.revision;
	    if(!istr) throw Input_Error("parseXML error: Could not translate user revision number");
	    istr.clear();
	}
    }
}

// Gets the value associated with an element of the form
// <element>here is some text</element>. 
// i.e. in this case it would return "here is some text".
// If it was
// <element>here is <another>something else</another>some text</element>. 
// it would still return "here is some text" as it should because it just looks
// at text child nodes of the node provided.
    
std::string getTextValue(const DOMNode* node){
    DOMNodeList *child = node->getChildNodes();
    std::string accum;
    char *cpt;
    if(child->getLength() > 0){
	for(unsigned int i=0; i<child->getLength(); i++){
	    if(child->item(i)->getNodeType() == DOMNode::TEXT_NODE){
		if((cpt = XMLString::transcode(child->item(i)->getNodeValue()))){
		    accum.append(cpt);
		    delete[] cpt;
		}
	    }else{
		throw Ultracam::Input_Error("parseXML error: null pointer inside getTextValue");
	    }
	}
    }else{
	return "";
    }
    return accum;
}

//! tests for equality of native and local string
bool same(const XMLCh* const native, const char* const local){
    XMLCh *npt = XMLString::transcode(local);
    bool  comp = (XMLString::compareString(native, npt) == 0);
    XMLString::release(&npt);
    return comp;
}

//! Translate native to local strings
const std::string XtoString(const XMLCh* const native){
    char* cpt = XMLString::transcode(native);
    std::string temp(cpt);
    XMLString::release(&cpt);
    return temp;
}

//! Return named attribute as a local string
std::string AttToString(const DOMElement* elem, const char* const name){
    XMLCh* xpt1 = XMLString::transcode(name);
    char  *cpt  = XMLString::transcode(elem->getAttribute(xpt1));
    std::string temp(cpt);
    XMLString::release(&xpt1);
    XMLString::release(&cpt);
    return temp;
}
