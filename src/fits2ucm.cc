/*

!!begin
!!title    Converts from FITS to Ultracam format
!!author   T.R. Marsh
!!created  14 Sep 2002
!!revised  21 Oct 2010
!!root     fits2ucm
!!index    fits2ucm
!!descr    converts a FITS file to an ULTRACAM ucm file
!!css      style.css
!!class    Programs
!!class    IO
!!class    FITS
!!head1    fits2ucm - converts an Ultracam frame to FITS format

!!emph{fits2ucm} reads a FITS file or many FITS files of CCD images
and converts it/them to Ultracam format files. Different FITS formats may well
require updates to this routine to adjust for different header names. Usually
I base these on one or two example frames so they may not be completely
general.  The times in the headers are corrected to the mid-exposure time as
assumed in the ultracam software. The file name is preserved by this routine
so that 'abc.fits' for instance becomes 'abc.ucm'

If you want to add another format, please look at the code for instructions.

!!head2 Invocation

fits2ucm data format dtype

!!head2 Command line arguments

!!table

!!arg{data}{The file name or list of file names. If it ends with either ".fit", ".fits", ".fts" or ".FIT" it is assumed to be a single fits file, otherwise it is assumed to a list. If
you get a message about not opening SIMPLE it is probably thinking a genuine fits file is a list.}
!!arg{format}{Format of the FITS data. Choices at the moment are:
!!emph{JKT} for the 1m JKT on La Palma, 
!!emph{AUX} for the WHT's AUX Port, 
!!emph{Faulkes} for FITS data from the Faulkes telescope, 
!!emph{Dolores} for the TNG's Dolores, 
!!emph{FORS1} for VLT FORS1 data,
!!emph{EFOSC} for NTT EFOSC data,
!!emph{SAAO} for the UCT camera at SAAO, 
!!emph{NOT} for NOT ALFOSC data,
!!emph{NOTP} for NOT ALFOSC polarimetric data,
!!emph{ATC} for the ULTRASPEC multi-image FITS files from Derek Ives,
!!emph{RATCAM} RATCAM on the LT
!!emph{RISE} for Don Pollacco's camera for the LT,
!!emph{ACAM} for the WHT's ACAM replacement for AUX,
!!emph{SOFI} for split data from SOFI as produced by Steven Parsons,
!!emph{ST7} for an SBIG ST-7 CCD camera,
!!emph{ST10} for an SBIG ST-10 CCD camera,
!!emph{IAC80} for the IAC80 telescope at Izana,
!!emph{FASTCAM} for the Andor iXon DU-897 EMCCD camera used in FASTCAM 
(on the TCS, NOT, WHT and GRANTECAN), 
!!emph{QSI} for a QSI532 CCD camera,
!!emph{BUSCA} for the Calar Alto BUSCA camera (but only one CCD at a time). See !!ref{busca2ucm.html}{busca2ucm}
for an attempt to combine the different BUSCA filters.
!!emph{BASIC} minimal image only

}
!!arg{intout}{true for 2-byte integer output, false for floats. If you specify the integer format, ensure that
you are not losing precision by so doing.}
!!table

Related routines: !!ref{ucm2fits.html}{ucm2fits}, !!ref{grab2fits.html}{grab2fits}, !!ref{busca2ucm.html}{busca2ucm}

!!end

*/

// Every time you see READ THIS, there will be comments on what to do.

#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include "cfitsio/fitsio.h"
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_ultracam.h"

void Tokenize(const std::string& str, std::vector<std::string>& tokens,
	      const std::string& delimiters = " "){

    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);

    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);
    
    while (std::string::npos != pos || string::npos != lastPos){

	// Found a token, add it to the vector.
	tokens.push_back(str.substr(lastPos, pos - lastPos));
	// Skip delimiters.  Note the "not_of"
	lastPos = str.find_first_not_of(delimiters, pos);
	// Find next "non-delimiter"
	pos = str.find_first_of(delimiters, lastPos);
    }
}


int main(int argc, char* argv[]){

    try{

	// Construct Input object
	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	// Define inputs
	input.sign_in("data",      Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("format",    Subs::Input::LOCAL, Subs::Input::PROMPT);
	input.sign_in("intout",    Subs::Input::LOCAL, Subs::Input::PROMPT);

	std::string fname;
	input.get_value("data", fname, "run001", "data file or name of list of data files");

	// Read file or list
	std::vector<std::string> flist;
	if(fname.find(".fit")  == fname.length() - 4  || 
	   fname.find(".fits") == fname.length() - 5 ||
	   fname.find(".FIT") == fname.length() - 4 ||
	   fname.find(".fts")  == fname.length() - 4){
	    flist.push_back(fname);
	}else{      
	    std::ifstream istr(fname.c_str());
	    while(istr >> fname){
		flist.push_back(fname);
	    }
	    istr.close();
	    if(flist.size() == 0) throw Ultracam::Input_Error("No file names loaded");
	}

	std::string format;
	input.get_value("format", format, "JKT", "data format (JKT, AUX, Faulkes, ... etc ... junk for a full list)");
	format = Subs::toupper(format);

	// READ THIS
	// You need to decide on a name for your new format and add it to the following statement
	if(format != "JKT" && format != "AUX" && format != "FAULKES" && format != "DOLORES" && format != "FORS1" && \
	   format != "SAAO" && format != "NOT" && format != "ATC" && format != "RATCAM" && format != "RISE" && \
	   format != "ACAM" && format != "SOFI" && format != "ST7" && format != "ST10" && format != "IAC80" && \
	   format != "FASTCAM" && format != "QSI" && format != "BUSCA" && format != "EFOSC" && format != "NOTP")
	  throw Ultracam::Input_Error("Unrecognised format = " + format + ". Valid choices are:\n\n"
				      "JKT     --- 1m JKT on La Palma\n"
				      "AUX     --- 4.2m WHT's Aux Port camera\n"
				      "Faulkes --- Faulkes telescopes\n"
				      "Dolores --- 3.6m TNG's Dolores\n"
				      "FORS1   --- VLT FORS1\n"
				      "EFOSC   --- NTT EFOSC\n"
				      "SAAO    --- UCT camera data from SAAO\n"
				      "NOT     --- NOT ALFOSC data\n"
				      "NOTP    --- NOT ALFOSC polarimetric data\n"
				      "ATC     --- Derek Ives' multi-image FITS \n"
				      "RATCAM  --- 2m Liverpool Telescope RATCAM\n"
				      "RISE    --- 2m Liverpool Telescope RISE\n"
				      "ACAM    --- WHT's ACAM\n"
				      "SOFI    --- IR data from SOFI/NTT\n"
				      "ST7     --- SBIG ST-7 camera\n" 
				      "ST10    --- SBIG ST-10 camera\n" 
				      "IAC80   --- CCD camera on the IAC80 at Izana \n"
				      "FASTCAM --- FASTCAM on the NOT, WHT or TCS\n"
				      "QSI     --- QSI532 CCD Camera\n"
				      "BUSCA   --- BUSCA camera at Calar Alto");

	bool intout;
	input.get_value("intout", intout, false, "2-byte integer output (else float)?");

	std::string fits;

	char card[FLEN_CARD], errmsg[FLEN_ERRMSG];
	int xbin, ybin, llx, lly;

	for(size_t nfile=0; nfile<flist.size(); nfile++){
	  
	    fits = flist[nfile];

	    // open the file
	    fitsfile *fptr;
	    int status = 0;
	    if(fits_open_file(&fptr, fits.c_str(), READONLY, &status)){
		fits_get_errstatus(status, errmsg);
		fits_close_file(fptr, &status);
		throw Ultracam::Ultracam_Error("fits2ucm: " + fits + ": " + errmsg);
	    }

	    // First deal with the headers, except ATC format for which
	    // we deal with header and data.

	    // READ THIS
	    //
	    // If you want to add a new format, you need to add further items to the next if block which
	    // in the majority of cases deals with headers only, and the succeeding if block for data. Please 
	    // try to add to the end of the if block to preserve the relative order of formats as much as
	    // possible. This makes it easier to navigate this very long file. You can pick up any header 
	    // items you like, but above all UT_date and Exposure must be set. Look below for examples.
	    //
	    // Note that ULTRACAM's native format allows you to store multiple CCDs but usually you will only
	    // want to store one. It also allows you to store any number of sub-windows of a CCD so requires 
	    // you to supply the maximum dimensions, needed so that one can correctly display the positioning 
	    // of the windows within the CCD.
	    //
	    // A statement like:
	    // 		    
	    // data[0].push_back(Ultracam::Windata(x1_start,y1_start,dims[0],dims[1],xbin,ybin,1072,1072));
	    //
	    // is adding a window to the first CCD with dimensions 1072 by 1072.

	    if(format == "ATC"){

		// Ensure that we begin at the beginning, then read the headers.
		int hdutype;
		fits_movabs_hdu(fptr, 1, &hdutype, &status);
		fits_read_key(fptr,TINT,"X_BIN",&xbin,NULL,&status);
		fits_read_key(fptr,TINT,"Y_BIN",&ybin,NULL,&status);
	
		int x1_start, y1_start, x1_size, y1_size;
		fits_read_key(fptr,TINT,"X1_START",&x1_start,NULL,&status);
		fits_read_key(fptr,TINT,"Y1_START",&y1_start,NULL,&status);
		fits_read_key(fptr,TINT,"X1_SIZE", &x1_size, NULL,&status);
		fits_read_key(fptr,TINT,"Y1_SIZE", &y1_size, NULL,&status);
	
		int x2_start, y2_start, x2_size, y2_size;
		fits_read_key(fptr,TINT,"X2_START",&x2_start,NULL,&status);
		fits_read_key(fptr,TINT,"Y2_START",&y2_start,NULL,&status);
		fits_read_key(fptr,TINT,"X2_SIZE", &x2_size, NULL,&status);
		fits_read_key(fptr,TINT,"Y2_SIZE", &y2_size, NULL,&status);

		int nhdu;
		fits_get_num_hdus(fptr, &nhdu, &status);
		if(status){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("ATC 01: " + fits + ": " + errmsg);
		}
	
		int nwin = 1;
		if(x2_size > 0 && y2_size > 0) nwin++;

		// Loop through all images
		const int NIMAGE = (nhdu - 1) / nwin;
		std::cout << "Number of images = " << NIMAGE << std::endl;

		int naxis, anynul;
		long int dims[2], fpixel[2];
		int bitpix;
		nhdu = 1;
		int ndigit = int(log10(float(NIMAGE))+1);
		for(int nim=0; nim<NIMAGE; nim++){

		    // Create an empty Ultracam frame with 1 CCD.
		    Ultracam::Frame data(1);
	  
		    nhdu++;
		    if(fits_movabs_hdu(fptr, nhdu, &hdutype, &status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("ATC 02: " + fits + ": " + errmsg);
		    }
		    if(hdutype != IMAGE_HDU){
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("ATC 03: " + fits + ": encountered an unexpected non-image, HDU number = " + Subs::str(nhdu));
		    }

		    if(fits_get_img_param(fptr,2, &bitpix, &naxis, dims, &status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("ATC 04: " + fits + ": " + errmsg);
		    }
		    if(naxis != 2){
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("ATC 05: " + fits + ": encountered non-2D data, HDU number = " + Subs::str(nhdu));
		    }
	  
		    // Create a Windata
		    data[0].push_back(Ultracam::Windata(x1_start,y1_start,dims[0],dims[1],xbin,ybin,1072,1072));
		      
		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
		      
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			if(fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][data[0].size()-1].row(j), &anynul, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error(std::string("ATC 06: ") + fits + std::string(": ") + std::string(errmsg));
			}
			fpixel[1]++;
		    }

		    if(nwin == 2){

			nhdu++;
			if(fits_movabs_hdu(fptr, nhdu, &hdutype, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error("ATC 07: " + fits + ": " + errmsg);
			}
			if(hdutype != IMAGE_HDU){
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error("ATC 07: " + fits + ": encountered an unexpected non-image, HDU number = " + Subs::str(nhdu));
			}

			if(fits_get_img_param(fptr,2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error("ATC 08: " + fits + ": " + errmsg);
			}
			if(naxis != 2){
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error("ATC 09: " + fits + ": encountered non-2D data, HDU number = " + Subs::str(nhdu));
			}
	  
			// Create a Windata
			data[0].push_back(Ultracam::Windata(x2_start,y2_start,dims[0],dims[1],xbin,ybin,1072,1072));
		      
			// Start reading from lower left
			fpixel[0] = fpixel[1] = 1;
	    
			// Read the data in, row by row 
			for(long j=0; j<dims[1]; j++){
			    if(fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][data[0].size()-1].row(j), &anynul, &status)){
				fits_get_errstatus(status, errmsg);
				fits_close_file(fptr, &status); 
				throw Ultracam::Ultracam_Error(std::string("ATC 10: ") + fits + std::string(": ") + std::string(errmsg));
			    }
			    fpixel[1]++;
			}
		    }

		    // Write out the ultracam file
		    std::string name = fits.substr(0,fits.find_last_of('.')) + "_" + Subs::str(nim+1, ndigit);
		    data.write(name, intout ? Ultracam::Windata::RAW : Ultracam::Windata::NORMAL);
		    std::cerr << name << ".ucm written to disk." << std::endl;

		}

	    }else{

		// Create an empty Ultracam frame with 1 CCD.
		Ultracam::Frame data(1);

		// Ensure that we begin at the beginning
		int hdutype;
		if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("01: " + fits + ": " + errmsg);
		}
	
		// Now format dependent header stuff
	
		if(format == "JKT"){
	  
		    // Read binning factors, time etc.
		    fits_read_key(fptr,TINT,"CCDXBIN",&xbin,NULL,&status);
		    fits_read_key(fptr,TINT,"CCDYBIN",&ybin,NULL,&status);
		    double mjd;
		    fits_read_key(fptr,TDOUBLE,"MJD-OBS",&mjd,NULL,&status);
		    float exposure;
		    fits_read_key(fptr,TFLOAT,"EXPOSED",&exposure,NULL,&status);
	  
		    if(status){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("JKT 01: " + fits + ": " + errmsg);
		    }
	  
		    Subs::Time ut_date(mjd);
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));

		}else if(format == "AUX"){
	  
		    char cbuff[256];
	  
		    // Read binning factors, time etc.
		    fits_read_key(fptr,TINT,"CCDXBIN",&xbin,NULL,&status);
		    fits_read_key(fptr,TINT,"CCDYBIN",&ybin,NULL,&status);
		    fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status);
		    if(status){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("AUX 01: " + fits + ": " + errmsg);
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
	  
		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("12: " + fits + ": " + errmsg);
		    }
		    int day, month, year;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();
	  
		    if(fits_read_key(fptr,TSTRING,"UTSTART",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("13: " + fits + ": " + errmsg);
		    }
		    int hour, minute;
		    double second;
		    istr.str(cbuff);
		    istr >> hour >> c >> minute >> c >> second;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate time = ") + std::string(cbuff));
		    istr.clear();
	  
		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPOSED",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("14: " + fits + ": " + errmsg);
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);
	  
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  
	  
		}else if(format == "FAULKES"){

		    char cbuff[256];

		    // Read binning factors, time etc.
		    if(fits_read_key(fptr,TINT,"CCDXBIN",&xbin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("15: " + fits + ": " + errmsg);
		    }
		    if(fits_read_key(fptr,TINT,"CCDYBIN",&ybin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("16: " + fits + ": " + errmsg);
		    }

		    if(fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("17: " + fits + ": " + errmsg);
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));

		    if(fits_read_key(fptr,TSTRING,"DATE",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("18: " + fits + ": " + errmsg);
		    }

		    int day, month, year;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();

		    if(fits_read_key(fptr,TSTRING,"UTSTART",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("19: " + fits + ": " + errmsg);
		    }

		    int hour, minute;
		    double second;
		    istr.str(cbuff);
		    istr >> hour >> c >> minute >> c >> second;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate time = ") + cbuff);
		    istr.clear();

		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("20: " + fits + ": " + errmsg);
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);

		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "DOLORES"){

		    char cbuff[256];

		    // Read binning factors, time etc.
		    if(fits_read_key(fptr,TINT,"CRDELT1",&xbin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("21: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(fits_read_key(fptr,TINT,"CRDELT2",&ybin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("22: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    if(fits_read_key(fptr,TSTRING,"OBJCAT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("23: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));

		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("24: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    int day, month, year;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();

		    if(fits_read_key(fptr,TSTRING,"EXPSTART",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("25: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    int hour, minute, second;
		    istr.str(cbuff);
		    istr >> hour >> c >> minute >> c >> second;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate time = ") + std::string(cbuff));
		    istr.clear();

		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("25: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);

		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "FORS1"){

		    char cbuff[256];
	  
		    // Read binning factors, time etc.
		    if(fits_read_key(fptr,TINT,"HIERARCH ESO DET WIN1 BINX",&xbin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("27: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(fits_read_key(fptr,TINT,"HIERARCH ESO DET WIN1 BINY",&ybin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("28: ") + fits + std::string(": ") + std::string(errmsg));
		    }
	  
		    if(fits_read_key(fptr,TSTRING,"HIERARCH ESO OBS TARG NAME",&cbuff,NULL,&status)){
			status = 0;
			if(fits_read_key(fptr,TSTRING,"HIERARCH ESO OBS NAME",&cbuff,NULL,&status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("29: ") + fits + std::string(": ") + std::string(errmsg));
			}
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
	  
		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("30: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    int day, month, year;
		    int hour, minute;
		    float second;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day >> c >> hour >> c >> minute >> c >> second ;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();

		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("31: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);
	  
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "SAAO"){

		    char cbuff[256];

		    // SAAO headers don't seem to have binning factors
		    xbin = ybin = 1;

		    if(fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("32: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));

		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("33: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    int day, month, year;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();

		    if(fits_read_key(fptr,TSTRING,"UT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("34: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    int hour, minute, second;
		    istr.str(cbuff);
		    istr >> hour >> c >> minute >> c >> second;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate time = ") + std::string(cbuff));
		    istr.clear();

		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"ITIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("35: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);

		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "NOT"){

		    char cbuff[256];

		    // Read binning factors, time etc.
		    if(fits_read_key(fptr,TINT,"CDELT1",&xbin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			std::cerr << "status  = " << status << ", message = " << errmsg << std::endl;
			throw Ultracam::Ultracam_Error("36: " + fits + ": " + errmsg);
		    }
		    if(fits_read_key(fptr,TINT,"CDELT2",&ybin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("37: " + fits + ": " + errmsg);
		    }

		    if(fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("38: " + fits + ": " + errmsg);
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));

		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("39: " + fits + ": " + errmsg);
		    }
		    int day, month, year;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();

		    double hour;
		    if(fits_read_key(fptr,TDOUBLE,"UT",&hour,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("40: " + fits + ": " + errmsg);
		    }

		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("41: " + fits + ": " + errmsg);
		    }
		    Subs::Time ut_date(day, month, year, hour);
		    ut_date.add_second(exposure/2.);
	  
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		    if(fits_read_key(fptr,TSTRING,"FILENAME",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("42: " + fits + ": " + errmsg);
		    }
		    data.set("Filename", new Subs::Hstring(std::string(cbuff), "Original NOT/ALFOSC file name"));

		}else if(format == "NOTP"){

		    char cbuff[256];

		    // Read binning factors, time etc.
		    if(fits_read_key(fptr,TINT,"DETXBIN",&xbin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			std::cerr << "status  = " << status << ", message = " << errmsg << std::endl;
			throw Ultracam::Ultracam_Error("36: " + fits + ": " + errmsg);
		    }
		    if(fits_read_key(fptr,TINT,"DETYBIN",&ybin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("37: " + fits + ": " + errmsg);
		    }

		    if(fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("38: " + fits + ": " + errmsg);
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));

		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("39: " + fits + ": " + errmsg);
		    }
		    int day, month, year;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();

		    double hour;
		    if(fits_read_key(fptr,TDOUBLE,"UT",&hour,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("40: " + fits + ": " + errmsg);
		    }

		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("41: " + fits + ": " + errmsg);
		    }
		    Subs::Time ut_date(day, month, year, hour);
		    ut_date.add_second(exposure/2.);
	  
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		    if(fits_read_key(fptr,TSTRING,"FILENAME",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("42: " + fits + ": " + errmsg);
		    }
		    data.set("Filename", new Subs::Hstring(std::string(cbuff), "Original NOT/ALFOSC file name"));

		}else if(format == "RISE"){

		    char cbuff[256];

		    // Read binning factors, time etc.
		    fits_read_key(fptr,TINT,"CCDXBIN",&xbin,NULL,&status);
		    fits_read_key(fptr,TINT,"CCDYBIN",&ybin,NULL,&status);		    
		    
		    if(fits_read_key(fptr,TSTRING,"DATE",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("RISE 01: " + fits + ": " + errmsg);
		    }
		    int day, month, year;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day;
		    if(!istr)
			throw Ultracam::Ultracam_Error("RISE 02: failed to translate date = " + std::string(cbuff));
		    istr.clear();
	  
		    if(fits_read_key(fptr,TSTRING,"UTSTART",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("RISE 03: " + fits + ": " + errmsg);
		    }
		    int hour, minute;
		    double second;
		    istr.str(cbuff);
		    istr >> hour >> c >> minute >> c >> second;
		    if(!istr)
			throw Ultracam::Ultracam_Error("RISE 04: failed to translate time = " + std::string(cbuff));
		    istr.clear();
	  
		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("RISE 05: " + fits + ": " + errmsg);
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);
	  
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "RATCAM"){

		    char cbuff[256];

		    // Read binning factors, time etc.
		    fits_read_key(fptr,TINT,"CCDXBIN",&xbin,NULL,&status);
		    fits_read_key(fptr,TINT,"CCDYBIN",&ybin,NULL,&status);		    
		    
		    if(fits_read_key(fptr,TSTRING,"DATE",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("RATCAM 01: " + fits + ": " + errmsg);
		    }
		    int day, month, year;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day;
		    if(!istr)
			throw Ultracam::Ultracam_Error("RATCAM 02: failed to translate date = " + std::string(cbuff));
		    istr.clear();
	  
		    if(fits_read_key(fptr,TSTRING,"UTSTART",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("RATCAM 03: " + fits + ": " + errmsg);
		    }
		    int hour, minute;
		    double second;
		    istr.str(cbuff);
		    istr >> hour >> c >> minute >> c >> second;
		    if(!istr)
			throw Ultracam::Ultracam_Error("RATCAM 04: failed to translate time = " + std::string(cbuff));
		    istr.clear();
	  
		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("RATCAM 05: " + fits + ": " + errmsg);
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);
	  
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "ACAM"){
	  
		    char cbuff[256];
	  
		    // Read binning factors, time etc.
		    fits_read_key(fptr,TINT,"CCDXBIN",&xbin,NULL,&status);
		    fits_read_key(fptr,TINT,"CCDYBIN",&ybin,NULL,&status);
		    fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status);
		    if(status){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("AUX 01: " + fits + ": " + errmsg);
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
	  
		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("12: " + fits + ": " + errmsg);
		    }
		    int day, month, year;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();
	  
		    if(fits_read_key(fptr,TSTRING,"UTSTART",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("13: " + fits + ": " + errmsg);
		    }
		    int hour, minute;
		    double second;
		    istr.str(cbuff);
		    istr >> hour >> c >> minute >> c >> second;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate time = ") + std::string(cbuff));
		    istr.clear();
	  
		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPOSED",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("14: " + fits + ": " + errmsg);
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);
	  
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "SOFI"){
	  
		    char cbuff[256];
	  
		    // No binning with SOFI
		    xbin = ybin = 1;

		    if(fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("SOFI 01: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
			
		    double mjd;
		    fits_read_key(fptr,TDOUBLE,"MJD-OBS",&mjd,NULL,&status);
		    float exposure;
		    fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status);
		    if(status){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("SOFI 02: " + fits + ": " + errmsg);
		    }
		    
		    Subs::Time ut_date(mjd);
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));
			
		    // Read binning factors, time etc.
		    fits_read_key(fptr,TINT,"STARTX",&llx,NULL,&status);
		    fits_read_key(fptr,TINT,"STARTY",&lly,NULL,&status);
		    if(status){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("SOFI 03: " + fits + ": " + errmsg);
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
	  
		}else if(format == "ST7" || format == "ST10"){

		    char cbuff[256];
		    
		    // Read binning factors, time etc.
		    if(fits_read_key(fptr,TINT,"XBINNING",&xbin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("SBIG 01: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(fits_read_key(fptr,TINT,"YBINNING",&ybin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("SBIG 02: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    
		    if(fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("SBIG 03: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
		    
		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("SBIG 04: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    int day, month, year;
		    int hour, minute;
		    float second;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day >> c >> hour >> c >> minute >> c >> second ;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();
		    
		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("SBIG 05: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);
		    
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "IAC80"){

		  char cbuff[256];
		  
		  // Read and parse binning factors
		  if(fits_read_key(fptr,TSTRING,"CCDSUM",&cbuff,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("IAC80 00: ") + fits + std::string(": ") + std::string(errmsg));
		  }
		  std::string fitsHead = cbuff;
		  std::vector<std::string> binFacs;
		  Tokenize(fitsHead,binFacs);
		  xbin = Subs::string_to_int(binFacs[0]);
		  ybin = Subs::string_to_int(binFacs[1]);
		  
		  if(fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("IAC80 01: ") + fits + std::string(": ") + std::string(errmsg));
		  }
		  data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
		  
		  if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("IAC80 02: " + fits + ": " + errmsg);
		  }
		  int day, month, year;
		  std::string buff;
		  std::istringstream istr(buff);
		  istr.str(cbuff);
		  char c;
		  istr >> year >> c >> month >> c >> day;
		  if(!istr)
		    throw Ultracam::Ultracam_Error("IAC80 03: failed to translate date = " + std::string(cbuff));
		  istr.clear();
		  
		  if(fits_read_key(fptr,TSTRING,"UT",&cbuff,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("IAC80 04: " + fits + ": " + errmsg);
		  }
		  int hour, minute;
		  double second;
		  istr.str(cbuff);
		  istr >> hour >> c >> minute >> c >> second;
		  if(!istr)
		    throw Ultracam::Ultracam_Error("IAC80 05: failed to translate time = " + std::string(cbuff));
		  istr.clear();
		  
		  float exposure;
		  if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("IAC80 06: " + fits + ": " + errmsg);
		  }
		  Subs::Time ut_date(day, month, year, hour, minute, second);
		  ut_date.add_second(exposure/2.);
	  
		  // store time
		  data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		  data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "FASTCAM"){
		  
		  char cbuff[256];
		  
		  // Read binning factors
		  if(fits_read_key(fptr,TSTRING,"BINNING",&cbuff,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("FASTCAM 01: ") + fits + std::string(": ") + std::string(errmsg));
		  }
		  std::string fitsHead = cbuff;
		  std::vector<std::string> binFacs;
		  Tokenize(fitsHead,binFacs,"X");
		  xbin = Subs::string_to_int(binFacs[0]);
		  ybin = Subs::string_to_int(binFacs[1]);
		  
		  if(fits_read_key(fptr,TSTRING,"OBJNAME",&cbuff,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("FASTCAM 02: ") + fits + std::string(": ") + std::string(errmsg));
		  }
		  data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
		  
		  if(fits_read_key(fptr,TSTRING,"DATE",&cbuff,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("FASTCAM 03: " + fits + ": " + errmsg);
		  }
		  int day, month, year;
		  std::string buff;
		  std::istringstream istr(buff);
		  istr.str(cbuff);
		  char c;
		  istr >> year >> c >> month >> c >> day;
		  if(!istr)
		    throw Ultracam::Ultracam_Error("FASTCAM 04: failed to translate date = " + std::string(cbuff));
		  istr.clear();
		  
		  if(fits_read_key(fptr,TSTRING,"UT",&cbuff,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("FASTCAM 05: " + fits + ": " + errmsg);
		  }
		  int hour, minute;
		  double second;
		  istr.str(cbuff);
		  istr >> hour >> c >> minute >> c >> second;
		  if(!istr)
		    throw Ultracam::Ultracam_Error("FASTCAM 06: failed to translate time = " + std::string(cbuff));
		  istr.clear();
		  
		  float exposure;
		  if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("FASTCAM 07: " + fits + ": " + errmsg);
		  }
		  Subs::Time ut_date(day, month, year, hour, minute, second);
		  ut_date.add_second(exposure/2000.);
		  
		  // store time
		  data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		  data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  
		  
		  
		}else if(format == "QSI"){
		    
		    char cbuff[256];
		    
		    // Read binning factors, time etc.
		    if(fits_read_key(fptr,TINT,"XBINNING",&xbin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("QSI 01: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(fits_read_key(fptr,TINT,"YBINNING",&ybin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("QSI 02: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    
		    if(fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("QSI 03: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
		    
		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("QSI 04: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    int day, month, year;
		    int hour, minute;
		    float second;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day >> c >> hour >> c >> minute >> c >> second ;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("Failed to translate date = ") + std::string(cbuff));
		    istr.clear();
		    
		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("QSI 05: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);
		    
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}else if(format == "BUSCA"){
		    
		    char cbuff[256];
		    
		    if(fits_read_key(fptr,TSTRING,"OBJECT",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("BUSCA 01: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));

		    if(fits_read_key(fptr,TSTRING,"SEQUENCE",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("BUSCA 02: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    std::string sequence(cbuff);
		    if(sequence.find("_bin2") == std::string::npos)
			throw Ultracam::Ultracam_Error(std::string("BUSCA 03: ") + fits + std::string(": unrecognised sequence = ") + 
						       sequence);

		    // Assume no binning until proven otherwise
		    xbin = 2;
		    ybin = 2;			  
		    
		    double jd;
		    fits_read_key(fptr,TDOUBLE,"JULIAN",&jd,NULL,&status);
		    
		    if(fits_read_key(fptr,TSTRING,"SHOPEN",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("BUSCA 03: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    int hour, minute, second;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> hour >> c >> minute >> c >> second ;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("BUSCA 04: Failed to translate SHOPEN = ") + std::string(cbuff));
		    istr.clear();

		    float exposure;
		    fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status);
		    if(status){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("BUSCA 04: " + fits + ": " + errmsg);
		    }

		    //Create a time
		    Subs::Time ut_date(int(jd-2400000.5) + (hour + minute/60. + (second+exposure/2.)/3600.)/24.);
	  
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));

		}else if(format == "EFOSC"){

		    char cbuff[256];
	  
		    // Read binning factors, time etc.
		    if(fits_read_key(fptr,TINT,"HIERARCH ESO DET WIN1 BINX",&xbin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("EFOSC 01: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(fits_read_key(fptr,TINT,"HIERARCH ESO DET WIN1 BINY",&ybin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("EFOSC 02: ") + fits + std::string(": ") + std::string(errmsg));
		    }
	  
		    if(fits_read_key(fptr,TSTRING,"HIERARCH ESO OBS TARG NAME",&cbuff,NULL,&status)){
			status = 0;
			if(fits_read_key(fptr,TSTRING,"HIERARCH ESO OBS NAME",&cbuff,NULL,&status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("EFOSC 03: ") + fits + std::string(": ") + std::string(errmsg));
			}
		    }
		    data.set("Object", new Subs::Hstring(std::string(cbuff), "Object name"));
	  
		    if(fits_read_key(fptr,TSTRING,"DATE-OBS",&cbuff,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("EFOSC 04: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    int day, month, year;
		    int hour, minute;
		    float second;
		    std::string buff;
		    std::istringstream istr(buff);
		    istr.str(cbuff);
		    char c;
		    istr >> year >> c >> month >> c >> day >> c >> hour >> c >> minute >> c >> second ;
		    if(!istr)
			throw Ultracam::Ultracam_Error(std::string("EFOSC 05: failed to translate date = ") + std::string(cbuff));
		    istr.clear();

		    float exposure;
		    if(fits_read_key(fptr,TFLOAT,"EXPTIME",&exposure,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("EFOSC 06: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    Subs::Time ut_date(day, month, year, hour, minute, second);
		    ut_date.add_second(exposure/2.);
	  
		    // store time
		    data.set("UT_date", new Subs::Htime(ut_date, "UTC at mid-eposure"));
		    data.set("Exposure", new Subs::Hfloat(exposure, "Exposure time, seconds"));	  

		}

		// READ THIS
		//
		// Now another if block, which is concerned with getting the data. This varies according to whether you
		// are having to dig through HDUs or use the primary one etc. Again there are quite a few examples to work
		// from. Again please add to the end of the if block to preserve the ordering.

		int nhdu;
		if(fits_get_num_hdus(fptr, &nhdu, &status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("101: " + fits + ": " + errmsg);
		}

		int naxis, anynul;
		long int dims[2], fpixel[2];
		int bitpix;
	
		if(format == "FAULKES"){

		    int windowed;
		    if(fits_read_key(fptr,TLOGICAL,"CCDWMODE",&windowed,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("102: " + fits + ": " + errmsg);
		    }

		    if(windowed){
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("103: ") + fits + std::string(": windowed mode for Faulkes not yet supported by fits2ucm.\n") +
						       std::string("Please send me an example file and I will upgrade it. TRM"));
		    }

		    if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("104: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    if(hdutype == IMAGE_HDU){
			if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("103: ") + fits + std::string(": ") + std::string(errmsg));
			}
			if(naxis == 2){
			    data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,2200,2200));
			}else{
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("104: ") + fits + std::string(": naxis does not equal 2"));
			}
		    }else{
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("105: ") + fits + std::string(": HDU is not an image."));
		    }

		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
	  
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
			if(status){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error(std::string("106: ") + fits + ": row " + Subs::str(j+1) + std::string(". " ) + std::string(errmsg));
			}
			fpixel[1]++;
		    }

		}else if(format == "JKT" || format == "AUX" || format == "ACAM"){
	  
		    // Go through looking for image hdus with NAXIS = 2 and non-zero dimensions
		    // Assume each represents a separate window
		    for(int i=1; i<= nhdu; i++){
			if(!fits_movabs_hdu(fptr, i, &hdutype, &status)){
			    if(hdutype == IMAGE_HDU){
				if(!fits_get_img_param(fptr,2, &bitpix, &naxis, dims, &status)){
				    if(naxis == 2){
					if(!fits_read_key(fptr,TSTRING,"RTDATSEC",card,NULL,&status)){
					    int llx, lly, dum;
					    std::istringstream istr(card);
					    istr.ignore();
					    if(!istr){
						fits_close_file(fptr, &status);
						throw Ultracam::Ultracam_Error("107: error reading data region = " + std::string(card));
					    }
					    istr >> llx;
					    if(!istr){
						fits_close_file(fptr, &status);
						throw Ultracam::Ultracam_Error("108: error reading data region = " + std::string(card));
					    }
					    istr.ignore();
					    if(!istr){
						fits_close_file(fptr, &status);
						throw Ultracam::Ultracam_Error("109: error reading data region = " + std::string(card));
					    }
					    istr >> dum;
					    if(!istr){
						fits_close_file(fptr, &status);
						throw Ultracam::Ultracam_Error("110: error reading data region = " + std::string(card));
					    }
					    istr.ignore();
					    if(!istr){
						fits_close_file(fptr, &status);
						throw Ultracam::Ultracam_Error("111: error reading data region = " + std::string(card));
					    }
					    istr >> lly;
					    if(!istr){
						fits_close_file(fptr, &status);
						throw Ultracam::Ultracam_Error("112: error reading data region = " + std::string(card));
					    }
		      
					    // Create the Windata
					    if(format == "JKT"){
						data[0].push_back(Ultracam::Windata(llx,lly,dims[0],dims[1],xbin,ybin,2088,2120));
					    }else if(format == "AUX"){
						data[0].push_back(Ultracam::Windata(llx,lly,dims[0],dims[1],xbin,ybin,1110,1050));
					    }else if(format == "ACAM"){
						data[0].push_back(Ultracam::Windata(llx,lly,dims[0],dims[1],xbin,ybin,2148,4200));
					    }
		      
					    // Start reading from lower left
					    fpixel[0] = fpixel[1] = 1;
		      
					    // Read the data in, row by row 
					    for(long j=0; j<dims[1]; j++){
						fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][data[0].size()-1].row(j), &anynul, &status);
						if(status){
						    fits_get_errstatus(status, errmsg);
						    fits_close_file(fptr, &status); 
						    throw Ultracam::Ultracam_Error(std::string("113: ") + fits + std::string(": ") + std::string(errmsg));
						}
						fpixel[1]++;
					    }
					}else{
					    fits_get_errstatus(status, errmsg);
					    fits_close_file(fptr, &status);
					    throw Ultracam::Ultracam_Error(std::string("114: ") + fits + std::string(": ") + std::string(errmsg));
					}
				    }
				}else{
				    fits_get_errstatus(status, errmsg);
				    fits_close_file(fptr, &status);
				    throw Ultracam::Ultracam_Error(std::string("115: ") + fits + std::string(": ") + std::string(errmsg));
				}
			    }

			}else{
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("116: ") + fits + std::string(": ") + std::string(errmsg));
			}
		    }

		}else if(format == "DOLORES"){
		    
		    // Read window position
		    int crpix1, crpix2;
		    if(fits_read_key(fptr,TINT,"CRPIX1",&crpix1,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("122: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(fits_read_key(fptr,TINT,"CRPIX2",&crpix2,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("123: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    int detoff1, detoff2;
		    if(fits_read_key(fptr,TINT,"DETOFF1",&detoff1,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("124: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(fits_read_key(fptr,TINT,"DETOFF2",&detoff2,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("125: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    if(hdutype == IMAGE_HDU){
			if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("126: ") + fits + std::string(": ") + std::string(errmsg));
			}
			if(naxis == 2){
			    data[0].push_back(Ultracam::Windata(crpix1+xbin*detoff1,crpix2+xbin*detoff2,dims[0],dims[1],xbin,ybin,2100,2100));
			}else{
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("127: ") + fits + std::string(": naxis does not equal 2"));
			}
		    }else{
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("128: ") + fits + std::string(": HDU is not an image."));
		    }
	  
		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
	  
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
			if(status){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error("129: " + fits + ": row " + Subs::str(j+1) + ". "  + errmsg);
			}
			fpixel[1]++;
		    }  
	
		}else if(format == "FORS1"){
	  
		    if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("130: ") + fits + std::string(": ") + std::string(errmsg));
		    }
	  
		    if(hdutype == IMAGE_HDU){
			if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("131: ") + fits + std::string(": ") + std::string(errmsg));
			}
			if(naxis == 2){
			    data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,2080,2048));
			}else{
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("132: ") + fits + std::string(": naxis does not equal 2"));
			}
		    }else{
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("133: ") + fits + std::string(": HDU is not an image."));
		    }
	
		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
	  
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
			if(status){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error("134: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
			}
			fpixel[1]++;
		    }    

		}else if(format == "SAAO"){

		    if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("135: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    if(hdutype == IMAGE_HDU){
			if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("136: ") + fits + std::string(": ") + std::string(errmsg));
			}
			if(naxis == 2){
			    if(dims[0] == 210 && dims[1] == 144){
				xbin = ybin = 2;
			    }else if(dims[0] == 140 && dims[1] == 96){
				xbin = ybin = 3;
			    }else if(dims[0] == 105 && dims[1] == 72){
				xbin = ybin = 4;
			    }else if(dims[0] == 84 && dims[1] == 57){
				xbin = ybin = 5;
			    }else if(dims[0] == 70 && dims[1] == 48){
				xbin = ybin = 6;
			    }else{
				fits_close_file(fptr, &status);
				throw Ultracam::Ultracam_Error("137: " + fits + " failed to identify binning factors from the window size");
			    }
			    data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,420,288));
			}else{
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error("138: " + fits + ": naxis does not equal 2");
			}
		    }else{
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("139: " + fits + ": HDU is not an image.");
		    }

		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
	  
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
			if(status){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error("140: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
			}
			fpixel[1]++;
		    }
	
		}else if(format == "NOT"){

		    // Read window position
		    int crpix1, crpix2;
		    if(fits_read_key(fptr,TINT,"CRPIX1",&crpix1,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("141: " + fits + ": " + errmsg);
		    }
		    if(fits_read_key(fptr,TINT,"CRPIX2",&crpix2,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("142: " + fits + ": " + errmsg);
		    }

		    if(hdutype == IMAGE_HDU){
			if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error("143: " + fits + ": " + errmsg);
			}
			if(naxis == 2){
			    data[0].push_back(Ultracam::Windata(crpix1,crpix2,dims[0],dims[1],xbin,ybin,2198,2052));
			}else{
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error("144: " + fits + ": naxis does not equal 2");
			}
		    }else{
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("145: " + fits + ": HDU is not an image.");
		    }
	  
		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
	  
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
			if(status){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error("146: " + fits + ": row " + Subs::str(j+1) + ". "  + errmsg);
			}
			fpixel[1]++;
		    }

		}else if(format == "NOTP"){

		  char cbuff[256];

		  // Read and parse binning factors
		  if(fits_read_key(fptr,TSTRING,"DETWIN1",&cbuff,NULL,&status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("NOTP 00: ") + fits + std::string(": ") + std::string(errmsg));
		  }
		  std::string fitsHead = cbuff;
		  std::vector<std::string> binFacs;
		  Tokenize(fitsHead,binFacs,"[]:,");
		  int llx, lly;
		  llx = Subs::string_to_int(binFacs[0]);
		  lly = Subs::string_to_int(binFacs[2]);

		  if(fits_movabs_hdu(fptr, 2, &hdutype, &status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("NOTP 01: ") + fits + std::string(": ") + std::string(errmsg));
		  }

		  if(hdutype == IMAGE_HDU){
		    if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error("NOTP 02: " + fits + ": " + errmsg);
		    }

		    if(naxis == 2){
		      data[0].push_back(Ultracam::Windata(llx,lly,dims[0],dims[1],xbin,ybin,2198,2052));
		    }else{
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error("NOTP 03: " + fits + ": naxis does not equal 2");
		    }
		  }else{
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error("NOTP 04: " + fits + ": HDU is not an image.");
		  }
		  
		  // Start reading from lower left
		  fpixel[0] = fpixel[1] = 1;
		  
		  // Read the data in, row by row 
		  for(long j=0; j<dims[1]; j++){
		    fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
		    if(status){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status); 
		      throw Ultracam::Ultracam_Error("NOTP 05: " + fits + ": row " + Subs::str(j+1) + ". "  + errmsg);
		    }
		    fpixel[1]++;
		  }

		}else if(format == "RISE"){
	  
		    if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("117: ") + fits + std::string(": ") + std::string(errmsg));
		    }
	  
		    if(hdutype == IMAGE_HDU){
			if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("118: ") + fits + std::string(": ") + std::string(errmsg));
			}
			if(naxis == 2){
			    data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,1048,1048));
			}else{
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("119: ") + fits + std::string(": naxis does not equal 2"));
			}
		    }else{
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("120: ") + fits + std::string(": HDU is not an image."));
		    }
	
		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
	  
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
			if(status){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error("121: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
			}
			fpixel[1]++;
		    }    

		}else if(format == "RATCAM"){
	  
		    if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("201: ") + fits + std::string(": ") + std::string(errmsg));
		    }
	  
		    if(hdutype == IMAGE_HDU){
			if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("202: ") + fits + std::string(": ") + std::string(errmsg));
			}
			if(naxis == 2){
			    data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,2160,2048));
			}else{
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("203: ") + fits + std::string(": naxis does not equal 2"));
			}
		    }else{
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("204: ") + fits + std::string(": HDU is not an image."));
		    }
	
		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
	  
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
			if(status){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error("121: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
			}
			fpixel[1]++;
		    }    

  		}else if(format == "SOFI"){
		  
  		  if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
		    fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("147: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    if(hdutype == IMAGE_HDU){
			if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("148: ") + fits + std::string(": ") + std::string(errmsg));
			}
			data[0].push_back(Ultracam::Windata(llx,lly,dims[0],dims[1],xbin,ybin,1024,1024));
		    }else{
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error("149: " + fits + ": HDU is not an image.");
		    }

		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
	  
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
			if(status){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error("150: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
			}
			fpixel[1]++;
		    }
		    
		}else if(format == "ST7" || format == "ST10"){
		  
		  if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("151: ") + fits + std::string(": ") + std::string(errmsg));
		  }
		  
		  if(hdutype == IMAGE_HDU){
		    if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error(std::string("152: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(naxis == 2){
		      if(format == "ST7"){ 
			data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,765,510));
		      }else{
			data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,2184,1472));
		      }
		    }else{
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error(std::string("153: ") + fits + std::string(": naxis does not equal 2"));
		    }
		  }else{
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("154: ") + fits + std::string(": HDU is not an image."));
		  }
	
		  // Start reading from lower left
		  fpixel[0] = fpixel[1] = 1;
	  
		  // Read the data in, row by row 
		  for(long j=0; j<dims[1]; j++){
		    fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
		    if(status){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status); 
		      throw Ultracam::Ultracam_Error("155: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
		    }
		    fpixel[1]++;
		  }    
		
		}else if(format == "FASTCAM"){
	      
		  if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("156: ") + fits + std::string(": ") + std::string(errmsg));
		  }
	      
		  if(hdutype == IMAGE_HDU){
		    if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error(std::string("157: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(naxis == 2){
		      data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,512,512));
		    }else{
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error(std::string("158: ") + fits + std::string(": naxis does not equal 2"));
		    }
		  }else{
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("159: ") + fits + std::string(": HDU is not an image."));
		  }
	      
		  // Start reading from lower left
		  fpixel[0] = fpixel[1] = 1;
	      
		  // Read the data in, row by row 
		  for(long j=0; j<dims[1]; j++){
		    fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
		    if(status){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status); 
		      throw Ultracam::Ultracam_Error("160: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
		    }
		    fpixel[1]++;
		  }    
	      
		}else if(format == "IAC80"){
	      
		  if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("161: ") + fits + std::string(": ") + std::string(errmsg));
		  }
	      
		  if(hdutype == IMAGE_HDU){
		    if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error(std::string("162: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(naxis == 2){
		      data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,2098,2048));
		    }else{
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error(std::string("163: ") + fits + std::string(": naxis does not equal 2"));
		    }
		  }else{
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("164: ") + fits + std::string(": HDU is not an image."));
		  }
	      
		  // Start reading from lower left
		  fpixel[0] = fpixel[1] = 1;
	      
		  // Read the data in, row by row 
		  for(long j=0; j<dims[1]; j++){
		    fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
		    if(status){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status); 
		      throw Ultracam::Ultracam_Error("165: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
		    }
		    fpixel[1]++;
		  }    
	      
		}else if(format == "QSI"){
	  
		  if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
		    fits_get_errstatus(status, errmsg);
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("166: ") + fits + std::string(": ") + std::string(errmsg));
		  }
	  
		  if(hdutype == IMAGE_HDU){
		    if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error(std::string("167: ") + fits + std::string(": ") + std::string(errmsg));
		    }
		    if(naxis == 2){
		      data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,2184,1472));
		    }else{
		      fits_close_file(fptr, &status);
		      throw Ultracam::Ultracam_Error(std::string("168: ") + fits + std::string(": naxis does not equal 2"));
		    }
		  }else{
		    fits_close_file(fptr, &status);
		    throw Ultracam::Ultracam_Error(std::string("169: ") + fits + std::string(": HDU is not an image."));
		  }
	
		  // Start reading from lower left
		  fpixel[0] = fpixel[1] = 1;
	  
		  // Read the data in, row by row 
		  for(long j=0; j<dims[1]; j++){
		    fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
		    if(status){
		      fits_get_errstatus(status, errmsg);
		      fits_close_file(fptr, &status); 
		      throw Ultracam::Ultracam_Error("170: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
		    }
		    fpixel[1]++;
		  }    

		}else if(format == "BUSCA"){
	  
		    char cbuff[256];

		    const int MAXX = 4096;
		    const int MAXY = 4096;
		    
		    // Read number of windows
		    int nwin;
		    if(fits_read_key(fptr,TINT,"WINNR",&nwin,NULL,&status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("180: ") + fits + std::string(": ") + std::string(errmsg));
		    }

		    int yoff = 0;
		    for(int nw=0; nw<nwin; nw++){

		        char *tbuff = const_cast<char*>(("WINXY" + Subs::str(nw)).c_str());
			if(fits_read_key(fptr,TSTRING,tbuff,&cbuff,NULL,&status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error("190: " + fits + ": " + errmsg);
			}
			std::string llxy(cbuff);
			std::string::size_type start = llxy.find('[');			
			std::string::size_type end   = llxy.find(':');
			if(start == std::string::npos || end == std::string::npos)
			    throw Ultracam::Ultracam_Error(std::string("200: ") + fits + std::string(": failed to find '[' or ':' in ") + llxy);
			std::istringstream istr(llxy.substr(start+1,end-start-1));
			int llx;
			istr >> llx;
			if(!istr)
			    throw Ultracam::Ultracam_Error("210: failed to translate llx = " + llxy.substr(start+1,end-start-1));
			istr.clear();

			start = llxy.find(',');			
			end   = llxy.rfind(':');
			if(start == std::string::npos || end == std::string::npos)
			    throw Ultracam::Ultracam_Error(std::string("220: ") + fits + std::string(": failed to find ',' or ':' in ") + llxy);
			istr.str(llxy.substr(start+1,end-start-1));
			int lly;
			istr >> lly;
			if(!istr)
			    throw Ultracam::Ultracam_Error("230: failed to translate lly = " + llxy.substr(start+1,end-start-1));
			istr.clear();

			tbuff = const_cast<char*>(("WINDIM" + Subs::str(nw)).c_str());
			if(fits_read_key(fptr,TSTRING,tbuff,&cbuff,NULL,&status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error("240: " + fits + ": " + errmsg);
			}
			std::string wdims(cbuff);
			start = wdims.find('(');			
			end   = wdims.find(',');
			if(start == std::string::npos || end == std::string::npos)
			    throw Ultracam::Ultracam_Error(std::string("250: ") + fits + std::string(": failed to find '(' or ',' in ") + wdims);
			istr.str(wdims.substr(start+1,end-start-1));
			int nx;
			istr >> nx;
			if(!istr)
			    throw Ultracam::Ultracam_Error("260: failed to translate nx = " + wdims.substr(start+1,end-start-1));
			istr.clear();

			start = end;
			end   = wdims.find(')');
			if(start == std::string::npos || end == std::string::npos)
			    throw Ultracam::Ultracam_Error(std::string("270: ") + fits + std::string(": failed to find ',' or ')' in ") + wdims);
			istr.str(wdims.substr(start+1,end-start-1));
			int ny;
			istr >> ny;
			if(!istr)
			    throw Ultracam::Ultracam_Error("270: failed to translate ny = " + wdims.substr(start+1,end-start-1));
			istr.clear();

			// Finally have window position and dimensions (although there has to be a better way!)
			// Create window
			data[0].push_back(Ultracam::Windata(llx,lly,nx,ny,xbin,ybin,MAXX,MAXY));

			// Start reading 
			fpixel[0] = 1;
			fpixel[1] = 1 + yoff;
	  
			// Read the data in, row by row 
			for(long j=0; j<ny; j++){
			    fits_read_pix(fptr, TFLOAT, fpixel, nx, 0, data[0][data[0].size()-1].row(j), &anynul, &status);
			    if(status){
				fits_get_errstatus(status, errmsg);
				fits_close_file(fptr, &status); 
				throw Ultracam::Ultracam_Error("280: " + fits + ": row " + Subs::str(j+1) + ". "  + errmsg);
			    }
			    fpixel[1]++;
			} 
			yoff += ny;
		    }

		}else if(format == "EFOSC"){
	  
		    if(fits_movabs_hdu(fptr, 1, &hdutype, &status)){
			fits_get_errstatus(status, errmsg);
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("290: ") + fits + std::string(": ") + std::string(errmsg));
		    }
	  
		    if(hdutype == IMAGE_HDU){
			if(fits_get_img_param(fptr, 2, &bitpix, &naxis, dims, &status)){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("291: ") + fits + std::string(": ") + std::string(errmsg));
			}
			if(naxis == 2){
			    data[0].push_back(Ultracam::Windata(1,1,dims[0],dims[1],xbin,ybin,2080,2080));
			}else{
			    fits_close_file(fptr, &status);
			    throw Ultracam::Ultracam_Error(std::string("292: ") + fits + std::string(": naxis does not equal 2"));
			}
		    }else{
			fits_close_file(fptr, &status);
			throw Ultracam::Ultracam_Error(std::string("293: ") + fits + std::string(": HDU is not an image."));
		    }
	
		    // Start reading from lower left
		    fpixel[0] = fpixel[1] = 1;
	  
		    // Read the data in, row by row 
		    for(long j=0; j<dims[1]; j++){
			fits_read_pix(fptr, TFLOAT, fpixel, dims[0], 0, data[0][0].row(j), &anynul, &status);
			if(status){
			    fits_get_errstatus(status, errmsg);
			    fits_close_file(fptr, &status); 
			    throw Ultracam::Ultracam_Error("294: " + fits + ": row " + Subs::str(j+1) + ". " + errmsg);
			}
			fpixel[1]++;
		    }    
	

		}  
		
		// Write out the ultracam file
		data.write(fits.substr(0,fits.find_last_of('.')), intout ? Ultracam::Windata::RAW : Ultracam::Windata::NORMAL);
		std::cerr << fits.substr(0,fits.find_last_of('.')) << ".ucm written to disk." << std::endl;
	      
	    } // end of data if block

	    // Close the fits file
	    fits_close_file(fptr, &status);

	} // next file
    }
    catch(const std::string& err){
	std::cerr << err << std::endl;
    }
}



